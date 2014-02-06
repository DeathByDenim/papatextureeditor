/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  Jarno van der Kolk <jarno@jarno.ca>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "papafile.h"
#include <QFile>

#include <QDebug>
#include <QImage>
#include <qcolor.h>
#include <cmath>

PapaFile::PapaFile(const QString& filename)
{
	init();
	load(filename);
}

PapaFile::PapaFile()
{
	init();
}

PapaFile::PapaFile(const PapaFile& other)
{
}

PapaFile::~PapaFile()
{
}

PapaFile& PapaFile::operator=(const PapaFile& other)
{
}

void PapaFile::init()
{
	Valid = false;
	LastError = "";
	NumberOfBones = 0;
	NumberOfTextures = 0;
	NumberOfVertexBuffers = 0;
	NumberOfIndexBuffers = 0;
	NumberOfMaterials = 0;
	NumberOfMeshes = 0;
	NumberOfSkeletons = 0;
	NumberOfModels = 0;
}

bool PapaFile::load(QString filename)
{
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
	{
		LastError = "Couldn't open file";
		return false;
	}

	Header papaheader;
	if(file.read((char *)&papaheader, sizeof(Header)) != sizeof(Header))
	{
		LastError = "Failed to read header";
		return false;
	}
	
	if(QByteArray(papaheader.Identification, 4) != "apaP")
	{
		LastError = "Not a PAPA file";
		return false;
	}

	NumberOfBones = papaheader.NumberOfBones;
	NumberOfTextures = papaheader.NumberOfTextures;
	NumberOfVertexBuffers = papaheader.NumberOfVertexBuffers;
	NumberOfIndexBuffers = papaheader.NumberOfIndexBuffers;
	NumberOfMaterials = papaheader.NumberOfMaterials;
	NumberOfMeshes = papaheader.NumberOfMeshes;
	NumberOfSkeletons = papaheader.NumberOfSkeletons;
	NumberOfModels = papaheader.NumberOfModels;


	// Read the bones
	if(papaheader.OffsetBonesHeader >= 0)
	{
		if(!file.seek(papaheader.OffsetBonesHeader))
		{
			LastError = "Failed to seek to BonesHeader";
			return false;
		}
		for(qint64 i = 0; i < NumberOfBones; i++)
		{
			BonesHeader boneheader;
			if(file.read((char *)&boneheader, sizeof(BonesHeader)) != sizeof(BonesHeader))
			{
				LastError = "Failed to read BonesHeader";
				return false;
			}

			qint64 oldpos = file.pos();
			if(!file.seek(boneheader.OffsetBoneName))
			{
				LastError = QString("Failed to seek to bone %1").arg(i);
				return false;
			}
			char *bonename = new char[boneheader.LengthOfBoneName];
			if(file.read(bonename, boneheader.LengthOfBoneName) != boneheader.LengthOfBoneName)
			{
				LastError = QString("Failed to read bone %1").arg(i);
				return false;
			}
			file.seek(oldpos);

			bone_t bone;
			bone.name = QByteArray(bonename, boneheader.LengthOfBoneName);

			Bones.push_back(bone);
		}
	}

	
	// Read the textures
	if(papaheader.OffsetTextureInformation >= 0)
	{
		if(!file.seek(papaheader.OffsetTextureInformation))
		{
			LastError = "Failed to seek to TextureInformation";
			return false;
		}
		for(qint64 i = 0; i < NumberOfTextures; i++)
		{
			TextureInformationHeader textureinformationheader;
			if(file.read((char *)&textureinformationheader, sizeof(TextureInformationHeader)) != sizeof(TextureInformationHeader))
			{
				LastError = QString("Failed to read TextureInformationHeader for texture %1").arg(i);
				return false;
			}

			texture_t texture;
			switch(textureinformationheader.TextureFormat)
			{
				case texture_t::A8R8G8B8:
					texture.Format = texture_t::A8R8G8B8;
					break;
				case texture_t::DXT1:
					texture.Format = texture_t::DXT1;
					break;
				case texture_t::DXT5:
					texture.Format = texture_t::DXT5;
					break;
				default:
					texture.Format = texture_t::Invalid;
			}
			texture.Width = textureinformationheader.Width;
			texture.Height = textureinformationheader.Height;
			texture.Data = file.read(textureinformationheader.Length);
			texture.NumberMinimaps = (int)textureinformationheader.NumberMinimaps;
			if(texture.Data.length() != textureinformationheader.Length)
			{
				LastError = QString("Failed to read texture data for texture %1").arg(i);
				return false;
			}

			if(texture.Format == texture_t::DXT1)
			{
				if(!decodeDXT1(texture))
				{
					LastError = QString("Failed to decode texture data for texture %1").arg(i);
					return false;
				}
			}

			Textures.push_back(texture);
		}
	}

	Valid = true;
	
	return true;
}

