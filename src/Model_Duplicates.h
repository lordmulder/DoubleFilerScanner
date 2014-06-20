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

class DuplicateItem;
class QFile;

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
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	
	//Export formats
	typedef enum
	{
		FORMAT_INI = 0,
		FORMAT_XML = 1
	}
	exportFormat_t;

	unsigned int duplicateCount(void) const;
	unsigned int duplicateFileCount(const QModelIndex &index) const;
	const QString &getFilePath(const QModelIndex &index) const;
	const qint64 &getFileSize(const QModelIndex &index) const;
	QString toString(void);
	
	void clear(void);
	bool renameFile(const QModelIndex &index, const QString &newFileName);
	bool deleteFile(const QModelIndex &index);

	bool exportToFile(const QString &outFile, const int &format);

public slots:
	void addDuplicate(const QByteArray &hash, const QStringList &files, const qint64 &size);

protected:
	DuplicateItem *m_root;

	QIcon *m_iconDflt;
	QIcon *m_iconDupl;
	QFont *m_fontDflt;
	QFont *m_fontBold;

	bool exportToIni(const QString &outFile);
	bool exportToXml(const QString &outFile);
};
