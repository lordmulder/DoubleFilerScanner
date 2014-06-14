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
#include "Window_Directories.h"
#include "Taskbar.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QMovie>
#include <QProcess>
#include <QMap>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>

#include <cassert>

static void UNSET_MODEL(QTreeView *view)
{
	QItemSelectionModel *selectionModel = view->selectionModel();
	view->setModel(NULL);
	MY_DELETE(selectionModel);
}

static void ENABLE_MENU(QMenu *menu, const bool &enabled)
{
	QList<QAction*> actions = menu->actions();
	for(QList<QAction*>::Iterator iter = actions.begin(); iter != actions.end(); iter++)
	{
		(*iter)->setEnabled(enabled);
	}
}

#define ENSURE_APP_IS_IDLE() do \
{ \
	if(QApplication::activeModalWidget() || (!ui->buttonStart->isEnabled())) \
	{ \
		qWarning("Cannot perform action at this time!"); \
		return; \
	} \
} \
while(0)

static const char HOMEPAGE_URL[] = "http://muldersoft.com/";

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

	//Setup window icon
	changeWindowIcon(this, QIcon(":/res/Logo.png"));

	//Setup title
	setWindowTitle(tr("Double File Scanner %1").arg(QString().sprintf("v%u.%02u-%u", DOUBLESCANNER_VERSION_MAJOR, DOUBLESCANNER_VERSION_MINOR, DOUBLESCANNER_VERSION_PATCH)));

	//Setup size
	setMinimumSize(size());

	//Setup button connections
	connect(ui->buttonStart, SIGNAL(clicked()), this, SLOT(startScan()));
	connect(ui->buttonAbout, SIGNAL(clicked()), this, SLOT(showAbout()));
	connect(ui->buttonExit,  SIGNAL(clicked()), this, SLOT(close()));

	//Setup menu connections
	connect(ui->actionStart,     SIGNAL(triggered()), this, SLOT(startScan()));
	connect(ui->actionClear,   SIGNAL(triggered()), this, SLOT(clearData()));
	connect(ui->actionExport,   SIGNAL(triggered()), this, SLOT(exportToFile()));
	connect(ui->actionClipbrd,  SIGNAL(triggered()), this, SLOT(copyToClipboard()));
	connect(ui->actionExit,     SIGNAL(triggered()), this, SLOT(close()));
	connect(ui->actionHomepage, SIGNAL(triggered()), this, SLOT(showHomepage()));
	connect(ui->actionAbout,    SIGNAL(triggered()), this, SLOT(showAbout()));
	
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

	//Create context menu
	ui->treeView->addActions(ui->menuEdit->actions());
	ui->treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
	ENABLE_MENU(ui->menuEdit, false);

	//Enable drag&drop support
	setAcceptDrops(true);
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
	QMainWindow::showEvent(e);
	resizeEvent(NULL);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
	QMainWindow::resizeEvent(e);

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

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
	if(ui->buttonStart->isEnabled())
	{
		QStringList formats = e->mimeData()->formats();
	
		if(formats.contains("application/x-qt-windows-mime;value=\"FileNameW\"", Qt::CaseInsensitive) && formats.contains("text/uri-list", Qt::CaseInsensitive))
		{
			e->acceptProposedAction();
		}
	}
}

void MainWindow::dropEvent(QDropEvent *e)
{
	if(ui->buttonStart->isEnabled())
	{
		m_droppedFolders.clear();
		QList<QUrl> urls = e->mimeData()->urls();

		for(QList<QUrl>::ConstIterator iter = urls.constBegin(); iter != urls.constEnd(); iter++)
		{
			QFileInfo item(iter->toLocalFile());
			if(item.exists() && item.isDir())
			{
				m_droppedFolders << item.canonicalFilePath();
			}
		}

		if(!m_droppedFolders.isEmpty())
		{
			QTimer::singleShot(0, this, SLOT(startScan()));
		}
	}
}

bool MainWindow::winEvent(MSG *message, long *result)
{
	return Taskbar::handleWinEvent(message, result);
}

//===================================================================
// Slots
//===================================================================

void MainWindow::startScan(void)
{
	ENSURE_APP_IS_IDLE();

	DirectoriesDialog *directoriesDialog = new DirectoriesDialog(this);

	if(!m_droppedFolders.isEmpty())
	{
		directoriesDialog->addDirectories(m_droppedFolders);
		m_droppedFolders.clear();
	}

	if(directoriesDialog->exec() == QDialog::Accepted)
	{
		const bool recursive = directoriesDialog->getRecursive();
		const QStringList directories = directoriesDialog->getDirectories();

		m_abortFlag = false;
		setButtonsEnabled(false);

		ENABLE_MENU(ui->menuFile, false);
		ENABLE_MENU(ui->menuEdit, false);
		ENABLE_MENU(ui->menuHelp, false);

		UNSET_MODEL(ui->treeView);
		ui->label->setText(tr("Searching for files and directories, please be patient..."));
	
		m_signCompleted->hide();
		m_signCancelled->hide();
		m_signQuiescent->hide();

		ui->progressBar->setValue(0);
		ui->progressBar->setMaximum(0);

		Taskbar::setTaskbarState(this, Taskbar::TaskbarIndeterminateState);

		m_directoryScanner->setRecursive(recursive);
		m_directoryScanner->addDirectories(directories);
		m_directoryScanner->start();
	}

	MY_DELETE(directoriesDialog);
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
		ENABLE_MENU(ui->menuFile, true);
		ENABLE_MENU(ui->menuHelp, true);
		Taskbar::setTaskbarState(this, Taskbar::TaskbarErrorState);
		return;
	}
	
	Taskbar::setTaskbarState(this, Taskbar::TaskbarNormalState);
	Taskbar::setTaskbarProgress(this, 0, 100);

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
		ENABLE_MENU(ui->menuFile, true);
		ENABLE_MENU(ui->menuHelp, true);
		Taskbar::setTaskbarState(this, Taskbar::TaskbarErrorState);
		return;
	}

	m_abortFlag = true; /*to prevent an abort message after completion*/

	const QStringList &files = m_directoryScanner->getFiles();
	ui->label->setText(tr("Completed: %1 file(s) have been analyzed, %2 duplicate(s) have been identified.").arg(QString::number(files.count()), QString::number(m_model->duplicateCount())));

	if(m_model->duplicateCount() > 0)
	{
		UNSET_MODEL(ui->treeView);
		ui->treeView->setModel(m_model);
		ENABLE_MENU(ui->menuEdit, true);
	}
	else
	{
		m_signCompleted->show();
	}

	setButtonsEnabled(true);
	ENABLE_MENU(ui->menuFile, true);
	ENABLE_MENU(ui->menuHelp, true);

	QApplication::beep();
	ui->treeView->expandAll();
}

