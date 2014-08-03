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
#include "Utilities.h"
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
#include <QElapsedTimer>
#include <QInputDialog>
#include <QDateTime>

#include <cassert>

static void UNSET_MODEL(QTreeView *view)
{
	QItemSelectionModel *selectionModel = view->selectionModel();
	view->setModel(NULL);
	MY_DELETE(selectionModel);
}

static void SETUP_MODEL(QTreeView *view, QAbstractItemModel *model)
{
	UNSET_MODEL(view);
	view->setModel(model);
	if(QHeaderView *header = view->header())
	{
		header->setResizeMode(0, QHeaderView::ResizeToContents);
		header->setResizeMode(1, QHeaderView::Stretch);
		header->setResizeMode(2, QHeaderView::ResizeToContents);
		header->setMovable(false);
		header->setClickable(false);
	}
	view->expandAll();
}

static void ENABLE_MENU(QMenu *menu, const bool &enabled)
{
	QList<QAction*> actions = menu->actions();
	for(QList<QAction*>::Iterator iter = actions.begin(); iter != actions.end(); iter++)
	{
		(*iter)->setEnabled(enabled);
	}
}

static bool DELETE_ALL_BUT_ONE(DuplicatesModel *model, const QModelIndex &group, qint64 *size)
{
	int fileCount, currentIndex;
	fileCount = currentIndex = model->rowCount(group);
	while((fileCount > 1) && (currentIndex > 0))
	{
		const QModelIndex fileIndex = model->index(--currentIndex, 0, group);
		if(fileIndex.isValid())
		{
			const qint64 currentSize = model->getFileSize(fileIndex);
			if(model->deleteFile(fileIndex))
			{
				if(size)
				{
					*size += currentSize;
				}
				fileCount--;
			}
		}
	}
	return (fileCount < 2);
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
	m_abortFlag = true;
	m_pauseFlag = m_unattendedFlag = false;
	
	//Determine threads count
	const int threadCount = qBound(0, getEnvString("DBLSCAN_THREADS").toInt(), 64);

	//Setup window flags
	setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMaximizeButtonHint);

	//Setup UI
	ui->setupUi(this);

	//Setup title
	setWindowTitle(tr("Double File Scanner %1").arg(QString().sprintf("v%u.%02u-%u", DOUBLESCANNER_VERSION_MAJOR, DOUBLESCANNER_VERSION_MINOR, DOUBLESCANNER_VERSION_PATCH)));

	//Setup window icon
	changeWindowIcon(this, QIcon(":/res/Logo.png"));

	//Setup button connections
	connect(ui->buttonStart, SIGNAL(clicked()), this, SLOT(startScan()));
	connect(ui->buttonAbout, SIGNAL(clicked()), this, SLOT(showAbout()));
	connect(ui->buttonExit,  SIGNAL(clicked()), this, SLOT(close()));

	//Setup menu connections
	connect(ui->actionStart,     SIGNAL(triggered()), this, SLOT(startScan()));
	connect(ui->actionClear,     SIGNAL(triggered()), this, SLOT(clearData()));
	connect(ui->actionAutoClean, SIGNAL(triggered()), this, SLOT(autoClean()));
	connect(ui->actionExit,      SIGNAL(triggered()), this, SLOT(close()));
	connect(ui->actionOpen,      SIGNAL(triggered()), this, SLOT(openFile()));
	connect(ui->actionGoto,      SIGNAL(triggered()), this, SLOT(gotoFile()));
	connect(ui->actionRename,    SIGNAL(triggered()), this, SLOT(renameFile()));
	connect(ui->actionDelete,    SIGNAL(triggered()), this, SLOT(deleteFile()));
	connect(ui->actionClipbrd,   SIGNAL(triggered()), this, SLOT(copyToClipboard()));
	connect(ui->actionExport,    SIGNAL(triggered()), this, SLOT(exportToFile()));
	connect(ui->actionHomepage,  SIGNAL(triggered()), this, SLOT(showHomepage()));
	connect(ui->actionAbout,     SIGNAL(triggered()), this, SLOT(showAbout()));
	
	//Create model
	m_model = new DuplicatesModel();

	//Create directory scanner
	m_directoryScanner = new DirectoryScanner(&m_abortFlag, threadCount);
	connect(m_directoryScanner, SIGNAL(finished()), this, SLOT(directoryScannerFinished()), Qt::QueuedConnection);

	//Create file comparator
	m_fileComparator = new FileComparator(&m_abortFlag, threadCount);
	connect(m_fileComparator, SIGNAL(finished()), this, SLOT(fileComparatorFinished()), Qt::QueuedConnection);
	connect(m_fileComparator, SIGNAL(progressChanged(int)), this, SLOT(fileComparatorProgressChanged(int)), Qt::QueuedConnection);
	connect(m_fileComparator, SIGNAL(duplicateFound(const QByteArray&, const QStringList&, const qint64&)), m_model, SLOT(addDuplicate(const QByteArray, const QStringList, const qint64&)), Qt::BlockingQueuedConnection);

	//Setup tree view
	ui->treeView->setExpandsOnDoubleClick(false);
	connect(ui->treeView, SIGNAL(activated(QModelIndex)), this, SLOT(openFile(QModelIndex)));

	//Setup animator
	m_movie = new QMovie(":/res/Spinner.gif");
	m_animator = makeLabel(ui->treeView, ":/res/Spinner.gif");
	m_animator->setMovie(m_movie);
	m_animator->setToolTip(tr("You can press ESC to abort a running operation..."));

	//Create signs
	m_signQuiescent = makeLabel(ui->treeView, ":/res/Sign_Clocks.png", 0);
	m_signCompleted = makeLabel(ui->treeView, ":/res/Sign_Accept.png", 1);
	m_signCancelled = makeLabel(ui->treeView, ":/res/Sign_Cancel.png", 1);

	//Create context menu
	ui->treeView->addActions(ui->menuEdit->actions());
	ui->treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
	
	//Disable menu items initially
	setMenuItemsEnabled(false);

	//Create timer
	m_timer = new QElapsedTimer();

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
	MY_DELETE(m_timer);
}

