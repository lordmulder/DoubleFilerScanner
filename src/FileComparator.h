#pragma once

#include <QThread>
#include <QRunnable>
#include <QStringList>
#include <QQueue>

class QThreadPool;
class QEventLoop;

//=======================================================================================

class FileComparatorTask : public QObject, public QRunnable
{
	Q_OBJECT

public:
	FileComparatorTask(const QString &filePath);
	virtual ~FileComparatorTask(void);

signals:
	void fileAnalyzed(const QByteArray &hash, const QString &path);

protected:
	virtual void run(void);
	
	const QString m_filePath;
};

//=======================================================================================

class FileComparator : public QThread
{
	Q_OBJECT

public:
	FileComparator(const QStringList &files);
	virtual ~FileComparator(void);

private slots:
	void fileDone(const QByteArray &hash, const QString &path);
	
protected:
	virtual void run(void);
	void scanNextFile(const QString path);

	QThreadPool *m_pool;

	QQueue<QString> m_files;
	quint64 m_pendingTasks;
};
