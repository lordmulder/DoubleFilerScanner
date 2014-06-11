#pragma once

#include "Window_Main.h"
#include "UIC_Window_Main.h"

#include "Config.h"
#include "System.h"
#include "Thread_DirectoryScanner.h"
#include "Thread_FileComparator.h"
#include "Model_Duplicates.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QMovie>

#include <cassert>

static void UNSET_MODEL(QTreeView *view)
{
	QItemSelectionModel *selectionModel = view->selectionModel();
	view->setModel(NULL);
	MY_DELETE(selectionModel);
}

//===================================================================
// Constructor & Destructor
//===================================================================

MainWindow::MainWindow(void)
:
	ui(new Ui::MainWindow())
{
	//qWarning("MainWindow::MainWindow: Current thread id = %u", getCurrentThread());

	//Setup window flags
	setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMaximizeButtonHint);

	//Setup UI
	ui->setupUi(this);

	//Setup title
	setWindowTitle(tr("Double File Scanner %1").arg(QString().sprintf("v%u.%02u-%u", DOUBLESCANNER_VERSION_MAJOR, DOUBLESCANNER_VERSION_MINOR, DOUBLESCANNER_VERSION_PATCH)));

	//Setup size
	setMinimumSize(size());

	//Setup connections
	connect(ui->buttonStart, SIGNAL(clicked()), this, SLOT(startScan()));
	connect(ui->buttonAbout, SIGNAL(clicked()), this, SLOT(showAbout()));
	connect(ui->buttonExit,  SIGNAL(clicked()), this, SLOT(close()));

	//Create model
	m_model = new DuplicatesModel();

	//Create directory scanner
	m_directoryScanner = new DirectoryScanner();
	connect(m_directoryScanner, SIGNAL(finished()), this, SLOT(directoryScannerFinished()), Qt::QueuedConnection);

	//Create file comparator
	m_fileComparator = new FileComparator(m_model);
	connect(m_fileComparator, SIGNAL(finished()), this, SLOT(fileComparatorFinished()), Qt::QueuedConnection);
	connect(m_fileComparator, SIGNAL(progressChanged(int)), this, SLOT(fileComparatorProgressChanged(int)), Qt::QueuedConnection);

	//Setup tree view
	ui->treeView->header()->hide();

	//Setup animator
	m_animator = new QLabel(ui->treeView);
	QPixmap spinner(":/res/Spinner.gif");
	m_animator->setFixedSize(spinner.size());
	m_animator->setPixmap(spinner);
	m_movie = new QMovie(":/res/Spinner.gif");
	m_animator->hide();
	m_animator->setMovie(m_movie);
}

MainWindow::~MainWindow(void)
{
	delete ui;

	MY_DELETE(m_fileComparator);
	MY_DELETE(m_directoryScanner);
	MY_DELETE(m_movie);
	MY_DELETE(m_animator);
	MY_DELETE(m_model);
}

//===================================================================
// Events
//===================================================================

void MainWindow::showEvent(QShowEvent *e)
{
	resizeEvent(NULL);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
	const int x = (ui->treeView->viewport()->width()  - m_animator->width())  / 2;
	const int y = (ui->treeView->viewport()->height() - m_animator->height()) / 2;
	m_animator->move(x, y);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	if(!ui->buttonExit->isEnabled())
	{
		e->ignore();
	}
}

//===================================================================
// Slots
//===================================================================

void MainWindow::startScan(void)
{
	const QString path = QFileDialog::getExistingDirectory(this, tr("Choose directory to be scanned"), QDir::homePath(), QFileDialog::ShowDirsOnly);

	if(!path.isEmpty())
	{
		//qWarning("MainWindow::startScan: Current thread id = %u", getCurrentThread());

		UNSET_MODEL(ui->treeView);
		setButtonsEnabled(false);
		ui->label->setText(tr("Searching for files and directories, please be patient..."));
	
		ui->progressBar->setValue(0);
		ui->progressBar->setMaximum(0);

		m_directoryScanner->addDirectory(path);
		m_directoryScanner->start();
	}
}

void MainWindow::directoryScannerFinished(void)
{
	const QStringList &files = m_directoryScanner->getFiles();
	ui->label->setText(tr("%1 file(s) are being analyzed, this might take a few minutes...").arg(QString::number(files.count())));
	
	ui->progressBar->setMaximum(100);
	ui->progressBar->setValue(0);

	m_fileComparator->addFiles(files);
	m_fileComparator->start();
}

void MainWindow::fileComparatorProgressChanged(const int &progress)
{
	ui->progressBar->setValue(progress);
}

void MainWindow::fileComparatorFinished(void)
{
	const QStringList &files = m_directoryScanner->getFiles();
	ui->label->setText(tr("Completed: %1 file(s) have been analyzed, %2 duplicate(s) have been identified.").arg(QString::number(files.count()), QString::number(m_model->duplicateCount())));

	UNSET_MODEL(ui->treeView);
	ui->treeView->setModel(m_model);

	QApplication::beep();
	ui->treeView->expandAll();
	setButtonsEnabled(true);
}

void MainWindow::showAbout(void)
{
	QMessageBox::aboutQt(this);
}

//===================================================================
// Private Functions
//===================================================================

void MainWindow::setButtonsEnabled(const bool &enabled)
{
	ui->buttonStart->setEnabled(enabled);
	ui->buttonExit->setEnabled(enabled);
	ui->buttonAbout->setEnabled(enabled);
	
	if(!enabled)
	{
		m_animator->show();
		m_movie->start();

		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	}
	else
	{
		m_movie->stop();
		m_animator->hide();
	
		ui->progressBar->setMaximum(100);
		ui->progressBar->setValue(100);

		QApplication::restoreOverrideCursor();
	}
}