bool PapaFile::decodeDXT1(PapaFile::texture_t& texture)
{
	union colour_t
	{
		quint16 value;
		struct
		{
			quint16 red : 5;
			quint16 green : 6;
			quint16 blue : 5;
		} rgb;
		struct
		{
			quint16 red : 5;
			quint16 green : 5;
			quint16 blue : 5;
			quint16 alpha : 1;
		} rgba;
	};

	struct row_t
	{
		uchar col0 : 2;
		uchar col1 : 2;
		uchar col2 : 2;
		uchar col3 : 2;
	};

	const struct DXT1
	{
		colour_t colour0;
		colour_t colour1;
		row_t row[4];
	} *ptr;

	ptr = (const struct DXT1 *)(texture.Data.data());

	for(int i = 0; i < texture.NumberMinimaps - 4; ++i) // remove the -4 later.
	{
		int divider = pow(2, i);
		QImage image = QImage(texture.Width / divider, texture.Height / divider, QImage::Format_ARGB32);
		image.fill(0);

		for(int j = 0; j < texture.Width * texture.Height / (16 * divider * divider); j++)
		{
			colour_t colour2, colour3;
			QRgb palette[4];
			if(ptr->colour0.value > ptr->colour1.value)
			{	// Four colours
				
				// Interpolate the other two colours.
				colour2.rgb.red = (2 * ptr->colour0.rgb.red + ptr->colour1.rgb.red) / 3;
				colour2.rgb.green = (2 * ptr->colour0.rgb.green + ptr->colour1.rgb.green) / 3;
				colour2.rgb.blue = (2 * ptr->colour0.rgb.blue + ptr->colour1.rgb.blue) / 3;
				colour3.rgb.red = (ptr->colour0.rgb.red + 2 * ptr->colour1.rgb.red) / 3;
				colour3.rgb.green = (ptr->colour0.rgb.green + 2 * ptr->colour1.rgb.green) / 3;
				colour3.rgb.blue = (ptr->colour0.rgb.blue + 2 * ptr->colour1.rgb.blue) / 3;

				palette[0] = qRgb(255*ptr->colour0.rgb.red/32, 255*ptr->colour0.rgb.green/64, 255*ptr->colour0.rgb.blue/32);
				palette[1] = qRgb(255*ptr->colour1.rgb.red/32, 255*ptr->colour1.rgb.green/64, 255*ptr->colour1.rgb.blue/32);
				palette[2] = qRgb(255*colour2.rgb.red/32, 255*colour2.rgb.green/64, 255*colour2.rgb.blue/32);
				palette[3] = qRgb(255*colour3.rgb.red/32, 255*colour3.rgb.green/64, 255*colour3.rgb.blue/32);
			}
			else
			{	// Three colours with black
				
				// The remaining colour is the average of the other two colours.
				colour2.rgb.red = (ptr->colour0.rgb.red + ptr->colour1.rgb.red) / 2;  
				colour2.rgb.green = (ptr->colour0.rgb.green + ptr->colour1.rgb.green) / 2;  
				colour2.rgb.blue = (ptr->colour0.rgb.blue + ptr->colour1.rgb.blue) / 2;  
				colour3.value = 0;

				palette[0] = qRgb(255*ptr->colour0.rgb.red/32, 255*ptr->colour0.rgb.green/64, 255*ptr->colour0.rgb.blue/32);
				palette[1] = qRgb(255*ptr->colour1.rgb.red/32, 255*ptr->colour1.rgb.green/64, 255*ptr->colour1.rgb.blue/32);
				palette[2] = qRgb(255*colour2.rgb.red/32, 255*colour2.rgb.green/64, 255*colour2.rgb.blue/32);
				palette[3] = qRgb(0, 0, 0);
			}

			int x = j % (texture.Width/(4*divider));
			int y = j / (texture.Height/(4*divider));
			for(int k = 0; k < 4; k++)
			{
				image.setPixel(4*x, 4*y + k, palette[ptr->row[k].col0]);
				image.setPixel(4*x + 1, 4*y + k, palette[ptr->row[k].col1]);
				image.setPixel(4*x + 2, 4*y + k, palette[ptr->row[k].col2]);
				image.setPixel(4*x + 3, 4*y + k, palette[ptr->row[k].col3]);
			}

			ptr++;
		}
		texture.Image.push_back(image);
	}

	return true;
}

QString PapaFile::format()
{
	if(Textures.length() > 0)
	{
		switch(Textures[0].Format)
		{
			case texture_t::A8R8G8B8:
				return "A8R8G8B8";
			case texture_t::DXT1:
				return "DXT1";
			case texture_t::DXT5:
				return "DXT5";
		}
	}
	
	return "Unsupported";
}

const QImage *PapaFile::image(int textureindex, int mipindex)
{
	if(textureindex < Textures.count())
	{
		if(mipindex < Textures[textureindex].Image.count())
		{
			return &Textures[textureindex].Image[mipindex];
		}
		else
			return NULL;
	}
	else
		return NULL;
}

#include "papafile.moc"
