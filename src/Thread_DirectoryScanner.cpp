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

DirectoryScanner::DirectoryScanner(const bool recursive)
:
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

	qDebug("Pending dirs: %d", m_pendingDirs.count());

	while((!m_pendingDirs.empty()) && (m_pendingTasks < MAX_ENQUEUED_TASKS))
	{
		qDebug("Test");
		scanDirectory(m_pendingDirs.dequeue());
	}

	exec();

	if(!m_pendingDirs.empty())
	{
		qWarning("Thread is about to exit while there still are pending directories!");
	}

	qDebug("Found %d files!", m_files.count());
	m_files.sort();

	qDebug("Thread will exit!\n");
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
	}

	while((!m_pendingDirs.empty()) && (m_pendingTasks < MAX_ENQUEUED_TASKS))
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

const QStringList &DirectoryScanner::getFiles(void) const
{
	if(this->isRunning())
	{
		qWarning("Result requested while thread is still running!");
		return EMPTY_STRINGLIST;
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
	qDebug("%s", m_directory.toUtf8().constData());

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

	files.sort();
	dirs.sort();

	emit directoryAnalyzed(&files, &dirs);
}
