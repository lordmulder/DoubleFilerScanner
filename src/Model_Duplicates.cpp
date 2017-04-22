///////////////////////////////////////////////////////////////////////////////
// Double File Scanner
// Copyright (C) 2014-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Model_Duplicates.h"

#include <QFont>
#include <QIcon>
#include <QDir>
#include <QSettings>
#include <QXmlStreamWriter>

#include "Config.h"
#include "Utilities.h"
#include "System.h"

#include <cassert>

static const QString EMPTY_STRING;
static const QByteArray EMPTY_BYTEARRAY;
static const qint64 ZERO_SIZE = 0;

static inline QString LIMIT_STR(const QString &str, const int maxLen)
{
	const int len = str.length();
	if(len > maxLen)
	{
		const int suffix = str.lastIndexOf(L'.');
		return (suffix > 0) ? (str.left(maxLen - (len - suffix)) + QChar(ushort(0x2026)) + str.mid(suffix + 1)) : (str.left(maxLen - 1) + QChar(ushort(0x2026)));
	}
	return str;
}

static inline QString FORMAT_NUMBER(qint64 value)
{
	QString formated;
	while(value > 1000)
	{
		formated.prepend(QString().sprintf(",%03d", int(value % 1000i64)));
		value /= 1000i64;
	}
	formated.prepend(QString().sprintf("%d", int(value % 1000i64)));
	return formated;
}

//===================================================================
// Duplicates Items
//===================================================================

enum
{
	ITEM_ROOT  = 0,
	ITEM_GROUP = 1,
	ITEM_FILE  = 2
}
ItemType_t;

class DuplicateItem
{
public:
	DuplicateItem(DuplicateItem *const parent = NULL)
	:
		m_parent(parent)
	{
		if(parent)
		{
			parent->addChild(this);
		}
	}

	virtual ~DuplicateItem(void)
	{
		removeAllChilderen();
	}

	inline DuplicateItem *child(const int index) const { return m_childeren[index]; }
	inline int childCount(void) const { return m_childeren.count(); }
	inline void addChild(DuplicateItem *child) { m_childeren << child; }
	inline int row(void) { return m_parent ? m_parent->m_childeren.indexOf(this) : 0; }
	inline DuplicateItem *parent(void) const { return m_parent; }

	void removeAllChilderen(void)
	{
		while(!m_childeren.isEmpty())
		{
			DuplicateItem *child = m_childeren.takeLast();
			MY_DELETE(child);
		}
	}

	void removeChild(DuplicateItem *const child)
	{
		m_childeren.removeAll(child);
		delete child;
	}

	virtual int type(void)
	{
		return ITEM_ROOT;
	}

protected:
	DuplicateItem *const m_parent;
	QList<DuplicateItem*> m_childeren;

private:
	DuplicateItem &operator=(const DuplicateItem&)
	{
		throw std::runtime_error("Unimplemented");
	}
};

class DuplicateItem_Group : public DuplicateItem
{
public:
	DuplicateItem_Group(DuplicateItem *const parent, const QByteArray &hash)
	:
		DuplicateItem(parent),
		m_hash(hash)
	{
		/*nithing to do here*/
	}

	virtual int type(void)
	{
		return ITEM_GROUP;
	}

	inline const QByteArray &getHash(void) const { return m_hash; }

protected:
	const QByteArray m_hash;
};

class DuplicateItem_File : public DuplicateItem
{
public:
	DuplicateItem_File(DuplicateItem *const parent, const QString &filePath, const qint64 &fileSize)
	:
		DuplicateItem(parent),
		m_fileInfo(filePath),
		m_fileSize(fileSize)
	{
		/*nithing to do here*/
	}

	virtual int type(void)
	{
		return ITEM_FILE;
	}

	inline const QString getName(void) const         { return m_fileInfo.fileName();         }
	inline const QString getPath(void) const         { return m_fileInfo.absolutePath();     }
	inline const QString getFilePath(void) const     { return m_fileInfo.absoluteFilePath(); }
	inline void setFilePath(const QString &filePath) { m_fileInfo.setFile(filePath);         }
	inline const qint64 &getFileSize(void) const     { return m_fileSize;                    }

protected:
	QFileInfo m_fileInfo;
	const qint64 m_fileSize;
};

//===================================================================
// Constructor & Destructor
//===================================================================

DuplicatesModel::DuplicatesModel(void)
{
	m_iconDflt = new QIcon(":/res/Icon_Bullet.png");
	m_iconDupl = new QIcon(":/res/Icon_Duplicate.png");

	m_fontDflt = new QFont("Monospace");
	m_fontDflt->setStyleHint(QFont::TypeWriter);
	
	m_fontBold = new QFont("Monospace");
	m_fontBold->setStyleHint(QFont::TypeWriter);
	m_fontBold->setBold(true);

	m_root = new DuplicateItem();
}

