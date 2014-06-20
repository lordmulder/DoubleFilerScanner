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

#include <QThread>
#include <QRunnable>
#include <QStringList>
#include <QQueue>
#include <QHash>
#include <QReadWriteLock>

class QThreadPool;
class QEventLoop;
class DuplicatesModel;

//=======================================================================================

class FileComparatorTask : public QObject, public QRunnable
{
	Q_OBJECT

public:
	FileComparatorTask(const QString &filePath, volatile bool *abortFlag);
	virtual ~FileComparatorTask(void);

signals:
	void fileAnalyzed(const QByteArray &hash, const QString &path, const qint64 &fileSize);

protected:
	virtual void run(void);
	
	const QString m_filePath;
	volatile bool* const m_abortFlag;
};

//=======================================================================================

class FileComparator : public QThread
{
	Q_OBJECT

public:
	FileComparator(volatile bool *abortFlag, const int &threadCount = -1);
	virtual ~FileComparator(void);

	void addFiles(const QStringList &files);

private slots:
	void fileDone(const QByteArray &hash, const QString &path, const qint64 &fileSize);

signals:
	void progressChanged(const int &progress);
	void duplicateFound(const QByteArray &hash, const QStringList &path, const qint64 size);

protected:
	virtual void run(void);
	void scanNextFile(const QString path);

	QThreadPool *m_pool;

	QQueue<QString> m_files;
	quint64 m_pendingTasks;

	QHash<QByteArray, QString> m_hashes;
	QHash<QByteArray, qint64> m_fileSizes;

	int m_totalFileCount;
	int m_completedFileCount;
	int m_progressValue;

	volatile bool *const m_abortFlag;
};
