///////////////////////////////////////////////////////////////////////////////
// Double File Scanner
// Copyright (C) 2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QAbstractItemModel>
#include <QStringList>
#include <QReadwriteLock>

class DuplicateItem;

//DuplicatesModel class
class DuplicatesModel: public QAbstractItemModel
{
	Q_OBJECT

public:
	DuplicatesModel(void);
	virtual ~DuplicatesModel(void);
	
	//QAbstractItemModel
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	
	unsigned int duplicateCount(void) const;
	const QString &getFilePath(const QModelIndex &index) const;
	void addDuplicate(const QByteArray &hash, const QStringList files);
	void clear(void);

protected:
	DuplicateItem *m_root;

	QIcon *m_dupIcon;
	QIcon *m_bulIcon;
	QFont *m_fontDflt;
	QFont *m_fontBold;

	mutable QReadWriteLock m_lock;
};
