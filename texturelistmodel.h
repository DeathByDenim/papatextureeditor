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

#ifndef TEXTURELISTMODEL_H
#define TEXTURELISTMODEL_H

#include <QAbstractItemModel>
#include "papafile.h"

class TextureListModel : public QAbstractListModel
{
	Q_OBJECT

public:
	TextureListModel(QObject *parent = 0);
	~TextureListModel();
	virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	bool importImage(const QString& name, const QModelIndex& index);
	bool loadFromDirectory(const QString& foldername);
	PapaFile *papa(const QModelIndex& index);
	QString info(const QModelIndex& index);
	bool savePapa(const QModelIndex& index, const QString& filename = "");
	QString lastError() {return LastError;}
	bool isEditable(const QModelIndex& index);

private:
	QList<PapaFile *> Papas;
	QString LastError;
};

#endif // TEXTURELISTMODEL_H