//===================================================================
// Events
//===================================================================

void MainWindow::showEvent(QShowEvent *e)
{
	QMainWindow::showEvent(e);
	resizeEvent(NULL);
	handleCommandLineArgs();
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
			m_directoryScanner->suspend(false);
			m_fileComparator  ->suspend(false);
		}
	}
	else if(e->key() == Qt::Key_Pause)
	{
		if(!m_abortFlag)
		{
			togglePause();
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

	bool recursive = true;
	QStringList directories;

	if(m_unattendedFlag)
	{
		m_unattendedFlag = false;
		directories << m_droppedFolders;
		m_droppedFolders.clear();
	}
	else
	{
		DirectoriesDialog *directoriesDialog = new DirectoriesDialog(this);

		if(!m_droppedFolders.isEmpty())
		{
			directoriesDialog->addDirectories(m_droppedFolders);
			m_droppedFolders.clear();
		}

		if(directoriesDialog->exec() == QDialog::Accepted)
		{
			recursive = directoriesDialog->getRecursive();
			directories << directoriesDialog->getDirectories();
		}

		MY_DELETE(directoriesDialog);
	}

	if(!directories.isEmpty())
	{
		setButtonsEnabled(false);
		setMenuItemsEnabled(false);
		
		m_abortFlag = false;
		m_pauseFlag = false;

		UNSET_MODEL(ui->treeView);
		ui->label->setText(tr("Searching for files and directories, please be patient..."));
		m_model->clear();

		showSign(-1);
		updateProgress(-1);

		m_directoryScanner->setRecursive(recursive);
		m_directoryScanner->addDirectories(directories);
		m_directoryScanner->suspend(false);
		m_directoryScanner->start();

		m_timer->start();
	}
}

void MainWindow::directoryScannerFinished(void)
{
	updateProgress(0);

	if(m_abortFlag)
	{
		m_timer->invalidate();
		QApplication::beep();
		ui->label->setText(tr("The operation has been aborted by the user!"));
		showSign(2);
		setButtonsEnabled(true);
		Taskbar::setTaskbarState(this, Taskbar::TaskbarErrorState);
		return;
	}

	const QStringList &files = m_directoryScanner->getFiles();
	ui->label->setText(tr("%1 file(s) are being analyzed, this might take a few minutes...").arg(QString::number(files.count())));

	m_fileComparator->addFiles(files);
	m_fileComparator->suspend(false);
	m_fileComparator->start();
}

void MainWindow::fileComparatorFinished(void)
{
	if(m_abortFlag)
	{
		m_timer->invalidate();
		QApplication::beep();
		ui->label->setText(tr("The operation has been aborted by the user!"));
		showSign(2);
		setButtonsEnabled(true);
		Taskbar::setTaskbarState(this, Taskbar::TaskbarErrorState);
		return;
	}
	
	m_abortFlag = true; /*to prevent an abort message after completion*/

	if(m_timer->isValid())
	{
		const quint64 elapsed = m_timer->elapsed();
		m_timer->invalidate();
		qDebug("Operation took %.3f seconds to complete.\n", double(elapsed) / 1000.0);
	}

	const QStringList &files = m_directoryScanner->getFiles();
	ui->label->setText(tr("Completed: %1 file(s) have been analyzed, %2 duplicate(s) have been identified.").arg(QString::number(files.count()), QString::number(m_model->duplicateCount())));

	if(m_model->duplicateCount() > 0)
	{
		SETUP_MODEL(ui->treeView, m_model);
		setMenuItemsEnabled(true);
	}
	else
	{
		showSign(1);
	}

	setButtonsEnabled(true);
	QApplication::beep();
}

void MainWindow::fileComparatorProgressChanged(const int &progress)
{
	updateProgress(progress);
	Taskbar::setTaskbarProgress(this, progress, 100);
}

void MainWindow::openFile(void)
{
	ENSURE_APP_IS_IDLE();
	QModelIndex selected = getSelectedItem();
	
	if(!selected.isValid())
	{
		QMessageBox::warning(this, tr("Warning"), tr("Sorry, no file is currently selected!"));
		return;
	}

	openFile(selected);
}

void MainWindow::openFile(const QModelIndex &index)
{
	ENSURE_APP_IS_IDLE();

	const QString &filePath = m_model->getFilePath(index);
	if(!filePath.isEmpty())
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
	}
	else
	{
		QMessageBox::warning(this, tr("Warning"), tr("Sorry, selected item doesn't look like a file!"));
	}
}

