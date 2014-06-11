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
	void addDuplicate(const QByteArray &hash, const QStringList files);
	void clear(void);

protected:
	DuplicateItem *m_root;

	QIcon *m_dupIcon;
	QFont *m_fontDefault;
	QFont *m_fontBold;

	mutable QReadWriteLock m_lock;
};
