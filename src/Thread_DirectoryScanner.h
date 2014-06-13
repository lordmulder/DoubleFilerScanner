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
#include <QSet>

class QThreadPool;
class QEventLoop;

//=======================================================================================

class DirectoryScannerTask : public QObject, public QRunnable
{
	Q_OBJECT

public:
	DirectoryScannerTask(const QString &directory, volatile bool *abortFlag);
	virtual ~DirectoryScannerTask(void);

signals:
	void directoryAnalyzed(const QStringList *files, const QStringList *dirs);

protected:
	virtual void run(void);
	
	const QString m_directory;
	volatile bool *const m_abortFlag;
};

//=======================================================================================

class DirectoryScanner : public QThread
{
	Q_OBJECT

public:
	DirectoryScanner(volatile bool *abortFlag, const bool recursive = true);
	virtual ~DirectoryScanner(void);

	void setRecursive(const bool &recusrive);
	void addDirectory(const QString &path);
	void addDirectories(const QStringList &paths);
	const QStringList getFiles(void) const;

private slots:
	void directoryDone(const QStringList *files, const QStringList *dirs);
	
protected:
	virtual void run(void);
	void scanDirectory(const QString path);
	
	bool m_recusrive;

	QThreadPool *m_pool;
	QQueue<QString> m_pendingDirs;
	QSet<QString> m_files;
	quint64 m_pendingTasks;

	volatile bool *const m_abortFlag;
};
