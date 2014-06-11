#pragma once

#include "Model_Duplicates.h"

DuplicatesModel::DuplicatesModel(const QHash<QByteArray, QStringList> &data)
:
	m_data(data),
	m_keys(m_data.keys())
{
}

DuplicatesModel::~DuplicatesModel(void)
{
}

QModelIndex DuplicatesModel::index(int row, int column, const QModelIndex &parent) const
{
	return QModelIndex();
}

QModelIndex DuplicatesModel::parent(const QModelIndex &index) const
{
	return QModelIndex();
}

int DuplicatesModel::rowCount(const QModelIndex &parent) const
{
	return 0;
}

int DuplicatesModel::columnCount(const QModelIndex &parent) const
{
	return 0;
}

QVariant DuplicatesModel::data(const QModelIndex &index, int role) const
{
	return QVariant();
}
