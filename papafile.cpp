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
#include <QImage>
#include <QColor>
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

PapaFile::~PapaFile()
{
}

void PapaFile::init()
{
	Valid = false;
	Modified = false;
	LastError = "";
}

bool PapaFile::load(QString filename)
{
	Filename = filename;

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

	qint16 numberOfBones = papaheader.NumberOfBones;
	qint16 numberOfTextures = papaheader.NumberOfTextures;
/*	qint16 numberOfVertexBuffers = papaheader.NumberOfVertexBuffers;
	qint16 numberOfIndexBuffers = papaheader.NumberOfIndexBuffers;
	qint16 numberOfMaterials = papaheader.NumberOfMaterials;
	qint16 numberOfMeshes = papaheader.NumberOfMeshes;
	qint16 numberOfSkeletons = papaheader.NumberOfSkeletons;
	qint16 numberOfModels = papaheader.NumberOfModels;
*/
	HeaderUnknowns.Unknown1[0] = papaheader.Unknown1[0];
	HeaderUnknowns.Unknown1[1] = papaheader.Unknown1[1];
	HeaderUnknowns.Unknown2[0] = papaheader.Unknown2[0];
	HeaderUnknowns.Unknown2[1] = papaheader.Unknown2[1];
	HeaderUnknowns.Unknown2[2] = papaheader.Unknown2[2];
	HeaderUnknowns.Unknown2[3] = papaheader.Unknown2[3];

	// Read the bones
	if(papaheader.OffsetBonesHeader >= 0)
	{
		if(!file.seek(papaheader.OffsetBonesHeader))
		{
			LastError = "Failed to seek to BonesHeader";
			return false;
		}
		for(qint64 i = 0; i < numberOfBones; i++)
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

		for(qint64 i = 0; i < numberOfTextures; i++)
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
			texture.Unknowns.Unknown1[0] = textureinformationheader.Unknown1[0];
			texture.Unknowns.Unknown1[1] = textureinformationheader.Unknown1[1];
			texture.Unknowns.Unknown2 = textureinformationheader.Unknown2;
			texture.Unknowns.Unknown3 = textureinformationheader.Unknown3;

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
	LastError = "";

	return true;
}

bool PapaFile::save(QString filename)
{
	if(filename == "")
		filename = Filename;

	if(!Modified && filename == Filename)
		return true;

	QFile papafile(filename);
	if(!papafile.open(QIODevice::ReadWrite))
	{
		LastError = QString("Failed to open %1").arg(filename);
		return false;
	}

	Header papaheader;
	papaheader.Identification[0] = 'a';
	papaheader.Identification[1] = 'p';
	papaheader.Identification[2] = 'a';
	papaheader.Identification[3] = 'P';

	papaheader.Unknown1[0] = HeaderUnknowns.Unknown1[0];
	papaheader.Unknown1[1] = HeaderUnknowns.Unknown1[1];

	papaheader.NumberOfBones = Bones.count();
	papaheader.NumberOfTextures = Textures.count();
	papaheader.NumberOfVertexBuffers = 0;
	papaheader.NumberOfIndexBuffers = 0;
	papaheader.NumberOfMaterials = 0;
	papaheader.NumberOfMeshes = 0;
	papaheader.NumberOfSkeletons = 0;
	papaheader.NumberOfModels = 0;

	papaheader.Unknown2[0] = HeaderUnknowns.Unknown2[0];
	papaheader.Unknown2[1] = HeaderUnknowns.Unknown2[1];
	papaheader.Unknown2[2] = HeaderUnknowns.Unknown2[2];
	papaheader.Unknown2[3] = HeaderUnknowns.Unknown2[3];

	papaheader.OffsetBonesHeader = -1;
	papaheader.OffsetTextureInformation = sizeof(Header);
	papaheader.OffsetVerticesInformation = -1;
	papaheader.OffsetIndicesInformation = -1;
	papaheader.OffsetMaterialInformation = -1;
	papaheader.OffsetMeshInformation = -1;
	papaheader.OffsetSkeletonInformation = -1;
	papaheader.OffsetModelInformation = -1;
	papaheader.OffsetAnimationInformation = -1;

	if(papafile.write((const char *)&papaheader, sizeof(Header)) != sizeof(Header))
	{
		LastError = "Failed to write header.";
		return false;
	}

	for(QList<texture_t>::iterator tex = Textures.begin(); tex != Textures.end(); ++tex)
	{
		switch(tex->Format)
		{
			case texture_t::A8R8G8B8:
				if(!encodeA8R8G8B8(*tex))
				{
					LastError = "Encoding texture in A8R8G8B8 failed.";
					return false;
				}
				break;
			case texture_t::X8R8G8B8:
				if(!encodeX8R8G8B8(*tex))
				{
					LastError = "Encoding texture in X8R8G8B8 failed.";
					return false;
				}
				break;
			case texture_t::DXT1:
				if(!encodeDXT1(*tex))
				{
					LastError = "Encoding texture in DXT1 failed.";
					return false;
				}
				break;
			case texture_t::DXT5:
				if(!encodeDXT5(*tex))
				{
					LastError = "Encoding texture in DXT5 failed.";
					return false;
				}
				break;
			default:
				LastError = QString("Encoding not supported in format %1").arg(tex->Format);
				return false;
		}

		TextureInformationHeader textureinformationheader;
		textureinformationheader.Unknown1[0] = tex->Unknowns.Unknown1[0];
		textureinformationheader.Unknown1[1] = tex->Unknowns.Unknown1[1];
		textureinformationheader.TextureFormat = tex->Format;
		textureinformationheader.NumberMinimaps = tex->NumberMinimaps;
		textureinformationheader.Unknown2 = tex->Unknowns.Unknown2;
		textureinformationheader.SRGB = (tex->sRGB ? 1 : 0);
		textureinformationheader.Width = tex->Width;
		textureinformationheader.Height = tex->Height;
		textureinformationheader.Length = tex->Data.length();
		textureinformationheader.Unknown3 = tex->Unknowns.Unknown3;

		if(papafile.write((const char*)&textureinformationheader, sizeof(TextureInformationHeader)) != sizeof(TextureInformationHeader))
		{
			LastError = "Failed to write texture information header for texture.";
			return false;
		}
		if(papafile.write(tex->Data) != tex->Data.length())
		{
			LastError = "Failed to write data for texture.";
			return false;
		}
	}

	papaheader.OffsetBonesHeader = papafile.pos();
	
	quint64 bonestringpos = papafile.pos() + Bones.count() * sizeof(BonesHeader);
	for(QList<bone_t>::const_iterator bone = Bones.constBegin(); bone != Bones.constEnd(); ++bone)
	{
		BonesHeader boneheader;
		boneheader.LengthOfBoneName = bone->name.length();
		boneheader.OffsetBoneName = bonestringpos;
		bonestringpos += boneheader.LengthOfBoneName;
		
		if(papafile.write((const char *)&boneheader, sizeof(BonesHeader)) != sizeof(BonesHeader))
		{
			LastError = "Failed to write bone header for bone.";
			return false;
		}
	}

	for(QList<bone_t>::const_iterator bone = Bones.constBegin(); bone != Bones.constEnd(); ++bone)
	{
		if(papafile.write(bone->name.toAscii()) != bone->name.length())
		{
			LastError = "Failed to write name for bone.";
			return false;
		}
	}	

	if(!papafile.seek(0))
	{
		LastError = "Failed to seek to beginning.";
		return false;
	}
	if(papafile.write((const char *)&papaheader, sizeof(Header)) != sizeof(Header))
	{
		LastError = "Failed to rewrite header.";
		return false;
	}

	Filename = filename;
	LastError = "";
	Modified = false;

	return true;
}


void PapaFile::findOptimalColours(PapaFile::colour_t& colour0, PapaFile::colour_t& colour1, const QList<colour_t> &colours)
{
	// Find extreme colours
	float maxdistance = 0;
	int col0 = -1, col1 = -1;
	for(int i = 0; i < colours.count(); i++)
	{
		float ri = colours[i].rgb.red / 32.;
		float gi = colours[i].rgb.green / 64.;
		float bi = colours[i].rgb.blue / 32.;
		for(int j = i + 1; j < colours.count(); j++)
		{
			float rj = colours[j].rgb.red / 32.;
			float gj = colours[j].rgb.green / 64.;
			float bj = colours[j].rgb.blue / 32.;

			float distance = (ri - rj) * (ri - rj) + (gi - gj) * (gi - gj) + (bi - bj) * (bi - bj);

			if(distance > maxdistance)
			{
				maxdistance = distance;
				col0 = i;
				col1 = j;
			}
		}
	}
	Q_ASSERT(col0 >= 0);
	Q_ASSERT(col1 >= 0);

	colour_t bestcolour0 = colours[col0];
	colour_t bestcolour1 = colours[col1];
	float lowestchi2 = calculateChi2_four(bestcolour0, bestcolour1, colours);

	for(int i = 0; i < 100; i++)
	{
		colour_t c0 = bestcolour0;
		colour_t c1 = bestcolour1;
		c0.rgb.red += (5.0 * rand()) / RAND_MAX - 2.5;
		c0.rgb.green += (5.0 * rand()) / RAND_MAX - 2.5;
		c0.rgb.blue += (5.0 * rand()) / RAND_MAX - 2.5;
		c1.rgb.red += (5.0 * rand()) / RAND_MAX - 2.5;
		c1.rgb.green += (5.0 * rand()) / RAND_MAX - 2.5;
		c1.rgb.blue += (5.0 * rand()) / RAND_MAX - 2.5;
		float chi2 = calculateChi2_four(c0, c1, colours);

		if(chi2 < lowestchi2)
		{
			lowestchi2 = chi2;
			bestcolour0 = c0;
			bestcolour1 = c1;
		}
	}

	if(bestcolour0.value > bestcolour1.value)
	{
		colour0.value = bestcolour0.value;
		colour1.value = bestcolour1.value;
	}
	else
	{
		colour0.value = bestcolour1.value;
		colour1.value = bestcolour0.value;
	}
}
/*
inline float colourDistance(const colour_t &col1, const colour_t &col2)
{
}
*/
float PapaFile::calculateChi2_four(colour_t col1, colour_t col2, const QList<colour_t> &colours)
{
	colour_t colourts[4];
	colourts[0].value = col1.value;
	colourts[1].rgb.red = (col1.rgb.red + 2 * col2.rgb.red) / 3;
	colourts[1].rgb.green = (col1.rgb.green + 2 * col2.rgb.green) / 3;
	colourts[1].rgb.blue = (col1.rgb.blue + 2 * col2.rgb.blue) / 3;
	colourts[2].rgb.red = (2 * col1.rgb.red + col2.rgb.red) / 3;
	colourts[2].rgb.green = (2 * col1.rgb.green + col2.rgb.green) / 3;
	colourts[2].rgb.blue = (2 * col1.rgb.blue + col2.rgb.blue) / 3;
	colourts[3].value = col2.value;

	float chi2 = 0;
	for(QList<colour_t>::const_iterator col = colours.constBegin(); col != colours.constEnd(); ++col)
	{
		float rcol = col->rgb.red / 32.;
		float gcol = col->rgb.green / 64.;
		float bcol = col->rgb.blue / 32.;
		float mindistance = 9999999999;
		for(int i = 0; i < 4; i++)
		{
			float ri = colourts[i].rgb.red / 32.;
			float gi = colourts[i].rgb.green / 64.;
			float bi = colourts[i].rgb.blue / 32.;
			float distance = (ri - rcol) * (ri - rcol) + (gi - gcol) * (gi - gcol) + (bi - bcol) * (bi - bcol);

			if(distance < mindistance)
				mindistance = distance;
		}

		chi2 += mindistance;
	}

	return chi2;
}

quint8 PapaFile::findClosestColour(QRgb pixelcolour, QRgb* palette)
{
	quint8 closestindex = 255;
	float distance = 99999999999999;
	for(quint8 i = 0; i < 4; i++)
	{
		float distance2 =
			(qRed(pixelcolour) - qRed(palette[i])) * (qRed(pixelcolour) - qRed(palette[i])) +
			(qGreen(pixelcolour) - qGreen(palette[i])) * (qGreen(pixelcolour) - qGreen(palette[i])) +
			(qBlue(pixelcolour) - qBlue(palette[i])) * (qBlue(pixelcolour) - qBlue(palette[i]));
			
		if(distance2 < distance)
		{
			distance = distance2;
			closestindex = i;
		}
	}

	return closestindex;
}


bool PapaFile::decodeA8R8G8B8(PapaFile::texture_t& texture)
{
	int offset = 0;

	for(int m = 0; m < texture.NumberMinimaps; m++)
	{
		int divider = pow(2, m);
		qint16 width = texture.Width / divider;
		qint16 height = texture.Height / divider;
		
// TODO: Do something with the sRGB bit
//		convertFromSRGB();

		QImage image = QImage(width, height, QImage::Format_ARGB32);
		for(int j = 0; j < 4 * width * height; j += 4)
		{
			int pixelindex = j / 4;
			image.setPixel(pixelindex % width, (pixelindex / width), qRgba(texture.Data[offset + j], texture.Data[offset + j+1], texture.Data[offset + j+2], texture.Data[offset + j+3]));
		}
		texture.Image.push_back(image);
		offset += 4 * width * height;
	}

	return true;
}

bool PapaFile::encodeA8R8G8B8(PapaFile::texture_t& texture)
{
	int offset = 0;

	for(int m = 0; m < texture.NumberMinimaps; m++)
	{
		int divider = pow(2, m);
		qint16 width = texture.Width / divider;
		qint16 height = texture.Height / divider;
		
// TODO: Do something with the sRGB bit
//		convertFromSRGB();

		QImage image = texture.Image[m];
		for(int j = 0; j < 4 * width * height; j += 4)
		{
			int pixelindex = j / 4;
			QRgb colour = image.pixel(pixelindex % width, pixelindex / width);
			texture.Data[offset + j] = qRed(colour);
			texture.Data[offset + j+1] = qGreen(colour);
			texture.Data[offset + j+2] = qBlue(colour);
			texture.Data[offset + j+3] = qAlpha(colour);
		}
		offset += 4 * width * height;
	}

	return true;
}


bool PapaFile::decodeX8R8G8B8(PapaFile::texture_t& texture)
{
	// Same as A8R8G8B8, but alpha remains unused.

	int offset = 0;

	for(int m = 0; m < texture.NumberMinimaps; m++)
	{
		int divider = pow(2, m);
		qint16 width = texture.Width / divider;
		qint16 height = texture.Height / divider;

		QImage image = QImage(width, height, QImage::Format_RGB32);
		for(int j = 0; j < 4 * width * height; j += 4)
		{
			int pixelindex = j / 4;
			image.setPixel(pixelindex % width, (pixelindex / width), qRgb(texture.Data[offset + j], texture.Data[offset + j+1], texture.Data[offset + j+2]));
		}
		texture.Image.push_back(image);
		offset += 4 * width * height;
	}

	return true;
}

bool PapaFile::encodeX8R8G8B8(PapaFile::texture_t& texture)
{
	int offset = 0;

	for(int m = 0; m < texture.NumberMinimaps; m++)
	{
		int divider = pow(2, m);
		qint16 width = texture.Width / divider;
		qint16 height = texture.Height / divider;

		QImage image = texture.Image[m];
		for(int j = 0; j < 4 * width * height; j += 4)
		{
			int pixelindex = j / 4;
			QRgb colour = image.pixel(pixelindex % width, pixelindex / width);
			texture.Data[offset + j] = qRed(colour);
			texture.Data[offset + j+1] = qGreen(colour);
			texture.Data[offset + j+2] = qBlue(colour);
//			texture.Data[offset + j+3] = <NOT USED>;
		}
		offset += 4 * width * height;
	}

	return true;
}


bool PapaFile::decodeDXT1(PapaFile::texture_t& texture)
{
	const struct DXT1 *ptr = (const struct DXT1 *)(texture.Data.data());

	for(int m = 0; m < texture.NumberMinimaps; ++m)
	{
		int divider = pow(2, m);
		qint16 width = texture.Width / divider;
		qint16 height = texture.Height / divider;

		QImage image = QImage(width, height, QImage::Format_RGB32);
		image.fill(0);

		for(int j = 0; j < (width * height + 15)/ 16; j++) // The +15 is to make sure it works for 2x2 and 1x1 minimaps
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
/*
			if(texture.sRGB)
				convertFromSRGB(palette, 4);
*/
			int x0 = j % ((width+3)/4);
			int y0 = j / ((height+3)/4);

			quint32 rgbbits = ptr->rgbbits;
			for(int y = 0; y < std::min(4, (int)height); ++y)
			{
				for(int x = 0; x < std::min(4, (int)width); ++x)
				{
					quint8 colourindex = rgbbits & 0b11;
					image.setPixel(4*x0 + x, 4*y0 + y, qRgb(qRed(palette[colourindex]), qGreen(palette[colourindex]), qBlue(palette[colourindex])));
					rgbbits >>= 2;
				}
			}

			ptr++;
		}
		texture.Image.push_back(image);
	}

	return true;
}

