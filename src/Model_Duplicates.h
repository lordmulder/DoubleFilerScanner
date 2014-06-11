#pragma once

#include <QAbstractItemModel>
#include <QStringList>

//DuplicatesModel class
class DuplicatesModel: public QAbstractItemModel
{
	Q_OBJECT

public:
	DuplicatesModel(const QHash<QByteArray, QStringList> &data);
	virtual ~DuplicatesModel(void);
	
	//QAbstractItemModel
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	
protected:
	const QHash<QByteArray, QStringList> m_data;
	const QList<QByteArray> m_keys;
};
