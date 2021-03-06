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

#ifndef PAPAFILE_H
#define PAPAFILE_H

#include <QObject>
#include <QImage>

class PapaFile : public QObject
{
    Q_OBJECT

public:
	PapaFile();
	PapaFile(const QString &filename);
//	PapaFile(const PapaFile& other);
	~PapaFile();
//	PapaFile& operator=(const PapaFile& other);
	bool load(QString filename);
	bool save(QString filename = "");
	bool isValid() {return Valid;}
	QString lastError() {return LastError;}
	QByteArray texture() {return Textures[0].Data;}
	int textureCount() {return Textures.count(); }
	const QImage *image(int textureindex, int mipindex = 0);
	QString format();
	QSize size(int textureindex) {if(textureindex < Textures.count()) return QSize(Textures[textureindex].Width, Textures[textureindex].Height); else return QSize();}
	QString name() {return Bones[0].name;}
	bool importImage(const QImage& newimage, const int textureindex);
	bool isModified() {return Modified;}
	bool canEncode() {return Textures.count() > 0 ? (Textures[0].Format == texture_t::A8R8G8B8 || Textures[0].Format == texture_t::X8R8G8B8 || Textures[0].Format == texture_t::DXT1) : false;}

private:
	// File format
	struct Header
	{
		char Identification[4];
		qint16 Unknown1[2];
		qint16 NumberOfBones;
		qint16 NumberOfTextures;
		qint16 NumberOfVertexBuffers;
		qint16 NumberOfIndexBuffers;
		qint16 NumberOfMaterials;
		qint16 NumberOfMeshes;
		qint16 NumberOfSkeletons;
		qint16 NumberOfModels;
		qint16 Unknown2[4];
		qint64 OffsetBonesHeader;
		qint64 OffsetTextureInformation;
		qint64 OffsetVerticesInformation;
		qint64 OffsetIndicesInformation;
		qint64 OffsetMaterialInformation;
		qint64 OffsetMeshInformation;
		qint64 OffsetSkeletonInformation;
		qint64 OffsetModelInformation;
		qint64 OffsetAnimationInformation;
	};
	struct BonesHeader
	{
		qint64 LengthOfBoneName;
		qint64 OffsetBoneName;
	};
	struct TextureInformationHeader
	{
		char Unknown1[2];
		char TextureFormat;
		uchar NumberMinimaps:4; //(TF_DXT1 only)
		uchar Unknown2:3;
		uchar SRGB:1 ;
		qint16 Width;
		qint16 Height;
		qint64 Length;
		qint64 Unknown3; //Always 128
	};


	// Internal format
	struct bone_t
	{
		QString name;
	};
	
	struct texture_t
	{
		enum
		{ // This comes from inside the papadump binary at position 0x0055E0DE
			Invalid = 0,
			A8R8G8B8,
			X8R8G8B8,
			A8B8G8R8,
			DXT1,
			DXT3,
			DXT5,
			R32F,
			RG32F,
			RGBA32F,
			R16F,
			RG16F,
			RGBA16F,
			R8G8,
			D0,
			D16,
			D24,
			D24S8,
			D32,
			R8I,
			R8UI,
			R16I,
			R16UI,
			RG8I,
			RG8UI,
			RG16I,
			RG16UI,
			R32I,
			R32UI,
			Shadow16,
			Shadow24,
			Shadow32
		} Format;
		qint16 Width, Height;
		int NumberMinimaps;
		bool sRGB;
		QByteArray Data;
		QList<QImage> Image;
		struct
		{
			char Unknown1[2];
			uchar Unknown2;
			qint64 Unknown3;
		} Unknowns;
	};

	union colour_t
	{
		quint16 value;
		struct
		{
			quint16 blue : 5;
			quint16 green : 6;
			quint16 red : 5;
		} rgb;
	};

	struct DXT1
	{
		colour_t colour0;
		colour_t colour1;
		quint32 rgbbits;
	};

	struct DXT5
	{
		quint8 alpha0;
		quint8 alpha1;
		quint8 alphabits[6];
		colour_t colour0;
		colour_t colour1;
		quint32 rgbbits;
	};

	void init();
	bool decodeA8R8G8B8(PapaFile::texture_t& texture);
	bool decodeX8R8G8B8(PapaFile::texture_t& texture);
	bool decodeDXT1(PapaFile::texture_t& texture);
	bool decodeDXT5(PapaFile::texture_t& texture);
	bool encodeA8R8G8B8(PapaFile::texture_t& texture);
	bool encodeX8R8G8B8(PapaFile::texture_t& texture);
	bool encodeDXT1(PapaFile::texture_t& texture);
	bool encodeDXT5(PapaFile::texture_t& texture);
	void convertFromSRGB(QRgb* palette, int size);
    void convertToSRGB(QRgb* palette, int size);
    void findOptimalColours(PapaFile::colour_t& colour0, PapaFile::colour_t& colour1, const QList<PapaFile::colour_t>& colours);
    float calculateChi2_four(PapaFile::colour_t col1, PapaFile::colour_t col2, const QList< PapaFile::colour_t >& colours);
    quint8 findClosestColour(QRgb pixelcolour, QRgb* palette);

	bool Valid;
	bool Modified;
	QString LastError;
	QList<bone_t> Bones;
	QList<texture_t> Textures;
	QString Filename;
	struct
	{
		qint16 Unknown1[2];
		qint16 Unknown2[4];
	} HeaderUnknowns;
};

#endif // PAPAFILE_H
