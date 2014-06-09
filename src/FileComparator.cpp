#include "FileComparator.h"

#include <QThreadPool>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QTimer>
#include <QCryptographicHash>

#include <cassert>

static const quint64 MAX_ENQUEUED_TASKS = 128;

//=======================================================================================
// File Comparator
//=======================================================================================

FileComparator::FileComparator(const QStringList &files)
{
	m_pendingTasks = 0;
	m_pool = new QThreadPool(this);
	m_files << files;
	moveToThread(this);
}

FileComparator::~FileComparator(void)
{
	qDebug("FileComparator deleted.");
	delete m_pool;
}

void FileComparator::run(void)
{
	m_pendingTasks = 0;

	while((!m_files.empty()) && (m_pendingTasks < MAX_ENQUEUED_TASKS))
	{
		scanNextFile(m_files.dequeue());
	}

	exec();

	if(!m_files.empty())
	{
		qWarning("Thread is about to exit while there still are pending directories!");
	}

	qDebug("Thread will exit!");
}

void FileComparator::scanNextFile(const QString path)
{
	FileComparatorTask *task = new FileComparatorTask(path);
	if(connect(task, SIGNAL(fileAnalyzed(const QByteArray&, const QString&)), this, SLOT(fileDone(const QByteArray&, const QString&)), Qt::BlockingQueuedConnection))
	{
		m_pendingTasks++;
		m_pool->start(task);
	}
}

void FileComparator::fileDone(const QByteArray &hash, const QString &path)
{
	qDebug("Hash: %s <-- %s", hash.toHex().constData(), path.toUtf8().constData());
	
	while((!m_files.empty()) && (m_pendingTasks < MAX_ENQUEUED_TASKS))
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


//=======================================================================================
// File Comparator Task
//=======================================================================================

FileComparatorTask::FileComparatorTask(const QString &filePath)
:
	m_filePath(filePath)
{
}

FileComparatorTask::~FileComparatorTask(void)
{
	//qDebug("FileComparatorTask deleted.");
}

void FileComparatorTask::run(void)
{
	qDebug("Checking: %s", m_filePath.toUtf8().constData());

	QFile file(m_filePath);

	if(file.open(QIODevice::ReadOnly))
	{
		QCryptographicHash hash(QCryptographicHash::Sha1);

		while(!(file.atEnd() || (file.error() != QFile::NoError)))
		{
			const QByteArray buffer = file.read(1048576);
			if(buffer.size() > 0)
			{
				hash.addData(buffer);
			}
		}

		file.close();
		emit fileAnalyzed(hash.result(), m_filePath);
	}
	else
	{
		qWarning("Failed to open: %s", m_filePath.toUtf8().constData());
		emit fileAnalyzed(QByteArray(), QString());
	}
}