void MainWindow::gotoFile(void)
{
	ENSURE_APP_IS_IDLE();
	QModelIndex selected = getSelectedItem();
	
	if(!selected.isValid())
	{
		QMessageBox::warning(this, tr("Warning"), tr("Sorry, no file is currently selected!"));
		return;
	}

	gotoFile(selected);
}

void MainWindow::gotoFile(const QModelIndex &index)
{
	ENSURE_APP_IS_IDLE();

	const QString &filePath = m_model->getFilePath(index);
	if(!filePath.isEmpty())
	{
		if(QFileInfo(filePath).exists())
		{
			shellExplore((const wchar_t*)QDir::toNativeSeparators(filePath).utf16());
		}
		else
		{
			QMessageBox::warning(this, tr("Warning"), tr("Sorry, selected file no longer exists!"));
		}
	}
	else
	{
		QMessageBox::warning(this, tr("Warning"), tr("Sorry, selected item doesn't look like a file!"));
	}
}

void MainWindow::renameFile(void)
{
	ENSURE_APP_IS_IDLE();
	QModelIndex selected = getSelectedItem();
	
	if(!selected.isValid())
	{
		QMessageBox::warning(this, tr("Warning"), tr("Sorry, no file is currently selected!"));
		return;
	}

	renameFile(selected);
}

