#pragma once

#include <QThread>
#include <QRunnable>
#include <QStringList>
#include <QQueue>

class QThreadPool;
class QEventLoop;

//=======================================================================================

class DirectoryScannerTask : public QObject, public QRunnable
{
	Q_OBJECT

public:
	DirectoryScannerTask(const QString &directory);
	virtual ~DirectoryScannerTask(void);

signals:
	void directoryAnalyzed(const QStringList *files, const QStringList *dirs);

protected:
	virtual void run(void);
	
	const QString m_directory;
};

//=======================================================================================

class DirectoryScanner : public QThread
{
	Q_OBJECT

public:
	DirectoryScanner(const QString &directory, const bool recursive = true);
	virtual ~DirectoryScanner(void);

	const QStringList &getFiles(void) const;

private slots:
	void directoryDone(const QStringList *files, const QStringList *dirs);
	
protected:
	virtual void run(void);
	void scanDirectory(const QString path);
	
	const QString m_directory;
	const bool m_recusrive;

	QThreadPool *m_pool;

	QStringList m_files;
	QQueue<QString> m_pendingDirs;
	quint64 m_pendingTasks;
};
