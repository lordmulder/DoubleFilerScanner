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

#include <QReadWriteLock>
#include <QFont>
#include <QIcon>
#include <QDir>

#include "Config.h"

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
	inline DuplicateItem *child(const int index) const { return m_childeren[index]; }
	inline int childCount(void) const { return m_childeren.count(); }
	inline void addChild(DuplicateItem *child) { m_childeren << child; }
	inline int row(void) { return m_parent ? m_parent->m_childeren.indexOf(this) : 0; }
	inline DuplicateItem *parent(void) const { return m_parent; }
	inline bool isFile(void) const { return m_isFile; }

	void removeAllChilderen(void);
	void dump(const int depth);

protected:
	const QString m_text;
	const bool m_isFile;

	DuplicateItem *const m_parent;
	QList<DuplicateItem*> m_childeren;
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
// Duplicates Model
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

QModelIndex DuplicatesModel::index(int row, int column, const QModelIndex &parent) const
{
	QReadLocker readLock(&m_lock);

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
	QReadLocker readLock(&m_lock);

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
	QReadLocker readLock(&m_lock);

	DuplicateItem *item = m_root;
	if(parent.isValid() && parent.internalPointer())
	{
		item = static_cast<DuplicateItem*>(parent.internalPointer());
	}

	return item->childCount();
}

int DuplicatesModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}

QVariant DuplicatesModel::data(const QModelIndex &index, int role) const
{
	QReadLocker readLock(&m_lock);

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

void DuplicatesModel::clear(void)
{
	QWriteLocker writeLock(&m_lock);

	beginResetModel();
	m_root->removeAllChilderen();
	endResetModel();
}

void DuplicatesModel::addDuplicate(const QByteArray &hash, const QStringList files)
{
	QWriteLocker writeLock(&m_lock);

	beginResetModel();
	DuplicateItem *currentKey = new DuplicateItem(m_root, hash.toHex().constData());
	for(QStringList::ConstIterator iterFile = files.constBegin(); iterFile != files.constEnd(); iterFile++)
	{
		DuplicateItem *currentFile = new DuplicateItem(currentKey, (*iterFile), true);
	}
	endResetModel();
}

unsigned int DuplicatesModel::duplicateCount(void) const
{
	QReadLocker readLock(&m_lock);
	return m_root->childCount();
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
