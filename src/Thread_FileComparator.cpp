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

#include "Thread_FileComparator.h"

#include "Model_Duplicates.h"
#include "Config.h"
#include "System.h"

#include <QThreadPool>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QTimer>
#include <QCryptographicHash>

#include <cassert>

static const quint64 MAX_ENQUEUED_TASKS = 128;
static const QHash<QByteArray, QStringList> EMPTY_DUPLICATES_LIST;

//=======================================================================================
// File Comparator
//=======================================================================================

FileComparator::FileComparator(DuplicatesModel *model, volatile bool *abortFlag)
:
	m_model(model),
	m_abortFlag(abortFlag)
{
	m_pendingTasks = 0;
	m_pool = new QThreadPool(this);

	m_completedFileCount = 0;
	m_totalFileCount = m_files.count();
	m_progressValue = -1;
}

FileComparator::~FileComparator(void)
{
	//qDebug("FileComparator deleted.");
	MY_DELETE(m_pool);
}

void FileComparator::run(void)
{
	qDebug("[Analyzing Files]");
	//qWarning("FileComparator::run: Current thread id = %u", getCurrentThread());

	m_hashes.clear();
	m_pendingTasks = 0;
	m_model->clear();

	m_completedFileCount = 0;
	m_totalFileCount = m_files.count();
	m_progressValue = -1;
	
	if(m_files.count() < 1)
	{
		qWarning("File list is empty -> Nothing to do!");
		emit progressChanged(100);
		return;
	}

	while((!m_files.empty()) && (m_pendingTasks < MAX_ENQUEUED_TASKS))
	{
		scanNextFile(m_files.dequeue());
	}

	exec();

	if(!m_files.empty())
	{
		qWarning("Thread is about to exit while there still are pending directories!");
		m_files.clear();
	}

	while(!m_pool->waitForDone(5000))
	{
		qWarning("Still have running taks -> waiting for completeion!");
	}

	if(!(*m_abortFlag))
	{
		qDebug("\n[Searching Duplicates]");
		quint32 duplicateCount = 0;

		const QList<QByteArray> keys = m_hashes.uniqueKeys();
		for(QList<QByteArray>::ConstIterator iter = keys.constBegin(); iter != keys.constEnd(); iter++)
		{
			qDebug("%s -> %d", iter->toHex().constData(), m_hashes.count(*iter));
			if(m_hashes.count(*iter) > 1)
			{
				m_model->addDuplicate((*iter), m_hashes.values(*iter));
				duplicateCount++;
			}
		}
	
		qDebug("Found %d files with duplicates!", duplicateCount);
		emit progressChanged(100);
	}

	qDebug("Thread will exit!\n");
}

void FileComparator::scanNextFile(const QString path)
{
	FileComparatorTask *task = new FileComparatorTask(path, m_abortFlag);
	if(connect(task, SIGNAL(fileAnalyzed(const QByteArray&, const QString&)), this, SLOT(fileDone(const QByteArray&, const QString&)), Qt::BlockingQueuedConnection))
	{
		m_pendingTasks++;
		m_pool->start(task);
	}
}

void FileComparator::fileDone(const QByteArray &hash, const QString &path)
{
	if(!(hash.isEmpty() || path.isEmpty()))
	{
		m_hashes.insertMulti(hash, path);
	}

	const int progress = qRound(double(++m_completedFileCount) / double(m_totalFileCount) * 99.0);

	if((progress > m_progressValue) && (!(*m_abortFlag)))
	{
		m_progressValue = progress;
		emit progressChanged(m_progressValue);
	}

	while((!m_files.empty()) && (m_pendingTasks < MAX_ENQUEUED_TASKS) && (!(*m_abortFlag)))
	{
		scanNextFile(m_files.dequeue());
	}

	assert(m_pendingTasks > 0);

	if(--m_pendingTasks == 0)
	{
		qDebug("All tasks done!");
		QTimer::singleShot(0, this, SLOT(quit()));
	}
}

void FileComparator::addFiles(const QStringList &files)
{
	if(this->isRunning())
	{
		qWarning("Cannot add input while thread is still running!");
		return;
	}

	m_files << files;
}

//=======================================================================================
// File Comparator Task
//=======================================================================================

FileComparatorTask::FileComparatorTask(const QString &filePath, volatile bool *abortFlag)
:
	m_filePath(filePath),
	m_abortFlag(abortFlag)
{
}

FileComparatorTask::~FileComparatorTask(void)
{
	//qDebug("FileComparatorTask deleted.");
}

void FileComparatorTask::run(void)
{
	if(*m_abortFlag)
	{
		emit fileAnalyzed(QByteArray(), QString());
	}
	
	qDebug("%s", m_filePath.toUtf8().constData());

	QFile file(m_filePath);

	if(file.open(QIODevice::ReadOnly))
	{
		QCryptographicHash hash(QCryptographicHash::Sha1);

		while(!(file.atEnd() || (file.error() != QFile::NoError) || (*m_abortFlag)))
		{
			const QByteArray buffer = file.read(1048576);
			if(buffer.size() > 0)
			{
				hash.addData(buffer);
			}
		}

		file.close();

		if(!(*m_abortFlag))
		{
			emit fileAnalyzed(hash.result(), m_filePath);
			return;
		}
	}

	if(!(*m_abortFlag))
	{
		qWarning("Failed to open: %s", m_filePath.toUtf8().constData());
	}

	emit fileAnalyzed(QByteArray(), QString());
}
