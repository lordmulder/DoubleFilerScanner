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

#include "Thread_DirectoryScanner.h"

#include "Config.h"
#include "System.h"

#include <QThreadPool>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QTimer>

#include <cassert>

static const quint64 MAX_ENQUEUED_TASKS = 128;
static const QStringList EMPTY_STRINGLIST;

//=======================================================================================
// Directory Scanner
//=======================================================================================

DirectoryScanner::DirectoryScanner(volatile bool *abortFlag, const bool recursive)
:
	m_abortFlag(abortFlag),
	m_recusrive(recursive)
{
	m_pendingTasks = 0;
	m_pool = new QThreadPool(this);
}

DirectoryScanner::~DirectoryScanner(void)
{
	//qDebug("DirectoryScanner deleted.");
	MY_DELETE(m_pool);
}

void DirectoryScanner::run(void)
{
	qDebug("[Scanning Directory]");
	//qWarning("DirectoryScanner::run: Current thread id = %u", getCurrentThread());

	m_files.clear();
	m_pendingTasks = 0;

	if(m_pendingDirs.count() < 1)
	{
		qWarning("File list is empty -> Nothing to do!");
		return;
	}

	qDebug("Pending dirs: %d", m_pendingDirs.count());

	while((!m_pendingDirs.empty()) && (m_pendingTasks < MAX_ENQUEUED_TASKS))
	{
		scanDirectory(m_pendingDirs.dequeue());
	}

	exec();

	if(!m_pendingDirs.empty())
	{
		qWarning("Thread is about to exit while there still are pending directories!");
		m_pendingDirs.clear();
	}

	while(!m_pool->waitForDone(5000))
	{
		qWarning("Still have running taks -> waiting for completeion!");
	}

	qDebug("Found %d files!", m_files.count());
	qDebug("Thread will exit!\n");
}

void DirectoryScanner::scanDirectory(const QString path)
{
	DirectoryScannerTask *task = new DirectoryScannerTask(path, m_abortFlag);
	if(connect(task, SIGNAL(directoryAnalyzed(const QStringList*, const QStringList*)), this, SLOT(directoryDone(const QStringList*, const QStringList*)), Qt::BlockingQueuedConnection))
	{
		m_pendingTasks++;
		m_pool->start(task);
	}
}

void DirectoryScanner::directoryDone(const QStringList *files, const QStringList *dirs)
{
	for(QStringList::ConstIterator iter = files->constBegin(); iter != files->constEnd(); iter++)
	{
		m_files.insert(*iter);
	}

	if(m_recusrive)
	{
		m_pendingDirs << (*dirs);
	}

	while((!m_pendingDirs.empty()) && (m_pendingTasks < MAX_ENQUEUED_TASKS) && (!(*m_abortFlag)))
	{
		scanDirectory(m_pendingDirs.dequeue());
	}

	assert(m_pendingTasks > 0);

	if(--m_pendingTasks == 0)
	{
		qDebug("All tasks done!");
		QTimer::singleShot(0, this, SLOT(quit()));
	}
}

void DirectoryScanner::addDirectory(const QString &path)
{
	if(this->isRunning())
	{
		qWarning("Cannot add input while thread is still running!");
		return;
	}

	m_pendingDirs << path;
}

void DirectoryScanner::addDirectories(const QStringList &paths)
{
	if(this->isRunning())
	{
		qWarning("Cannot add input while thread is still running!");
		return;
	}

	m_pendingDirs << paths;
}

void DirectoryScanner::setRecursive(const bool &recusrive)
{
	if(this->isRunning())
	{
		qWarning("Cannot add input while thread is still running!");
		return;
	}

	m_recusrive = recusrive;
}

const QStringList DirectoryScanner::getFiles(void) const
{
	if(this->isRunning())
	{
		qWarning("Result requested while thread is still running!");
		return EMPTY_STRINGLIST;
	}

	QStringList fileList = m_files.values();

	fileList.sort();
	return fileList;
}

//=======================================================================================
// Directory Scanner Task
//=======================================================================================

DirectoryScannerTask::DirectoryScannerTask(const QString &directory, volatile bool *abortFlag)
:
	m_directory(directory),
	m_abortFlag(abortFlag)
{
}

DirectoryScannerTask::~DirectoryScannerTask(void)
{
	//qDebug("DirectoryScannerTask deleted.");
}

void DirectoryScannerTask::run(void)
{
	QStringList files, dirs;

	if(*m_abortFlag)
	{
		emit directoryAnalyzed(&files, &dirs);
		return;
	}

	qDebug("%s", m_directory.toUtf8().constData());
	QDirIterator iter(m_directory);

	while(iter.hasNext() && (!(*m_abortFlag)))
	{
		const QString path = iter.next();
		const QString name = iter.fileName();
		
		if((name.compare(".", Qt::CaseInsensitive) != 0) && (name.compare("..", Qt::CaseInsensitive) != 0))
		{
			const QFileInfo info = iter.fileInfo();

			if(info.exists())
			{
				if(info.isFile())
				{
					files << info.canonicalFilePath();
				}
				else if(info.isDir())
				{
					dirs << info.canonicalFilePath();
				}
			}
		}
	}

	files.sort();
	dirs.sort();

	emit directoryAnalyzed(&files, &dirs);
}
