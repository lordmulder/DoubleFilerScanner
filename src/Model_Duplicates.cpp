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

#include "Model_Duplicates.h"

#include <QFont>
#include <QIcon>
#include <QDir>
#include <QSettings>
#include <QXmlStreamWriter>

#include "Config.h"
#include "System.h"

static const QString EMPTY_STRING;

//===================================================================
// Duplicates Items
//===================================================================

class DuplicateItem
{
public:
	DuplicateItem(DuplicateItem *const parent, const QString &text = QString(), const bool &isFile = false);
	~DuplicateItem(void);

	inline const QString &text(void) const { return m_text; }
	inline void setText(const QString &text) { m_text = text; }
	inline DuplicateItem *child(const int index) const { return m_childeren[index]; }
	inline int childCount(void) const { return m_childeren.count(); }
	inline void addChild(DuplicateItem *child) { m_childeren << child; }
	inline int row(void) { return m_parent ? m_parent->m_childeren.indexOf(this) : 0; }
	inline DuplicateItem *parent(void) const { return m_parent; }
	inline bool isFile(void) const { return m_isFile; }

	void removeAllChilderen(void);
	void removeChild(DuplicateItem *const child);
	void dump(const int depth);

protected:
	QString m_text;
	const bool m_isFile;

	DuplicateItem *const m_parent;
	QList<DuplicateItem*> m_childeren;

private:
	DuplicateItem &operator=(const DuplicateItem&);
};

DuplicateItem::DuplicateItem(DuplicateItem *const parent, const QString &text, const bool &isFile)
:
	m_parent(parent),
	m_text(text),
	m_isFile(isFile)
{
	if(parent)
	{
		parent->addChild(this);
	}
}

DuplicateItem::~DuplicateItem(void)
{
	removeAllChilderen();
}

void DuplicateItem::removeAllChilderen(void)
{
	while(!m_childeren.isEmpty())
	{
		DuplicateItem *child = m_childeren.takeLast();
		MY_DELETE(child);
	}
}

void DuplicateItem::removeChild(DuplicateItem *const child)
{
	m_childeren.removeAll(child);
}

void DuplicateItem::dump(int depth = 0)
{
	static const char *tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	qDebug("%.*s%s", depth, tabs, m_text.toUtf8().constData());
	for(QList<DuplicateItem*>::ConstIterator iter = m_childeren.constBegin(); iter != m_childeren.constEnd(); iter++)
	{
		(*iter)->dump(depth + 1);
	}
}

//===================================================================
// Constructor & Destructor
//===================================================================

DuplicatesModel::DuplicatesModel(void)
{
	m_bulIcon = new QIcon(":/res/Icon_Bullet.png");
	m_dupIcon = new QIcon(":/res/Icon_Duplicate.png");

	m_fontDflt = new QFont("Monospace");
	m_fontDflt->setStyleHint(QFont::TypeWriter);
	m_fontBold = new QFont("Monospace");
	m_fontBold->setStyleHint(QFont::TypeWriter);
	m_fontBold->setBold(true);

	m_root = new DuplicateItem(NULL, "ROOT BLOODY ROOT");
}

DuplicatesModel::~DuplicatesModel(void)
{
	MY_DELETE(m_root);
	MY_DELETE(m_fontDflt);
	MY_DELETE(m_fontBold);
	MY_DELETE(m_bulIcon);
	MY_DELETE(m_dupIcon);
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

int DuplicatesModel::columnCount(const QModelIndex&) const
{
	return 1;
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
	case Qt::DisplayRole:
		return QDir::toNativeSeparators(item->text());
	case Qt::FontRole:
		return item->isFile() ? (*m_fontDflt) : (*m_fontBold);
	case Qt::DecorationRole:
		return item->isFile() ? (*m_bulIcon) : (*m_dupIcon);
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
		if(DuplicateItem *currentFile = static_cast<DuplicateItem*>(index.internalPointer()))
		{
			if(currentFile->isFile())
			{
				if(DuplicateItem *parent = currentFile->parent())
				{
					return parent->childCount();
				}
			}
		}
	}
	
	return 0;
}

