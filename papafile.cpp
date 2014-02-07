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
				case texture_t::X8R8G8B8:
					texture.Format = texture_t::X8R8G8B8;
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
			texture.sRGB = (textureinformationheader.SRGB == 1);
			if(texture.Data.length() != textureinformationheader.Length)
			{
				LastError = QString("Failed to read texture data for texture %1").arg(i);
				return false;
			}

			switch(texture.Format)
			{
				case texture_t::A8R8G8B8:
					if(!decodeA8R8G8B8(texture))
					{
						LastError = QString("Failed to decode A8R8G8B8 texture data for texture %1").arg(i);
						return false;
					}
					break;
				case texture_t::X8R8G8B8:
					if(!decodeX8R8G8B8(texture))
					{
						LastError = QString("Failed to decode X8R8G8B8 texture data for texture %1").arg(i);
						return false;
					}
					break;
				case texture_t::DXT1:
					if(!decodeDXT1(texture))
					{
						LastError = QString("Failed to decode DXT1 texture data for texture %1").arg(i);
						return false;
					}
					break;
				case texture_t::DXT5:
					if(!decodeDXT5(texture))
					{
						LastError = QString("Failed to decode DXT5 texture data for texture %1").arg(i);
						return false;
					}
					break;
				default:
					LastError = QString("Failed to decode unsupported texture data for texture %1").arg(i);
					return false;
			}

			Textures.push_back(texture);
		}
	}

	Valid = true;

	return true;
}

bool PapaFile::decodeA8R8G8B8(PapaFile::texture_t& texture)
{
	QImage image = QImage(texture.Width, texture.Height, QImage::Format_ARGB32);

	qDebug() << "decodeA8R8G8B8: length =" << texture.Data.count();
	qDebug() << "decodeA8R8G8B8: width  =" << texture.Width;
	qDebug() << "decodeA8R8G8B8: height =" << texture.Height;

	// Why is length not 4 * width * height? Weird.
	qDebug() << "decodeA8R8G8B8: left over bytes??? =" << (texture.Data.count() - 4 * texture.Height * texture.Width);

	for(int j = 0; j < 4 * texture.Width * texture.Height; j += 4)
	{
		int pixelindex = j / 4;
		image.setPixel(pixelindex % texture.Width, texture.Height - 1 - (pixelindex / texture.Width), qRgba(texture.Data[j], texture.Data[j+1], texture.Data[j+2], texture.Data[j+3]));
	}
	texture.Image.push_back(image);

	return true;
}

bool PapaFile::decodeX8R8G8B8(PapaFile::texture_t& texture)
{
	return false;
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

	for(int i = 0; i < texture.NumberMinimaps - 4; ++i) // TODO remove the -4 later.
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

			if(texture.sRGB)
				convertFromSRGB(palette, 4);

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

bool PapaFile::decodeDXT5(PapaFile::texture_t& texture)
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
	};

	struct row_t
	{
		uchar col0 : 2;
		uchar col1 : 2;
		uchar col2 : 2;
		uchar col3 : 2;
	};

	const struct DXT5
	{
		quint8 alpha0;
		quint8 alpha1;
		quint64 alphabits : 48;
		colour_t colour0;
		colour_t colour1;
		quint32 rgbbits;
	} *ptr;

	qDebug() << "sizeof(DXT5)" << sizeof(DXT5);

	ptr = (const struct DXT5 *)(texture.Data.data());

	for(int i = 0; i < 1/*texture.NumberMinimaps - 4*/; i++) // TODO remove the -4 later.
	{
		int divider = pow(2, i);
		QImage image = QImage(texture.Width / divider, texture.Height / divider, QImage::Format_ARGB32);
		image.fill(qRgb(255, 0, 0));

		for(int j = 0; j < texture.Width * texture.Height / (16 * divider * divider); j++)
		{
			// Calculate the eigth alpha values in use.
			quint8 alphapalette[8];
			alphapalette[0] = ptr->alpha0;
			alphapalette[1] = ptr->alpha1;
			if(ptr->alpha0 > ptr->alpha1)
			{
				for(int k = 0; k < 6; k++)
					alphapalette[k+2] = ((6. - k) * ptr->alpha0 + (k + 1.) * ptr->alpha1) / 7.;
			}
			else
			{
				for(int k = 0; k < 4; k++)
					alphapalette[k+2] = ((4. - k) * ptr->alpha0 + (k + 1.) * ptr->alpha1) / 5.;
				alphapalette[6] = 0;
				alphapalette[7] = 255;
			}

			for(int k = 0; k < 8; k++)
				alphapalette[k] = 255;

			// Get the alpha value for each pixel.
			quint8 alphatexel[4][4];
			quint64 alphabits = ptr->alphabits;// (quint64)ptr->alphabits[0] + 256 * ((quint64)ptr->alphabits[1] + 256 * ((quint64)ptr->alphabits[2] + 256 * ((quint64)ptr->alphabits[3] + 256 * ((quint64)ptr->alphabits[4] + 256 * (quint64)ptr->alphabits[5]))));
			for(int y = 0; y < 4; ++y)
			{
				for(int x = 0; x < 4; ++x)
				{
					alphatexel[x][y] = alphapalette[alphabits & 0b111];
					alphabits >>= 3;
				}
			}

			QRgb palette[4];
			colour_t colour2, colour3;
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


			int x0 = j % (texture.Width/(4*divider));
			int y0 = j / (texture.Height/(4*divider));

			quint32 rgbbits = ptr->rgbbits;
			for(int y = 0; y < 4; ++y)
			{
				for(int x = 0; x < 4; ++x)
				{
					quint8 colourindex = rgbbits & 0b11;
					image.setPixel(4*x0 + x, 4*y0 + y, qRgba(qRed(palette[colourindex]), qRed(palette[colourindex]), qRed(palette[colourindex]), alphatexel[x][y]));
					rgbbits >>= 2;
				}
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
			case texture_t::X8R8G8B8:
				return "X8R8G8B8";
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

void PapaFile::convertFromSRGB(QRgb* palette, int size)
{
	// From https://en.wikipedia.org/w/index.php?title=SRGB&oldid=586514424#The_reverse_transformation
	for(int i = 0; i < size; i++)
	{
		float red = qRed(palette[i]) / 255.;
		if(red <= 0.04045)
			red /= 12.92;
		else
			red = pow((red + 0.055) / (1.055), 2.4);

		float green = qGreen(palette[i]) / 255.;
		if(green <= 0.04045)
			green /= 12.92;
		else
			green = pow((green + 0.055) / (1.055), 2.4);

		float blue = qBlue(palette[i]) / 255.;
		if(blue <= 0.04045)
			blue /= 12.92;
		else
			blue = pow((blue + 0.055) / (1.055), 2.4);

		palette[i] = qRgb(
			255*(0.4124*red + 0.3576*green + 0.1805*blue),
			255*(0.2126*red + 0.7152*green + 0.0722*blue),
			255*(0.0193*red + 0.1192*green + 0.9502*blue)
		);
	}
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

bool PapaFile::setImage(const QImage& image, int textureindex, int mipindex)
{
	if(textureindex < Textures.count())
	{
		if(mipindex < Textures[textureindex].Image.count())
		{
			Textures[textureindex].Image[mipindex] = image;
			return true;
		}
		else
			return false;
	}
	else
		return false;
}


#include "papafile.moc"