DuplicatesModel::~DuplicatesModel(void)
{
	MY_DELETE(m_root);
	MY_DELETE(m_fontDflt);
	MY_DELETE(m_fontBold);
	MY_DELETE(m_iconDflt);
	MY_DELETE(m_iconDupl);
}

//===================================================================
// Model Functions
//===================================================================

QModelIndex DuplicatesModel::index(int row, int column, const QModelIndex &parent) const
{
	DuplicateItem *parentItem = m_root;
	if(parent.isValid() && parent.internalPointer())
	{
		parentItem = static_cast<DuplicateItem*>(parent.internalPointer());
	}

	if(parentItem && (row >= 0) && (row < parentItem->childCount()))
	{
		return createIndex(row, column, parentItem->child(row));
	}

	return QModelIndex();
}

QModelIndex DuplicatesModel::parent(const QModelIndex &index) const
{
	DuplicateItem *item = m_root;
	if(index.isValid() && index.internalPointer())
	{
		item = static_cast<DuplicateItem*>(index.internalPointer());
	}

	DuplicateItem *parentItem = item->parent();
	if(parentItem && (parentItem != m_root))
	{
		return createIndex(parentItem->row(), 0, parentItem);
	}

	return QModelIndex();
}

int DuplicatesModel::rowCount(const QModelIndex &parent) const
{
	DuplicateItem *item = m_root;
	if(parent.isValid() && parent.internalPointer())
	{
		item = static_cast<DuplicateItem*>(parent.internalPointer());
	}

	return item->childCount();
}

int DuplicatesModel::columnCount(const QModelIndex &parent) const
{
	return 3;
}

QVariant DuplicatesModel::data(const QModelIndex &index, int role) const
{
	DuplicateItem *item = m_root;
	if(index.isValid() && index.internalPointer())
	{
		item = static_cast<DuplicateItem*>(index.internalPointer());
	}

	switch(role)
	{
	/* ============= DISPLAY ROLE ============= */
	case Qt::DisplayRole:
		if(DuplicateItem_File *file = dynamic_cast<DuplicateItem_File*>(item))
		{
			switch(index.column())
			{
				case 0: return LIMIT_STR(file->getName(), 32);
				case 1: return QDir::toNativeSeparators(file->getPath());
				case 2: return Utilities::sizeToString(file->getFileSize());
			}
		}
		else if(DuplicateItem_Group *group = dynamic_cast<DuplicateItem_Group*>(item))
		{
			if(index.column() == 0)
			{
				return QString().sprintf("%.16s (%c%d)", group->getHash().toHex().constData(), ushort(0xd7), group->childCount());
			}
		}
		break;
	/* ============= TOOL-TIP ROLE ============= */
	case Qt::ToolTipRole:
		if(DuplicateItem_File *file = dynamic_cast<DuplicateItem_File*>(item))
		{
			return (index.column() < 2) ? QDir::toNativeSeparators(file->getFilePath()) : tr("%1 Bytes").arg(FORMAT_NUMBER(file->getFileSize()));
		}
		else if(DuplicateItem_Group *group = dynamic_cast<DuplicateItem_Group*>(item))
		{
			return QString().sprintf("SHA-1 Digest: %s", group->getHash().toHex().constData());
		}
		break;
	/* ============= DECORATION ROLE ============= */
	case Qt::DecorationRole:
		if(index.column() == 0)
		{
			return (item->type() == ITEM_GROUP) ? (*m_iconDupl) : (*m_iconDflt);
		}
		break;
	/* ============= TEXT-ALIGNMENT ROLE ============= */
	case Qt::TextAlignmentRole:
		if(index.column() == 2)
		{
			return QVariant(Qt::AlignRight | Qt::AlignVCenter);
		}
		break;
	/* ============= FONT ROLE ============= */
	case Qt::FontRole:
		return (item->type() == ITEM_GROUP) ? (*m_fontBold) : (*m_fontDflt);
	}

	return QVariant();
}

QVariant DuplicatesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal)
	{
		switch(role)
		{
		case Qt::DisplayRole:
			switch(section)
			{
			case 0:
				return tr("File Name");
			case 1:
				return tr("Location");
			case 2:
				return tr("Size");
			}
		}
	}

	return QVariant();
}

//===================================================================
// Query Functions
//===================================================================

unsigned int DuplicatesModel::duplicateCount(void) const
{
	return m_root->childCount();
}

unsigned int DuplicatesModel::duplicateFileCount(const QModelIndex &index) const
{
	if(index.isValid())
	{
		if(DuplicateItem *currentItem = static_cast<DuplicateItem*>(index.internalPointer()))
		{
			if(DuplicateItem_File *currentFile = dynamic_cast<DuplicateItem_File*>(currentItem))
			{
				if(DuplicateItem_Group *duplicateGroup = dynamic_cast<DuplicateItem_Group*>(currentFile->parent()))
				{
					return duplicateGroup->childCount();
				}
			}
		}
	}
	
	return 0;
}