void MainWindow::renameFile(const QModelIndex &index)
{
	ENSURE_APP_IS_IDLE();

	const QString &filePath = m_model->getFilePath(index);
	if(!filePath.isEmpty())
	{
		if(QFileInfo(filePath).exists() && QFileInfo(filePath).isFile())
		{
			QString targetName = QFileInfo(filePath).fileName();
			
			forever
			{
				bool ok = false;
				const QString temp = QInputDialog::getText(this, tr("Rename File"), tr("Please enter new file name:").leftJustified(128), QLineEdit::Normal, targetName, &ok);
				if(!ok)
				{
					return; /*aborted by user*/
				}
				targetName = cleanFileName(temp.simplified());
				if((!targetName.isEmpty()) && (targetName.compare(temp) == 0))
				{
					break; /*file name is valid*/
				}
				if(targetName.isEmpty())
				{
					targetName = QFileInfo(filePath).fileName();
				}
				QApplication::beep();
			}

			if(!m_model->renameFile(index, targetName))
			{
				QMessageBox::warning(this, tr("Warning"), tr("Sorry, failed to rename the selected file!"));
			}
		}
		else
		{
			QMessageBox::warning(this, tr("Warning"), tr("Sorry, the selected file doesn't exist anymore!"));
		}
	}
	else
	{
		QMessageBox::warning(this, tr("Warning"), tr("Sorry, selected item doesn't look like a file!"));
	}
}

void MainWindow::deleteFile(void)
{
	ENSURE_APP_IS_IDLE();
	QModelIndex selected = getSelectedItem();
	
	if(!selected.isValid())
	{
		QMessageBox::warning(this, tr("Warning"), tr("Sorry, no file is currently selected!"));
		return;
	}

	deleteFile(selected);
}