const QString &DuplicatesModel::getFilePath(const QModelIndex &index) const
{
	if(index.isValid())
	{
		if(DuplicateItem *currentFile = static_cast<DuplicateItem*>(index.internalPointer()))
		{
			if(currentFile->isFile())
			{
				return currentFile->text();
			}
		}
	}
	
	return EMPTY_STRING;
}

QString DuplicatesModel::toString(void)
{
	QStringList lines;
	const int hashCount = m_root->childCount();

	for(int i = 0; i < hashCount; i++)
	{
		DuplicateItem *currentHash = m_root->child(i);
		lines << currentHash->text();
		const int fileCount = currentHash->childCount();

		for(int j = 0; j < fileCount; j++)
		{
			DuplicateItem *currentFile = currentHash->child(j);
			lines << QString("- %1").arg(QDir::toNativeSeparators(currentFile->text()));
		}

		lines << QString();
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

void DuplicatesModel::addDuplicate(const QByteArray &hash, const QStringList &files)
{
	beginInsertRows(QModelIndex(), m_root->childCount(), m_root->childCount());

	DuplicateItem *currentKey = new DuplicateItem(m_root, hash.toHex().constData());
	for(QStringList::ConstIterator iterFile = files.constBegin(); iterFile != files.constEnd(); iterFile++)
	{
		new DuplicateItem(currentKey, (*iterFile), true);
	}

	endInsertRows();
}

bool DuplicatesModel::renameFile(const QModelIndex &index, const QString &newFileName)
{
	if(index.isValid())
	{
		if(DuplicateItem *currentFile = static_cast<DuplicateItem*>(index.internalPointer()))
		{
			if(currentFile->isFile())
			{
				const QString oldFilePath = currentFile->text();
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
						currentFile->setText(newFilePath);
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
		if(DuplicateItem *currentFile = static_cast<DuplicateItem*>(index.internalPointer()))
		{
			if(currentFile->isFile())
			{
				const QString oldFilePath = currentFile->text();
				if(QFileInfo(oldFilePath).exists() && QFileInfo(oldFilePath).isFile())
				{
					if(QFile::remove(currentFile->text()))
					{
						if(DuplicateItem *parentItem = currentFile->parent())
						{
							beginRemoveRows(parent(index), index.row(), index.row());
							parentItem->removeChild(currentFile);
							endRemoveRows();
						}
						return true;
					}
				}
				else
				{
					//file no longer exists -> remove from model!
					if(DuplicateItem *parentItem = currentFile->parent())
					{
						beginRemoveRows(parent(index), index.row(), index.row());
						parentItem->removeChild(currentFile);
						endRemoveRows();
					}
				}
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
	settings.clear();
	const int hashCount = m_root->childCount();
	
	if(settings.status() != QSettings::NoError)
	{
		qWarning("Failed to open output file!");
		return false;
	}

	for(int i = 0; i < hashCount; i++)
	{
		DuplicateItem *currentHash = m_root->child(i);

		settings.beginGroup(currentHash->text());
		unsigned int counter = 0;
		const int fileCount = currentHash->childCount();
		
		for(int j = 0; j < fileCount; j++)
		{
			DuplicateItem *currentFile = currentHash->child(j);
			settings.setValue(QString().sprintf("%08u", counter++), QDir::toNativeSeparators(currentFile->text()));
		}

		settings.endGroup();
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
	const int hashCount = m_root->childCount();

	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement("Duplicates");

	for(int i = 0; i < hashCount; i++)
	{
		DuplicateItem *currentHash = m_root->child(i);

		stream.writeStartElement("Group");
		stream.writeAttribute("Hash", currentHash->text());
		
		const int fileCount = currentHash->childCount();
		
		for(int j = 0; j < fileCount; j++)
		{
			DuplicateItem *currentFile = currentHash->child(j);

			stream.writeStartElement("File");
			stream.writeAttribute("Name", QDir::toNativeSeparators(currentFile->text()));
			stream.writeEndElement();
		}

		stream.writeEndElement();
	}

	stream.writeEndElement();
	file.close();
	
	return true;
}