const QString DuplicatesModel::getFilePath(const QModelIndex &index) const
{
	if(index.isValid())
	{
		if(DuplicateItem *currentItem = static_cast<DuplicateItem*>(index.internalPointer()))
		{
			if(DuplicateItem_File *currentFile = dynamic_cast<DuplicateItem_File*>(currentItem))
			{
				return currentFile->getFilePath();
			}
		}
	}
	
	return EMPTY_STRING;
}

const qint64 &DuplicatesModel::getFileSize(const QModelIndex &index) const
{
	if(index.isValid())
	{
		if(DuplicateItem *currentItem = static_cast<DuplicateItem*>(index.internalPointer()))
		{
			if(DuplicateItem_File *currentFile = dynamic_cast<DuplicateItem_File*>(currentItem))
			{
				return currentFile->getFileSize();
			}
		}
	}

	return ZERO_SIZE;
}

const QByteArray &DuplicatesModel::getGroupHash(const QModelIndex &index) const
{
	if(index.isValid())
	{
		if(DuplicateItem *currentItem = static_cast<DuplicateItem*>(index.internalPointer()))
		{
			if(DuplicateItem_Group *currentGroup = dynamic_cast<DuplicateItem_Group*>(currentItem))
			{
				return currentGroup->getHash();
			}
		}
	}

	return EMPTY_BYTEARRAY;
}

QString DuplicatesModel::toString(void)
{
	QStringList lines;
	const int hashCount = m_root->childCount();

	for(int i = 0; i < hashCount; i++)
	{
		if(DuplicateItem_Group *currentGroup = dynamic_cast<DuplicateItem_Group*>(m_root->child(i)))
		{
			lines << QString::fromLatin1(currentGroup->getHash().toHex().constData());
			const int fileCount = currentGroup->childCount();

			for(int j = 0; j < fileCount; j++)
			{
				DuplicateItem_File *currentFile = dynamic_cast<DuplicateItem_File*>(currentGroup->child(j));
				lines << QString("- %1").arg(QDir::toNativeSeparators(currentFile->getFilePath()));
			}

			lines << QString();
		}
	}

	return lines.join("\r\n");
}

//===================================================================
// Edit Functions
//===================================================================

void DuplicatesModel::clear(void)
{
	beginResetModel();
	m_root->removeAllChilderen();
	endResetModel();
}

void DuplicatesModel::addDuplicate(const QByteArray &hash, const QStringList &files, const qint64 &size)
{
	if(!files.isEmpty())
	{
		beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());
		DuplicateItem_Group *group = new DuplicateItem_Group(m_root, hash);
		for(QStringList::ConstIterator iterFile = files.constBegin(); iterFile != files.constEnd(); iterFile++)
		{
			new DuplicateItem_File(group, (*iterFile), size);
		}
		endInsertRows();
	}
}

