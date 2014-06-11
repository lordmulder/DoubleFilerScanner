#include "Thread_FileComparator.h"

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

FileComparator::FileComparator(const QStringList &files)
{
	m_pendingTasks = 0;
	m_pool = new QThreadPool(this);
	m_files << files;
	
	m_completedFileCount = 0;
	m_totalFileCount = m_files.count();
	m_progressValue = -1;
	
	moveToThread(this);
}

FileComparator::~FileComparator(void)
{
	//qDebug("FileComparator deleted.");
	delete m_pool;
}

void FileComparator::run(void)
{
	qDebug("[Analyzing Files]");

	m_hashes.clear();
	m_duplicates.clear();
	m_pendingTasks = 0;

	m_completedFileCount = 0;
	m_totalFileCount = m_files.count();
	m_progressValue = -1;
	
	while((!m_files.empty()) && (m_pendingTasks < MAX_ENQUEUED_TASKS))
	{
		scanNextFile(m_files.dequeue());
	}

	exec();

	if(!m_files.empty())
	{
		qWarning("Thread is about to exit while there still are pending directories!");
	}

	qDebug("\n[Searching Duplicates]");

	const QList<QByteArray> keys = m_hashes.uniqueKeys();
	for(QList<QByteArray>::ConstIterator iter = keys.constBegin(); iter != keys.constEnd(); iter++)
	{
		qDebug("%s -> %d", iter->toHex().constData(), m_hashes.count(*iter));
		if(m_hashes.count(*iter) > 1)
		{
			m_duplicates.insert((*iter), m_hashes.values(*iter));
		}
	}
	
	emit progressChanged(100);

	qDebug("Found %d files with duplicates!", m_duplicates.count());
	qDebug("Thread will exit!\n");
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
	if(!(hash.isEmpty() || path.isEmpty()))
	{
		m_hashes.insertMulti(hash, path);
	}

	const int progress = qRound(double(++m_completedFileCount) / double(m_totalFileCount) * 99.0);

	if(progress > m_progressValue)
	{
		m_progressValue = progress;
		emit progressChanged(m_progressValue);
	}

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

const QHash<QByteArray, QStringList> &FileComparator::getDuplicates(void) const
{
	if(this->isRunning())
	{
		qWarning("Result requested while thread is still running!");
		return EMPTY_DUPLICATES_LIST;
	}

	return m_duplicates;
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
	qDebug("%s", m_filePath.toUtf8().constData());

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
