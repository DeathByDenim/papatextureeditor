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

#include "texturelistmodel.h"
#include "papafileheader.h"
#include <QFile>

#include <QDebug>
#include <QDir>
#include <QImageReader>

TextureListModel::TextureListModel(QObject* parent)
 : QAbstractListModel(parent)
{
}

TextureListModel::~TextureListModel()
{
}

QVariant TextureListModel::data(const QModelIndex& index, int role) const
{
	switch(role)
	{
		case Qt::DisplayRole:
			return QVariant(Images.at(index.row()).boneName);
		default:
			return QVariant();
	}
}

int TextureListModel::rowCount(const QModelIndex& parent) const
{
	return Images.count();
}

QVariant TextureListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole)
		return QVariant("Texture");
	else
		return QAbstractListModel::headerData(section, orientation, role);
}

bool TextureListModel::loadFromFile(const QString& name)
{
	QFile papafile(name);
	if(!papafile.open(QIODevice::ReadOnly))
		return false;

	PapaFileHeader papaheader;
	papafile.read((char *)&papaheader, sizeof(PapaFileHeader));
	
	if(
		papaheader.NumberOfBones != 1 ||
		papaheader.NumberOfTextures != 1 ||
		papaheader.NumberOfVertexBuffers != 0 ||
		papaheader.NumberOfIndexBuffers != 0 ||
		papaheader.NumberOfMaterials != 0 ||
		papaheader.NumberOfMeshes != 0 ||
		papaheader.NumberOfSkeletons != 0 ||
		papaheader.NumberOfModels != 0
	)
	{
		return false;
	}


	beginResetModel();
	papafile.seek(papaheader.OffsetBonesHeader);
	for(qint64 i = 0; i < papaheader.NumberOfBones; i++)
	{
		PapaBoneHeader boneheader;
		papafile.read((char *)&boneheader, sizeof(PapaBoneHeader));

		qint64 oldpos = papafile.pos();
		papafile.seek(boneheader.OffsetBoneName);
		char *bonename = new char[boneheader.LengthOfBoneName];
		papafile.read(bonename, boneheader.LengthOfBoneName);
		papafile.seek(oldpos);

		ImageInFile_t newimage;
		newimage.boneName = QByteArray(bonename, boneheader.LengthOfBoneName);
		Images.push_back(newimage);
	}

	papafile.seek(papaheader.OffsetTextureInformation);
	for(qint64 i = 0; i < papaheader.NumberOfTextures; i++)
	{
		PapaTextureInformationHeader imageheader;
		papafile.read((char *)&imageheader, sizeof(PapaTextureInformationHeader));

		Images[Images.length()-1].papaFileName = name;
		Images[Images.length()-1].textureOffset = papafile.pos();
		Images[Images.length()-1].textureFormat = imageheader.TextureFormat;
		QByteArray imagedata = papafile.read(imageheader.Length);

		QImage image(imageheader.Width, imageheader.Height, QImage::Format_ARGB32);
		image.fill(qRgb(255, 0, 0));
		switch(imageheader.TextureFormat)
		{
			case TF_A8R8G8B8:
				decodeRGBAImage(image, imageheader.Width, imageheader.Height, imagedata);
				break;

			case TF_DXT1:
				decodeDXT1Image(image, imageheader.Width, imageheader.Height, imagedata);
				break;

			case TF_DXT5:
				decodeDXT5Image(image, imageheader.Width, imageheader.Height, imagedata);
				break;

			default:
				return false;
		}
		Images[Images.length()-1].format = imageheader.TextureFormat;
		Images[Images.length()-1].image = image;
	}

	endResetModel();
	
	return true;
}

bool TextureListModel::loadFromDirectory(const QString& foldername)
{
	QDir folder(foldername);
	if(!folder.exists())
		return false;

	Images.clear();

	QStringList papafiles = folder.entryList(QStringList("*.papa"), QDir::Files | QDir::Readable, QDir::Name);
	for(QStringList::const_iterator papafile = papafiles.constBegin(); papafile != papafiles.constEnd(); ++papafile)
	{
		qDebug() << *papafile;
		loadFromFile(foldername + '/' + *papafile);
	}
	
	return true;
}


const QImage& TextureListModel::image(const QModelIndex& index)
{
	if(index.row() < Images.count())
	{
		return Images[index.row()].image;
	}
	else
		return QImage();
}

QString TextureListModel::info(const QModelIndex& index)
{
	if(index.row() < Images.count())
	{
		QString info;
		ImageInFile_t imageinfo = Images[index.row()];
		info = QString("Size: %1 x %2, Format: ").arg(imageinfo.image.width()).arg(imageinfo.image.height());
		
		switch(imageinfo.format)
		{
			case TF_A8R8G8B8:
				info += "TF_A8R8G8B8";
				break;
			case TF_DXT1:
				info += "TF_DXT1";
				break;
			case TF_DXT5:
				info += "TF_DXT5";
				break;
			default:
				info += "Unknown";
				break;
		}
		
		return info;
	}
	else
		return "";
}