bool PapaFile::encodeDXT1(PapaFile::texture_t& texture)
{
	char *originalpointer = texture.Data.data();
	struct DXT1 *ptr = (struct DXT1 *)(originalpointer);
//	struct DXT1 *ptr = (struct DXT1 *)(texture.Data.data());

	for(int m = 0; m < texture.NumberMinimaps; ++m)
	{
		QImage image(texture.Image[m].convertToFormat(QImage::Format_RGB16)); // Provides dithering. Nice!
		qint16 width = image.width();
		qint16 height = image.height();

		for(int j = 0; j < (width * height + 15)/ 16; j++) // The +15 is to make sure it works for 2x2 and 1x1 minimaps
		{
			QList<colour_t> colours;

			int x0 = j % ((width+3)/4);
			int y0 = j / ((height+3)/4);

			// TODO: This can be sped up by directly accessing image.bits() and casting to colour_t.
			for(int y = 0; y < std::min(4, (int)height); ++y)
			{
				for(int x = 0; x < std::min(4, (int)width); ++x)
				{
					QRgb qcolour = image.pixel(4*x0 + x, 4*y0 + y);
					colour_t colour;
					colour.rgb.red = 32 * qRed(qcolour) / 256;
					colour.rgb.green = 64 * qGreen(qcolour) / 256;
					colour.rgb.blue = 32 * qBlue(qcolour) / 256;
					bool havealready = false;
					for(int i = 0; i < colours.count(); i++)
					{
						if(colours[i].value == colour.value)
						{
							havealready = true;
							break;
						}
					}
					if(!havealready)
						colours.push_back(colour);
				}
			}

			colour_t colour0, colour1;
			switch(colours.count())
			{
				case 0:
					return false;
				case 1:
					colour0 = colours[0];
					break;
				case 2:
					if(colours[0].value > colours[1].value)
					{
						colour0.value = colours[0].value;
						colour1.value = colours[1].value;
					}
					else
					{
						colour0.value = colours[1].value;
						colour1.value = colours[0].value;
					}
					break;
				default:
					findOptimalColours(colour0, colour1, colours);
			}

			colour_t colour2, colour3;
			QRgb palette[4];
			if(colour0.value > colour1.value)
			{	// Four colours
				
				// Interpolate the other two colours.
				colour2.rgb.red = (2 * colour0.rgb.red + colour1.rgb.red) / 3;
				colour2.rgb.green = (2 * colour0.rgb.green + colour1.rgb.green) / 3;
				colour2.rgb.blue = (2 * colour0.rgb.blue + colour1.rgb.blue) / 3;
				colour3.rgb.red = (colour0.rgb.red + 2 * colour1.rgb.red) / 3;
				colour3.rgb.green = (colour0.rgb.green + 2 * colour1.rgb.green) / 3;
				colour3.rgb.blue = (colour0.rgb.blue + 2 * colour1.rgb.blue) / 3;

				palette[0] = qRgb(255*colour0.rgb.red/32, 255*colour0.rgb.green/64, 255*colour0.rgb.blue/32);
				palette[1] = qRgb(255*colour1.rgb.red/32, 255*colour1.rgb.green/64, 255*colour1.rgb.blue/32);
				palette[2] = qRgb(255*colour2.rgb.red/32, 255*colour2.rgb.green/64, 255*colour2.rgb.blue/32);
				palette[3] = qRgb(255*colour3.rgb.red/32, 255*colour3.rgb.green/64, 255*colour3.rgb.blue/32);
			}
			else
			{	// Three colours with black
				
				// The remaining colour is the average of the other two colours.
				colour2.rgb.red = (colour0.rgb.red + colour1.rgb.red) / 2;  
				colour2.rgb.green = (colour0.rgb.green + colour1.rgb.green) / 2;  
				colour2.rgb.blue = (colour0.rgb.blue + colour1.rgb.blue) / 2;  
				colour3.value = 0;

				palette[0] = qRgb(255*colour0.rgb.red/32, 255*colour0.rgb.green/64, 255*colour0.rgb.blue/32);
				palette[1] = qRgb(255*colour1.rgb.red/32, 255*colour1.rgb.green/64, 255*colour1.rgb.blue/32);
				palette[2] = qRgb(255*colour2.rgb.red/32, 255*colour2.rgb.green/64, 255*colour2.rgb.blue/32);
				palette[3] = qRgb(0, 0, 0);
			}
/*
			if(texture.sRGB)
				convertToSRGB(palette, 4);
*/
			quint32 rgbbits = 0;;
			for(int y = std::min(4, (int)height) - 1; y >= 0; --y)
			{
				for(int x = std::min(4, (int)width) - 1; x >= 0; --x)
				{
					rgbbits <<= 2;
					QRgb pixelcolour = image.pixel(4*x0 + x, 4*y0 + y);
					quint8 colourindex = findClosestColour(pixelcolour, palette);
					Q_ASSERT(colourindex < 4);
					rgbbits += colourindex;
				}
			}

			ptr->colour0.value = colour0.value;
			ptr->colour1.value = colour1.value;
			ptr->rgbbits = rgbbits;

			ptr++;
		}
	}

	return true;
}


