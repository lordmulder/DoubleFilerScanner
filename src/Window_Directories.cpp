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

#include "Window_Directories.h"
#include "UIC_Window_Directories.h"

#include "Config.h"
#include "System.h"

#include <QDir>
#include <QFileDialog>
#include <QTimer>
#include <QLabel>
#include <QMessageBox>
#include <QDropEvent>
#include <QUrl>

#include <cassert>

//===================================================================
// Constructor & Destructor
//===================================================================

DirectoriesDialog::DirectoriesDialog(QWidget *const parent)
:
	QDialog(parent),
	ui(new Ui::DirectoriesDialog())
{
	//Setup window flags
	setWindowFlags(windowFlags() | Qt::Tool);

	//Setup UI
	ui->setupUi(this);

	//Setup size
	setMinimumSize(size());

	//Setup connections
	connect(ui->buttonAddDir,    SIGNAL(clicked()), this, SLOT(addDirectory()));
	connect(ui->buttonClear,     SIGNAL(clicked()), this, SLOT(clearDirectories()));
	connect(ui->buttonRemoveDir, SIGNAL(clicked()), this, SLOT(removeDirectory()));

	//Disable button initially
	ui->buttonOkay->setEnabled(false);

	//Set initial path
	m_lastPath = QDir::homePath();

	//Create label
	m_label = new QLabel(ui->listWidget);
	m_label->setText(tr("Please add at least one directory..."));
	m_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	

	//Configure font
	QFont font = m_label->font();
	font.setBold(true);
	m_label->setFont(font);

	//Configure color
	QPalette palette = m_label->palette();
	palette.setColor(QPalette::WindowText, QColor(Qt::darkGray));
	m_label->setPalette(palette);

	//Enable drag&drop support
	setAcceptDrops(true);
}

DirectoriesDialog::~DirectoriesDialog(void)
{
	MY_DELETE(m_label);
	delete ui;
}

//===================================================================
// Events
//===================================================================

void DirectoriesDialog::showEvent(QShowEvent *e)
{
	QDialog::showEvent(e);
	resizeEvent(NULL);

	if(ui->listWidget->count() < 1)
	{
		QTimer::singleShot(0, this, SLOT(addDirectory()));
	}
}

void DirectoriesDialog::resizeEvent(QResizeEvent *e)
{
	QDialog::resizeEvent(e);
	m_label->resize(ui->listWidget->viewport()->size());
}

void DirectoriesDialog::dragEnterEvent(QDragEnterEvent *e)
{
	QStringList formats = e->mimeData()->formats();
	
	if(formats.contains("application/x-qt-windows-mime;value=\"FileNameW\"", Qt::CaseInsensitive) && formats.contains("text/uri-list", Qt::CaseInsensitive))
	{
		e->acceptProposedAction();
	}
}

void DirectoriesDialog::dropEvent(QDropEvent *e)
{
	QStringList droppedFolders;
	QList<QUrl> urls = e->mimeData()->urls();

	for(QList<QUrl>::ConstIterator iter = urls.constBegin(); iter != urls.constEnd(); iter++)
	{
		QFileInfo item(iter->toLocalFile());
		if(item.exists() && item.isDir())
		{
			droppedFolders << item.canonicalFilePath();
		}
	}

	if(!droppedFolders.isEmpty())
	{
		addDirectories(droppedFolders);
	}
}

//===================================================================
// Slots
//===================================================================

void DirectoriesDialog::addDirectory(void)
{
	const QString path = QFileDialog::getExistingDirectory(this, tr("Choose Directory"), m_lastPath);

	if(!path.isEmpty())
	{
		for(int i = 0; i < ui->listWidget->count(); i++)
		{
			if(QDir::fromNativeSeparators(path).compare(QDir::fromNativeSeparators(ui->listWidget->item(i)->text()), Qt::CaseInsensitive) == 0)
			{
				QMessageBox::warning(this, tr("Warning"), tr("The selected directory is already on the list!"));
				return;
			}
		}

		QListWidgetItem *item = new QListWidgetItem(QIcon(":/res/Icon_Folder.png"), QDir::toNativeSeparators(path), ui->listWidget);
		ui->listWidget->addItem(item);
		
		QDir lastPath(path);
		lastPath.cdUp();
		m_lastPath = lastPath.path();

		ui->buttonOkay->setEnabled(ui->listWidget->count() > 0);
		m_label->setVisible(ui->listWidget->count() < 1);
	}
}

void DirectoriesDialog::removeDirectory(void)
{
	qDeleteAll(ui->listWidget->selectedItems());
	ui->buttonOkay->setEnabled(ui->listWidget->count() > 0);
	m_label->setVisible(ui->listWidget->count() < 1);
}

void DirectoriesDialog::clearDirectories(void)
{
	ui->listWidget->clear();
	ui->buttonOkay->setEnabled(ui->listWidget->count() > 0);
	m_label->setVisible(ui->listWidget->count() < 1);
}

//===================================================================
// Public Functions
//===================================================================

QStringList DirectoriesDialog::getDirectories(void)
{
	QStringList directories;

	for(int i = 0; i < ui->listWidget->count(); i++)
	{
		directories << QDir::fromNativeSeparators(ui->listWidget->item(i)->text());
	}
	
	return directories;
}

void DirectoriesDialog::addDirectories(const QStringList &directories)
{
	for(QStringList::ConstIterator iter = directories.constBegin(); iter != directories.constEnd(); iter++)
	{
		bool bSkipFolder = false;

		for(int i = 0; i < ui->listWidget->count(); i++)
		{
			if(QDir::fromNativeSeparators(*iter).compare(QDir::fromNativeSeparators(ui->listWidget->item(i)->text()), Qt::CaseInsensitive) == 0)
			{
				bSkipFolder = true;
				break;
			}
		}

		if(!bSkipFolder)
		{
			QListWidgetItem *item = new QListWidgetItem(QIcon(":/res/Icon_Folder.png"), QDir::toNativeSeparators(*iter), ui->listWidget);
			ui->listWidget->addItem(item);
		}
	}

	ui->buttonOkay->setEnabled(ui->listWidget->count() > 0);
	m_label->setVisible(ui->listWidget->count() < 1);
}

bool DirectoriesDialog::getRecursive(void)
{
	return ui->checkBoxRecursive->isChecked();
}
