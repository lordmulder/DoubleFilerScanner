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
	DirectoryScanner(const bool recursive = true);
	virtual ~DirectoryScanner(void);

	void addDirectory(const QString &path);
	const QStringList &getFiles(void) const;

private slots:
	void directoryDone(const QStringList *files, const QStringList *dirs);
	
protected:
	virtual void run(void);
	void scanDirectory(const QString path);
	
	const bool m_recusrive;

	QThreadPool *m_pool;
	QQueue<QString> m_pendingDirs;
	QStringList m_files;
	quint64 m_pendingTasks;
};
