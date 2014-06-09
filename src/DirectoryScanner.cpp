#include "DirectoryScanner.h"

#include <QThreadPool>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QTimer>

#include <cassert>

static const quint64 MAX_ENQUEUED_TASKS = 128;

//=======================================================================================
// Directory Scanner
//=======================================================================================

DirectoryScanner::DirectoryScanner(const QString &directory, const bool recursive)
:
	m_directory(directory),
	m_recusrive(recursive)
{
	m_pendingTasks = 0;
	m_pool = new QThreadPool(this);
}

DirectoryScanner::~DirectoryScanner(void)
{
	qDebug("DirectoryScanner deleted.");
	delete m_pool;
}

void DirectoryScanner::run(void)
{
	m_files.clear();
	m_pendingDirs.clear();
	m_pendingTasks = 0;

	scanDirectory(m_directory);
	exec();

	if(!m_pendingDirs.empty())
	{
		qWarning("Thread is about to exit while there still are pending directories!");
	}

	qDebug("Found %d files!", m_files.count());
	m_files.sort();

	qDebug("Thread will exit!");
}

void DirectoryScanner::scanDirectory(const QString path)
{
	DirectoryScannerTask *task = new DirectoryScannerTask(path);
	if(connect(task, SIGNAL(directoryAnalyzed(const QStringList*, const QStringList*)), this, SLOT(directoryDone(const QStringList*, const QStringList*)), Qt::BlockingQueuedConnection))
	{
		m_pendingTasks++;
		m_pool->start(task);
	}
}

void DirectoryScanner::directoryDone(const QStringList *files, const QStringList *dirs)
{
	m_files << (*files);

	if(m_recusrive)
	{
		m_pendingDirs << (*dirs);
		while((!m_pendingDirs.empty()) && (m_pendingTasks < MAX_ENQUEUED_TASKS))
		{
			scanDirectory(m_pendingDirs.dequeue());
		}
	}

	assert(m_pendingTasks > 0);

	if(--m_pendingTasks == 0)
	{
		qDebug("All tasks done!");
		QTimer::singleShot(0, this, SLOT(quit()));
	}
}

const QStringList &DirectoryScanner::getFiles(void) const
{
	if(this->isRunning())
	{
		qWarning("Result requested while thread is still running!");
	}

	return m_files;
}

//=======================================================================================
// Directory Scanner Task
//=======================================================================================

DirectoryScannerTask::DirectoryScannerTask(const QString &directory)
:
	m_directory(directory)
{
}

DirectoryScannerTask::~DirectoryScannerTask(void)
{
	//qDebug("DirectoryScannerTask deleted.");
}

void DirectoryScannerTask::run(void)
{
	qDebug("Scanning: %s", m_directory.toUtf8().constData());

	QStringList files, dirs;
	QDirIterator iter(m_directory);

	while(iter.hasNext())
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
					files << path;
				}
				else if(info.isDir())
				{
					dirs << path;
				}
			}
		}
	}

	emit directoryAnalyzed(&files, &dirs);
}
