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
#include <QBrush>

TextureListModel::TextureListModel(QObject* parent)
 : QAbstractListModel(parent), LastError("")
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
			return QVariant(Papas.at(index.row())->name());
		case Qt::ForegroundRole:
			if(Papas.at(index.row())->isModified())
				return QBrush(Qt::red);
			else
				return QVariant();
		default:
			return QVariant();
	}
}

int TextureListModel::rowCount(const QModelIndex& parent) const
{
	return Papas.count();
}

QVariant TextureListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole)
		return QVariant("Texture");
	else
		return QAbstractListModel::headerData(section, orientation, role);
}

bool TextureListModel::loadFromDirectory(const QString& foldername)
{
	QDir folder(foldername);
	if(!folder.exists())
		return false;

	for(QList<PapaFile *>::iterator papa = Papas.begin(); papa != Papas.end(); ++papa)
		delete (*papa);
	Papas.clear();

	beginResetModel();
	QStringList papafiles = folder.entryList(QStringList("*.papa"), QDir::Files | QDir::Readable, QDir::Name);
	for(QStringList::const_iterator papafile = papafiles.constBegin(); papafile != papafiles.constEnd(); ++papafile)
	{
		qDebug() << *papafile;
		PapaFile *papa = new PapaFile(foldername + '/' + *papafile);
		if(papa->isValid() && papa->textureCount() == 1)
			Papas.push_back(papa);
		else
			delete papa;
	}
	endResetModel();

	return true;
}


PapaFile *TextureListModel::papa(const QModelIndex& index)
{
	if(index.row() < Papas.count())
	{
		return Papas[index.row()];
	}
	else
		return NULL;
}

QString TextureListModel::info(const QModelIndex& index)
{
	if(index.row() < Papas.count())
	{
		QString info;
		PapaFile *papa = Papas[index.row()];
		const QImage *im = papa->image(0);
		if(im)
			info = QString("Size: %1 x %2, Format: %3").arg(im->width()).arg(im->height()).arg(papa->format());
		else
			info = QString("Size: ?????, Format: %3").arg(papa->format());

		return info;
	}
	else
		return "";
}

bool TextureListModel::isEditable(const QModelIndex& index)
{
	if(index.row() < Papas.count())
	{
		PapaFile *papa = Papas[index.row()];
		if(papa)
		{
			return papa->canEncode();
		}
	}
	
	return false;
}

bool TextureListModel::importImage(const QString& name, const QModelIndex& index)
{
	if(!index.isValid() || !Papas[index.row()]->image(0))
		return false;

	QImageReader *reader = new QImageReader(name);
	QImage newimage = reader->read();
	delete reader;

	if(Papas[index.row()]->size(0) == newimage.size() && Papas[index.row()]->canEncode())
	{
		qDebug() << "newimage.pixel(0, 0):" << qRed(newimage.pixel(0, 0)) << ", " << qGreen(newimage.pixel(0, 0)) << ", " << qBlue(newimage.pixel(0, 0));

		return Papas[index.row()]->importImage(newimage, 0);
	}
	else
		return false;
}
/*
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
*/

bool TextureListModel::savePapa(const QModelIndex& index, const QString& filename)
{
	if(!index.isValid())
		return false;

	if(index.row() < Papas.count())
	{
		if(!Papas[index.row()]->save(filename))
		{
			LastError = Papas[index.row()]->lastError();
			return false;
		}
	}
	else
		return false;

	LastError = "";
	return true;
}


#include "texturelistmodel.moc"