bool DuplicatesModel::renameFile(const QModelIndex &index, const QString &newFileName)
{
	if(index.isValid())
	{
		if(DuplicateItem *currentItem = static_cast<DuplicateItem*>(index.internalPointer()))
		{
			if(DuplicateItem_File *currentFile = dynamic_cast<DuplicateItem_File*>(currentItem))
			{
				const QString oldFilePath = currentFile->getFilePath();
				if(QFileInfo(oldFilePath).exists() && QFileInfo(oldFilePath).isFile())
				{
					QString newFilePath = QString("%1/%2").arg(QFileInfo(oldFilePath).absolutePath(), newFileName);
					if(oldFilePath.compare(newFilePath, Qt::CaseInsensitive) == 0)
					{
						return true; /*no need to rename*/
					}
					if(QFileInfo(newFilePath).exists())
					{
						const QString suffix   = QFileInfo(newFilePath).suffix();
						const QString baseName = QFileInfo(newFilePath).completeBaseName();
						const QString path     = QFileInfo(newFilePath).absolutePath();
						for(int n = 2; QFileInfo(newFilePath).exists(); n++)
						{
							newFilePath = QString("%1/%2 (%3).%4").arg(path, baseName, QString::number(n), suffix);
							if(n > SHRT_MAX) return false;
						}
					}
					if(QFile::rename(oldFilePath, newFilePath))
					{
						currentFile->setFilePath(newFilePath);
						emit dataChanged(index, index);
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool DuplicatesModel::deleteFile(const QModelIndex &index)
{
	if(index.isValid())
	{
		if(DuplicateItem *currentItem = static_cast<DuplicateItem*>(index.internalPointer()))
		{
			if(DuplicateItem_File *currentFile = dynamic_cast<DuplicateItem_File*>(currentItem))
			{
				const QString oldFilePath = currentFile->getFilePath();
				bool okay = true;
				if(QFileInfo(oldFilePath).exists() && QFileInfo(oldFilePath).isFile())
				{
					okay = QFile::remove(currentFile->getFilePath());
					if(!okay)
					{
						QFile::setPermissions(currentFile->getFilePath(), QFile::ReadUser | QFile::WriteUser);
						okay = QFile::remove(currentFile->getFilePath());
					}
				}
				if(okay)
				{
					if(DuplicateItem *parentItem = currentFile->parent())
					{
						beginRemoveRows(parent(index), index.row(), index.row());
						parentItem->removeChild(currentFile);
						endRemoveRows();
					}
				}
				return okay;
			}
		}
	}

	return false;
}

//===================================================================
// Export Functions
//===================================================================

bool DuplicatesModel::exportToFile(const QString &outFile, const int &format)
{
	switch(format)
	{
	case FORMAT_INI:
		return exportToIni(outFile);
		break;
	case FORMAT_XML:
		return exportToXml(outFile);
		break;
	}

	return false;
}

bool DuplicatesModel::exportToIni(const QString &outFile)
{
	QSettings settings(outFile, QSettings::IniFormat);
	const int groupCount = m_root->childCount();

	settings.clear();

	if((!settings.isWritable()) || (settings.status() != QSettings::NoError))
	{
		qWarning("Failed to open output file!");
		return false;
	}

	settings.setValue("generator", tr("Document created with Double File Scanner v%1").arg(QString().sprintf("%u.%02u-%u", DOUBLESCANNER_VERSION_MAJOR, DOUBLESCANNER_VERSION_MINOR, DOUBLESCANNER_VERSION_PATCH)));
	settings.setValue("rights", tr("Copyright (C) 2014-2017 LoRd_MuldeR <MuldeR2@GMX.de>. Some rights reserved."));

	for(int i = 0; i < groupCount; i++)
	{
		if(DuplicateItem_Group *currentGroup = dynamic_cast<DuplicateItem_Group*>(m_root->child(i)))
		{
			settings.beginGroup(currentGroup->getHash().toHex());
			unsigned int counter = 0;
			const int fileCount = currentGroup->childCount();
			for(int j = 0; j < fileCount; j++)
			{
				if(DuplicateItem_File *currentFile = dynamic_cast<DuplicateItem_File*>(currentGroup->child(j)))
				{
					settings.setValue(QString().sprintf("%08u", counter++), QDir::toNativeSeparators(currentFile->getFilePath()));
				}
			}
			settings.endGroup();
		}
	}

	if((!settings.isWritable()) || (settings.status() != QSettings::NoError))
	{
		qWarning("Failed to write output file!");
		return false;
	}

	return true;
}

bool DuplicatesModel::exportToXml(const QString &outFile)
{
	QFile file(outFile);

	if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		qWarning("Failed to open output file!");
		return false;
	}

	QXmlStreamWriter stream(&file);
	const int groupCount = m_root->childCount();

	stream.setAutoFormatting(true);
	stream.writeStartDocument();

	QString tmpl(" %1 ");
	stream.writeComment(tmpl.arg(tr("=====================================================================")));
	stream.writeComment(tmpl.arg(tr("Document created with Double File Scanner v%1").arg(QString().sprintf("%u.%02u-%u", DOUBLESCANNER_VERSION_MAJOR, DOUBLESCANNER_VERSION_MINOR, DOUBLESCANNER_VERSION_PATCH))));
	stream.writeComment(tmpl.arg(tr("Copyright (C) 2014-2017 LoRd_MuldeR <MuldeR2@GMX.de>. Some rights reserved.")));
	stream.writeComment(tmpl.arg(tr("=====================================================================")));

	stream.writeStartElement("Duplicates");

	for(int i = 0; i < groupCount; i++)
	{
		if(DuplicateItem_Group *currentGroup = dynamic_cast<DuplicateItem_Group*>(m_root->child(i)))
		{
			stream.writeStartElement("Group");
			stream.writeAttribute("Hash", currentGroup->getHash().toHex());
			const int fileCount = currentGroup->childCount();
			for(int j = 0; j < fileCount; j++)
			{
				if(DuplicateItem_File *currentFile = dynamic_cast<DuplicateItem_File*>(currentGroup->child(j)))
				{
					stream.writeStartElement("File");
					stream.writeAttribute("Name", QDir::toNativeSeparators(currentFile->getFilePath()));
					stream.writeEndElement();
				}
			}
			stream.writeEndElement();
		}
	}

	stream.writeEndElement();
	
	if(stream.hasError())
	{
		qWarning("Failed to write output file!");
		file.close();
		return false;
	}

	file.close();
	return true;
}