bool PapaFile::decodeDXT5(PapaFile::texture_t& texture)
{
	const struct DXT5 *ptr = (const struct DXT5 *)(texture.Data.data());

	for(int i = 0; i < texture.NumberMinimaps; i++)
	{
		int divider = pow(2, i);
		qint16 width = texture.Width / divider;
		qint16 height = texture.Height / divider;

		QImage image = QImage(width, height, QImage::Format_ARGB32);
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

			// Get the alpha value for each pixel.
			quint8 alphatexel[4][4];
			quint64 alphabits = 0;
			quint8 fakebits[8] = {0,0,0,0,0,0,0,0};
			for(int k = 0; k < 6; k++)
			{
				fakebits[k] = ptr->alphabits[k];
			}
			alphabits = *((quint64 *)&fakebits);

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

			if(texture.sRGB)
				convertFromSRGB(palette, 4);

			int x0;
			if(texture.Width/(4*divider) > 0)
				x0 = j % (texture.Width/(4*divider));
			else
				x0 = j;

			int y0;
			if(texture.Height/(4*divider) > 0)
				y0 = j / (texture.Height/(4*divider));
			else
				y0 = j;

			quint32 rgbbits = ptr->rgbbits;
			for(int y = 0; y < 4; ++y)
			{
				for(int x = 0; x < 4; ++x)
				{
					quint8 colourindex = rgbbits & 0b11;
					image.setPixel(4*x0 + x, 4*y0 + y, qRgba(qRed(palette[colourindex]), qGreen(palette[colourindex]), qBlue(palette[colourindex]), alphatexel[x][y]));
					rgbbits >>= 2;
				}
			}

			ptr++;
		}
		texture.Image.push_back(image);
	}

	return true;
}