void MainWindow::fileComparatorProgressChanged(const int &progress)
{
	ui->progressBar->setValue(progress);
	Taskbar::setTaskbarProgress(this, progress, 100);
}

void MainWindow::itemActivated(const QModelIndex &index)
{
	ENSURE_APP_IS_IDLE();

	const QString &filePath = m_model->getFilePath(index);
	if(!filePath.isEmpty())
	{
		shellExplore((const wchar_t*)QDir::toNativeSeparators(filePath).utf16());
	}
}

void MainWindow::clearData(void)
{
	ENSURE_APP_IS_IDLE();

	if(ui->treeView->model() || m_signCancelled->isVisible() || m_signCompleted->isVisible())
	{
		UNSET_MODEL(ui->treeView);
		ENABLE_MENU(ui->menuEdit, false);
		m_model->clear();

		ui->progressBar->setMaximum(100);
		ui->progressBar->setValue(0);

		Taskbar::setTaskbarState(this, Taskbar::TaskbarNoState);

		m_signCompleted->hide();
		m_signCancelled->hide();
		m_signQuiescent->show();

		ui->label->setText(tr("Cleared. Please click \"Start Scan\" in order to begin a new scan!"));
	}
}

void MainWindow::exportToFile(void)
{
	ENSURE_APP_IS_IDLE();

	QMap<QString,int> filters;
	filters.insert(tr("XML File (*.xml)"), DuplicatesModel::FORMAT_XML);
	filters.insert(tr("INI File (*.ini)"), DuplicatesModel::FORMAT_INI);

	QString selectedFilter = filters.key(DuplicatesModel::FORMAT_XML);
	const QString outFile = QFileDialog::getSaveFileName(this, tr("Select Output File"), QDir::homePath(), ((QStringList)filters.keys()).join(";;"), &selectedFilter);

	if(!outFile.isEmpty())
	{
		if(m_model->exportToFile(outFile, filters.value(selectedFilter)))
		{
			QMessageBox::information(this, tr("Success"), tr("File has been saved successfully."));
		}
		else
		{
			QMessageBox::warning(this, tr("Failed"), tr("Whoops, failed to save file!"));
		}
	}
}

void MainWindow::copyToClipboard(void)
{
	ENSURE_APP_IS_IDLE();

	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(m_model->toString());
	QApplication::beep();
}

void MainWindow::showHomepage(void)
{
	ENSURE_APP_IS_IDLE();
	QDesktopServices::openUrl(QUrl(HOMEPAGE_URL));
}

void MainWindow::showAbout(void)
{
	ENSURE_APP_IS_IDLE();

	QString text;
	const QString tmplt = QString("<nobr><tt>%1</tt></nobr><br>");

	text += tmplt.arg(QString().sprintf("<b>Double File Scanner, Version %u.%02u-%u</b>", DOUBLESCANNER_VERSION_MAJOR, DOUBLESCANNER_VERSION_MINOR, DOUBLESCANNER_VERSION_PATCH));
	text += tmplt.arg("Copyright (c) 2014 LoRd_MuldeR &lt;mulder2@gmx.de&gt;. Some rights reserved.");
	text += tmplt.arg(QString().sprintf("Built on %s at %s with %s for Win-%s.\n", DOUBLESCANNER_BUILD_DATE, DOUBLESCANNER_BUILD_TIME, DOUBLESCANNER_COMPILER, DOUBLESCANNER_ARCH));
	text += "<br>";
	text += tmplt.arg("This program is free software: you can redistribute it and/or modify");
	text += tmplt.arg("it under the terms of the GNU General Public License <http://www.gnu.org/>.");
	text += tmplt.arg("Note that this program is distributed with ABSOLUTELY NO WARRANTY.\n");
	text += "<br>";
	text += tmplt.arg(QString("Please check <a href=\"%1\">%1</a> for news and updates!").arg(HOMEPAGE_URL));

	QMessageBox msgBox(this);
	msgBox.setWindowTitle(tr("About Double File Scanner"));
	msgBox.setText(text);
	msgBox.setIconPixmap(QPixmap(":/res/Logo.png"));
	msgBox.exec();

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