void TextureListModel::decodeRGBAImage(QImage &image, qint64 width, qint64 height, QByteArray &imagedata)
{
	for(int j = 0; j < 4 * width * height; j += 4)
	{
		int pixelindex = j / 4;
		image.setPixel(pixelindex % width, height - 1 - (pixelindex / width), qRgba(imagedata[j], imagedata[j+1], imagedata[j+2], imagedata[j+3]));
	}
}

void TextureListModel::decodeDXT1Image(QImage &image, qint64 width, qint64 height, QByteArray &imagedata)
{
	union colour_t
	{
		quint16 value;
		struct
		{
			quint16 red : 5;
			quint16 green : 6;
			quint16 blue : 5;
		} colour4;
		struct
		{
			quint16 red : 5;
			quint16 green : 5;
			quint16 blue : 5;
			quint16 alpha : 1;
		} colour3;
	};
	
	struct row_t
	{
		uchar col0 : 2;
		uchar col1 : 2;
		uchar col2 : 2;
		uchar col3 : 2;
	};

	struct DXT1
	{
		colour_t colour0;
		colour_t colour1;
		row_t row[4];
	} *ptr;

	qDebug() << "sizeof(row_t) = " << sizeof(struct row_t);

	ptr = (struct DXT1 *)imagedata.data();
	for(int j = 0; j < ((width + 3) / 4) * ((height + 3) / 4); j++)
	{
		colour_t colour2, colour3;
		QRgb palette[4];
		if(ptr->colour0.value > ptr->colour1.value)
		{	// Four colours
			colour2.value = (2 * ptr->colour0.value + ptr->colour1.value) / 3;
			colour3.value = (ptr->colour0.value + 2 * ptr->colour1.value) / 3;

			palette[0] = qRgb(255*ptr->colour0.colour3.red/32, 255*ptr->colour0.colour3.green/64, 255*ptr->colour0.colour3.blue/32);
			palette[1] = qRgb(255*ptr->colour1.colour3.red/32, 255*ptr->colour1.colour3.green/64, 255*ptr->colour1.colour3.blue/32);
			palette[2] = qRgb(255*colour2.colour3.red/32, 255*colour2.colour3.green/64, 255*colour2.colour3.blue/32);
			palette[3] = qRgb(255*colour3.colour3.red/32, 255*colour3.colour3.green/64, 255*colour3.colour3.blue/32);
		}
		else
		{	// Three colours with alpha
			colour2.value = (ptr->colour0.value + ptr->colour1.value) / 2;  
			colour3.value = 0;
			
			palette[0] = qRgba(255*ptr->colour0.colour3.red/32, 255*ptr->colour0.colour3.green/32, 255*ptr->colour0.colour3.blue/32, 255*ptr->colour0.colour3.alpha);
			palette[1] = qRgba(255*ptr->colour1.colour3.red/32, 255*ptr->colour1.colour3.green/32, 255*ptr->colour1.colour3.blue/32, 255*ptr->colour1.colour3.alpha);
			palette[2] = qRgba(255*colour2.colour3.red/32, 255*colour2.colour3.green/32, 255*colour2.colour3.blue/32, 255*colour2.colour3.alpha);
			palette[3] = qRgba(255*colour3.colour3.red/32, 255*colour3.colour3.green/32, 255*colour3.colour3.blue/32, 255*colour3.colour3.alpha);
		}

		int x = j % ((height + 3)/4);
		int y = j / ((height + 3)/4);
		for(int k = 0; k < 4; k++)
		{
			image.setPixel(4*x + k, 4*y, palette[ptr->row[k].col0]);
			image.setPixel(4*x + k, 4*y + 1, palette[ptr->row[k].col1]);
			image.setPixel(4*x + k, 4*y + 2, palette[ptr->row[k].col2]);
			image.setPixel(4*x + k, 4*y + 3, palette[ptr->row[k].col3]);
		}

		ptr++;
	}
}

void TextureListModel::decodeDXT5Image(QImage &image, short int Width, short int Height, QByteArray &imagedata)
{

}

bool TextureListModel::importImage(const QString& name, const QModelIndex& index)
{
	if(!index.isValid())
		return false;

	QImageReader *reader = new QImageReader(name);
	QImage newimage = reader->read();
	delete reader;

	if(Images[index.row()].image.size() == newimage.size() && Images[index.row()].textureFormat == TF_A8R8G8B8)
	{
		Images[index.row()].image = newimage;
		return saveImage(index);
	}
	else
		return false;
}

bool TextureListModel::saveImage(const QModelIndex &index)
{
	if(!index.isValid())
		return false;

	QFile papafile(Images[index.row()].papaFileName);
	if(!papafile.open(QIODevice::ReadWrite))
		return false;

	papafile.seek(Images[index.row()].textureOffset);

	QImage image = Images[index.row()].image;
	QByteArray data(4*image.width()*image.height(), Qt::Uninitialized);
	for(int i = 0; i < 4 * image.width()*image.height(); i += 4)
	{
		int pixelindex = i / 4;
		QRgb colour = image.pixel(pixelindex % 1024, image.height() - 1 - (pixelindex / 1024));
		data[i] = (unsigned char)qRed(colour);
		data[i+1] = (unsigned char)qGreen(colour);
		data[i+2] = (unsigned char)qBlue(colour);
		data[i+3] = (unsigned char)qAlpha(colour);
	}
	papafile.write(data);
}


#include "texturelistmodel.moc"
