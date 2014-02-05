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
    PapaFile(const PapaFile& other);
    ~PapaFile();
    PapaFile& operator=(const PapaFile& other);
    bool load(QString filename);
    bool isValid() {return Valid;}
    QString lastError() {return LastError;}
    QByteArray texture() {return Textures[0].Data;}
    QImage image() {return Textures[0].Image[0];}
    QString format();
	QString name() {return Bones[0].name;}

	qint16 NumberOfBones;
	qint16 NumberOfTextures;
	qint16 NumberOfVertexBuffers;
	qint16 NumberOfIndexBuffers;
	qint16 NumberOfMaterials;
	qint16 NumberOfMeshes;
	qint16 NumberOfSkeletons;
	qint16 NumberOfModels;

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
		QByteArray Data;
		QList<QImage> Image;
	};

	void init();
	bool decodeDXT1(PapaFile::texture_t& texture);

	bool Valid;
	QString LastError;
	QList<bone_t> Bones;
	QList<texture_t> Textures;
};

#endif // PAPAFILE_H
