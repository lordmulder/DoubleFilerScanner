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
#include <QProcess>

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
	m_abortFlag = false;

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
	connect(ui->treeView, SIGNAL(activated(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));

	//Create directory scanner
	m_directoryScanner = new DirectoryScanner(&m_abortFlag);
	connect(m_directoryScanner, SIGNAL(finished()), this, SLOT(directoryScannerFinished()), Qt::QueuedConnection);

	//Create file comparator
	m_fileComparator = new FileComparator(m_model, &m_abortFlag);
	connect(m_fileComparator, SIGNAL(finished()), this, SLOT(fileComparatorFinished()), Qt::QueuedConnection);
	connect(m_fileComparator, SIGNAL(progressChanged(int)), this, SLOT(fileComparatorProgressChanged(int)), Qt::QueuedConnection);

	//Setup tree view
	ui->treeView->setExpandsOnDoubleClick(false);
	ui->treeView->setHeaderHidden(true);

	//Setup animator
	m_movie = new QMovie(":/res/Spinner.gif");
	m_animator = makeLabel(ui->treeView, ":/res/Spinner.gif");
	m_animator->setMovie(m_movie);

	//Create signs
	m_signQuiescent = makeLabel(ui->treeView, ":/res/Sign_Clocks.png", 0);
	m_signCompleted = makeLabel(ui->treeView, ":/res/Sign_Accept.png", 1);
	m_signCancelled = makeLabel(ui->treeView, ":/res/Sign_Cancel.png", 1);
}

MainWindow::~MainWindow(void)
{
	delete ui;

	MY_DELETE(m_fileComparator);
	MY_DELETE(m_directoryScanner);
	MY_DELETE(m_movie);
	MY_DELETE(m_animator);
	MY_DELETE(m_model);
	MY_DELETE(m_signCompleted);
	MY_DELETE(m_signCancelled);
	MY_DELETE(m_signQuiescent);
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
	centerWidget(m_animator);
	centerWidget(m_signCompleted);
	centerWidget(m_signCancelled);
	centerWidget(m_signQuiescent);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	if(!ui->buttonExit->isEnabled())
	{
		e->ignore();
	}
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_Escape)
	{
		if(!m_abortFlag)
		{
			qWarning("Operation has been aborted by user!");
			ui->label->setText(tr("Abortion has been request, terminating operation..."));
			m_abortFlag = true;
		}
	}

	QMainWindow::keyPressEvent(e);
}

//===================================================================
// Slots
//===================================================================

void MainWindow::startScan(void)
{
	const QString path = QFileDialog::getExistingDirectory(this, tr("Choose directory to be scanned"), QDir::homePath(), QFileDialog::ShowDirsOnly);

	if(!path.isEmpty())
	{
		setButtonsEnabled(false);
		m_abortFlag = false;
		UNSET_MODEL(ui->treeView);

		ui->label->setText(tr("Searching for files and directories, please be patient..."));
	
		m_signCompleted->hide();
		m_signCancelled->hide();
		m_signQuiescent->hide();

		ui->progressBar->setValue(0);
		ui->progressBar->setMaximum(0);

		m_directoryScanner->addDirectory(path);
		m_directoryScanner->start();
	}
}

void MainWindow::directoryScannerFinished(void)
{
	ui->progressBar->setMaximum(100);
	ui->progressBar->setValue(0);

	if(m_abortFlag)
	{
		QApplication::beep();
		ui->label->setText(tr("The operation has been aborted by the user!"));
		m_signCancelled->show();
		setButtonsEnabled(true);
		return;
	}
	
	const QStringList &files = m_directoryScanner->getFiles();
	ui->label->setText(tr("%1 file(s) are being analyzed, this might take a few minutes...").arg(QString::number(files.count())));

	m_fileComparator->addFiles(files);
	m_fileComparator->start();
}

void MainWindow::fileComparatorFinished(void)
{
	if(m_abortFlag)
	{
		QApplication::beep();
		ui->label->setText(tr("The operation has been aborted by the user!"));
		m_signCancelled->show();
		setButtonsEnabled(true);
		return;
	}

	m_abortFlag = true; /*to prevent an abort message after completion*/

	const QStringList &files = m_directoryScanner->getFiles();
	ui->label->setText(tr("Completed: %1 file(s) have been analyzed, %2 duplicate(s) have been identified.").arg(QString::number(files.count()), QString::number(m_model->duplicateCount())));

	if(m_model->duplicateCount() > 0)
	{
		UNSET_MODEL(ui->treeView);
		ui->treeView->setModel(m_model);
	}
	else
	{
		m_signCompleted->show();
	}

	QApplication::beep();
	ui->treeView->expandAll();
	setButtonsEnabled(true);
}

void MainWindow::fileComparatorProgressChanged(const int &progress)
{
	ui->progressBar->setValue(progress);
}

void MainWindow::itemActivated(const QModelIndex &index)
{
	const QString &filePath = m_model->getFilePath(index);
	if(!filePath.isEmpty())
	{
		shellExplore((const wchar_t*)QDir::toNativeSeparators(filePath).utf16());
	}
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

		QApplication::restoreOverrideCursor();
	}
}

void MainWindow::centerWidget(QWidget *widget)
{
	if(QAbstractScrollArea *parent = dynamic_cast<QAbstractScrollArea*>(widget->parent()))
	{
		const int x = parent->viewport()->width()  - widget->width();
		const int y = parent->viewport()->height() - widget->height();
		widget->move(x/2, y/2);
	}
	else if(QWidget *parent = dynamic_cast<QWidget*>(widget->parent()))
	{
		const int x = parent->width()  - widget->width();
		const int y = parent->height() - widget->height();
		widget->move(x/2, y/2);
	}
}

QLabel *MainWindow::makeLabel(QWidget *parent, const QString &fileName, const bool &hidden)
{
	QLabel *label = new QLabel(parent);
	QPixmap pixmap(fileName);

	if(!pixmap.isNull())
	{
		label->setPixmap(pixmap);
		label->setPixmap(pixmap);
		label->setFixedSize(pixmap.width(), pixmap.height());
	}
	else
	{
		qWarning("Failed to load pixmap: %s", fileName.toLatin1().constData());
	}

	if(hidden)
	{
		label->hide();
	}

	return label;
}