bool PapaFile::encodeDXT5(PapaFile::texture_t& /*texture*/)
{
	return false;
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
			default:
				return "Unsupported";
		}
	}

	return "None";
}

void PapaFile::convertFromSRGB(QRgb* palette, int size)
{
/*
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
	*/
}

void PapaFile::convertToSRGB(QRgb* palette, int size)
{/*
	// From https://en.wikipedia.org/w/index.php?title=SRGB&oldid=586514424#The_forward_transformation_.28CIE_xyY_or_CIE_XYZ_to_sRGB.29
	for(int i = 0; i < size; i++)
	{
		float red = qRed(palette[i]) / 255.;
		float green = qGreen(palette[i]) / 255.;
		float blue = qBlue(palette[i]) / 255.;
		
		float newred = 

		palette[i] = qRgb(
			255*(0.4124*red + 0.3576*green + 0.1805*blue),
			255*(0.2126*red + 0.7152*green + 0.0722*blue),
			255*(0.0193*red + 0.1192*green + 0.9502*blue)
		);
	}*/
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

bool PapaFile::importImage(const QImage &newimage, const int textureindex)
{
	if(textureindex < Textures.count())
	{
		if(Textures[textureindex].Image.count() == 0)
			return false;

		if(newimage.size() != Textures[textureindex].Image[0].size())
			return false;

		Textures[textureindex].Image.clear();
		Textures[textureindex].Image.push_back(newimage);
		for(int m = 1; m < Textures[textureindex].NumberMinimaps; m++)
		{
			int divider = pow(2, m);
			qint16 width = Textures[textureindex].Width / divider;
			qint16 height = Textures[textureindex].Height / divider;

			Textures[textureindex].Image.push_back(newimage.scaled(width, height));
		}
	}
	else
		return false;

	Modified = true;
	return true;
}

#include "papafile.moc"
