#pragma once

#include <QMainWindow>

//UIC forward declartion
namespace Ui {
	class MainWindow;
}

class QLabel;
class QMovie;
class DirectoryScanner;
class FileComparator;
class DuplicatesModel;
class QModelIndex;

//MainWindow class
class MainWindow: public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(void);
	virtual ~MainWindow(void);

private slots:
	void startScan(void);
	void directoryScannerFinished(void);
	void fileComparatorProgressChanged(const int &progress);
	void fileComparatorFinished(void);
	void itemActivated(const QModelIndex &index);
	void showAbout(void);

protected:
	virtual void closeEvent(QCloseEvent *e);
	virtual void resizeEvent(QResizeEvent *e);
	virtual void showEvent(QShowEvent *e);
	virtual void keyPressEvent(QKeyEvent *e);

	void centerWidget(QWidget *widget);
	QLabel *makeLabel(QWidget *parent, const QString &fileName, const bool &hidden = true);
	void setButtonsEnabled(const bool &enabled);

	QLabel *m_animator;
	QMovie *m_movie;

	QLabel *m_signCompleted;
	QLabel *m_signCancelled;
	QLabel *m_signQuiescent;

	volatile bool m_abortFlag;

	DuplicatesModel *m_model;
	DirectoryScanner *m_directoryScanner;
	FileComparator *m_fileComparator;

	Ui::MainWindow *const ui;
};