void MainWindow::deleteFile(const QModelIndex &index)
{
	ENSURE_APP_IS_IDLE();

	const QString &filePath = m_model->getFilePath(index);
	if(!filePath.isEmpty())
	{
		const QString text = QString("<nobr>%1</nobr><br><br><tt style=\"white-space:pre-wrap\">%2</tt>").arg(tr("Do you really want to permanently delete the selected file?"), QDir::toNativeSeparators(filePath));
		if(QMessageBox::question(this, tr("Delete File"), text, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
		{
			if(m_model->duplicateFileCount(index) < 2)
			{
				if(QMessageBox::warning(this, tr("Delete File"), tr("This file has no more duplicates left! Delete anyway?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
				{
					return;
				}
			}

			if(!m_model->deleteFile(index))
			{
				QMessageBox::warning(this, tr("Warning"), tr("Sorry, failed to delete the selected file!"));
			}
		}
	}
	else
	{
		QMessageBox::warning(this, tr("Warning"), tr("Sorry, selected item doesn't look like a file!"));
	}
}

void MainWindow::clearData(void)
{
	ENSURE_APP_IS_IDLE();

	if(ui->treeView->model() || m_signCancelled->isVisible() || m_signCompleted->isVisible())
	{
		UNSET_MODEL(ui->treeView);
		setMenuItemsEnabled(false);
		m_model->clear();

		updateProgress(0);
		Taskbar::setTaskbarState(this, Taskbar::TaskbarNoState);
		showSign(0);

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

void MainWindow::autoClean(void)
{
	ENSURE_APP_IS_IDLE();

	if(!(ui->treeView->model() && (ui->treeView->model()->rowCount() > 0)))
	{
		qWarning("Cannot perform clean up at this moment!");
		return;
	}

	if(QMessageBox::warning(this, tr("Automatic Clean-up"), tr("<nobr>This is going to delete all files but one for each duplicates group.</nobr><br><nobr>Files will be deleted permanently! Do you really want to contine?</nobr>"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
	{
		return;
	}

	setButtonsEnabled(false);
	setMenuItemsEnabled(false);
	m_abortFlag = false;

	UNSET_MODEL(ui->treeView);
	ui->label->setText(tr("Automatic clean-up operation is in progress, please be patient..."));
	showSign(-1);

	qint64 spaceSaved = 0;
	const int groupCount = m_model->rowCount();
	qDebug("[Automatic Clean-Up]");

	for(int i = 0; i < groupCount; i++)
	{
		updateProgress(i, groupCount);
		QApplication::processEvents();
		const QModelIndex currentGroup = m_model->index(i, 0);
		if(currentGroup.isValid())
		{
			qDebug("Deleting duplicates for %s", m_model->getGroupHash(currentGroup).toHex().constData());
			if(!DELETE_ALL_BUT_ONE(m_model, currentGroup, &spaceSaved))
			{
				qWarning("Failed to clean-up current duplicates group! (row %d)", i);
			}
		}
		if(m_abortFlag)
		{
			qWarning("Operation cancelled by user!");
			break;
		}
	}

	qDebug("Clean-up is complete.\n");

	setMenuItemsEnabled(true);
	setButtonsEnabled(true);

	if(m_abortFlag)
	{
		ui->label->setText(tr("The operation has been aborted by the user!"));
	}
	else
	{
		updateProgress(100);
		ui->label->setText(tr("Automatic clean-up operation completed: Saved %1 of disk space.").arg(Utilities::sizeToString(spaceSaved)));
		ui->actionAutoClean->setEnabled(false);
	}

	m_abortFlag = true;
	SETUP_MODEL(ui->treeView, m_model);
	QApplication::beep();
}

void MainWindow::showAbout(void)
{
	ENSURE_APP_IS_IDLE();
	const int year = qMax(QString::fromLatin1(&DOUBLESCANNER_BUILD_DATE[7]).toInt(), QDateTime::currentDateTime().date().year());

	QString text;
	const QString tmpl1 = QString("<nobr><b><tt>%1</tt></b></nobr><br>");
	const QString tmpl2 = QString("<nobr><tt>%1</tt></nobr><br>");

	text += tmpl1.arg(tr("<b>Double File Scanner, Version %1</b>").arg(QString().sprintf("%u.%02u-%u", DOUBLESCANNER_VERSION_MAJOR, DOUBLESCANNER_VERSION_MINOR, DOUBLESCANNER_VERSION_PATCH)));
	text += tmpl1.arg(tr("<b>Copyright (c) %1 LoRd_MuldeR &lt;mulder2@gmx.de&gt;. Some rights reserved.</b>").arg((year > 2014) ? QString().sprintf("%02d-%02d", 2014, year) : QString::number(2014)));
	text += tmpl1.arg(tr("<b>Built on %1 at %2 with %3 for Win-%4.</b>").arg(DOUBLESCANNER_BUILD_DATE, DOUBLESCANNER_BUILD_TIME, DOUBLESCANNER_COMPILER, DOUBLESCANNER_ARCH));
	text += "<hr><br>";
	text += tmpl2.arg("This program is free software; you can redistribute it and/or");
	text += tmpl2.arg("modify it under the terms of the GNU General Public License");
	text += tmpl2.arg("as published by the Free Software Foundation; either version 2");
	text += tmpl2.arg("of the License, or (at your option) any later version.");
	text += "<br>";
	text += tmpl2.arg("This program is distributed in the hope that it will be useful,");
	text += tmpl2.arg("but WITHOUT ANY WARRANTY; without even the implied warranty of");
	text += tmpl2.arg("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
	text += tmpl2.arg("GNU General Public License for more details.");
	text += "<br>";
	text += tmpl2.arg("You should have received a copy of the GNU General Public License");
	text += tmpl2.arg("along with this program; if not, write to the Free Software");
	text += tmpl2.arg("Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.");
	text += "<hr><br>";
	text += tmpl2.arg(tr("Please remember to check <a href=\"%1\">%1</a> for news and updates!").arg(HOMEPAGE_URL));

	QMessageBox msgBox(this);
	msgBox.setWindowTitle(tr("About Double File Scanner"));
	msgBox.setText(text);
	msgBox.setIconPixmap(QPixmap(":/res/Logo.png"));

	QAbstractButton *btnAccept = msgBox.addButton(tr("About Qt"), QMessageBox::AcceptRole);
	QAbstractButton *btnCancel = msgBox.addButton(tr("Discard"),  QMessageBox::RejectRole);
	btnAccept->setMinimumWidth(90);
	btnCancel->setMinimumWidth(90);
	btnAccept->setIcon(QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"));
	btnCancel->setIcon(QIcon(":/res/Button_Cancel.png"));

	msgBox.exec();
	if(msgBox.clickedButton() == btnAccept)
	{
		QMessageBox::aboutQt(this);
	}
}

//===================================================================
// Private Functions
//===================================================================

void MainWindow::setButtonsEnabled(const bool &enabled)
{
	ui->buttonStart->setEnabled(enabled);
	ui->buttonExit->setEnabled(enabled);
	ui->buttonAbout->setEnabled(enabled);
	ui->menuBar->setEnabled(enabled);

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

		QApplication::restoreOverrideCursor();
	}
}

void MainWindow::setMenuItemsEnabled(const bool &enabled)
{
	ENABLE_MENU(ui->menuEdit, enabled);
	ui->actionAutoClean->setEnabled(enabled);
	ui->actionExport->setEnabled(enabled);
	ui->actionClipbrd->setEnabled(enabled);
	ui->actionClear->setEnabled(enabled);
}

void MainWindow::updateProgress(const int &progress, const int &maxValue)
{
	if(progress >= 0)
	{
		ui->progressBar->setMaximum(maxValue);
		ui->progressBar->setValue(qMin(progress, maxValue));
		Taskbar::setTaskbarState(this, Taskbar::TaskbarNormalState);
		Taskbar::setTaskbarProgress(this, qMin(progress, maxValue), maxValue);
	}
	else
	{
		ui->progressBar->setMaximum(0);
		ui->progressBar->setValue(0);
		Taskbar::setTaskbarState(this, Taskbar::TaskbarIndeterminateState);
	}
}

void MainWindow::showSign(const int &id)
{
	m_signQuiescent->setVisible(id == 0);
	m_signCompleted->setVisible(id == 1);
	m_signCancelled->setVisible(id == 2);
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

void MainWindow::handleCommandLineArgs(void)
{
	m_droppedFolders.clear();
	const QStringList args = QApplication::arguments();
	bool appendNext = false;

	for(QStringList::ConstIterator iter = args.constBegin(); iter != args.constEnd(); iter++)
	{
		if(appendNext)
		{
			QFileInfo folder(QDir::fromNativeSeparators(*iter));
			if(folder.exists() && folder.isDir())
			{
				m_droppedFolders << folder.canonicalFilePath();
			}
			appendNext = false;
		}
		else if((*iter).compare("--scan", Qt::CaseInsensitive) == 0)
		{
			appendNext = true;
		}
	}

	if(!m_droppedFolders.isEmpty())
	{
		m_unattendedFlag = true;
		QTimer::singleShot(100, this, SLOT(startScan()));
	}
}

QModelIndex MainWindow::getSelectedItem(void)
{
	if(QItemSelectionModel *model = ui->treeView->selectionModel())
	{
		QModelIndexList selected = ui->treeView->selectionModel()->selectedIndexes();
		if(!selected.isEmpty())
		{
			return selected.first();
		}
	}

	return QModelIndex();
}

QString MainWindow::cleanFileName(const QString &fileName)
{
	QRegExp invalidChars("(\\\\|/|:|\\*|\\?|\"|<|>|\\|)");
	return QString(fileName).replace(invalidChars, "_");
}

void MainWindow::togglePause(void)
{
	m_pauseFlag = (!m_pauseFlag);
	if(m_pauseFlag)
	{
		qWarning("Operation has been suspende by user.");
		m_unpauseText = ui->label->text();
		ui->label->setText(tr("Operation has been suspended by user. Press 'pause' to proceed!"));
		m_movie->stop();
	}
	else
	{
		qWarning("Operation has been resumed by user.");
		ui->label->setText(m_unpauseText);
		m_movie->start();
	}
	m_directoryScanner->suspend(m_pauseFlag);
	m_fileComparator  ->suspend(m_pauseFlag);
}
