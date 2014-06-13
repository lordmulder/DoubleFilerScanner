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
}

DirectoriesDialog::~DirectoriesDialog(void)
{
	delete ui;
}

//===================================================================
// Events
//===================================================================

void DirectoriesDialog::showEvent(QShowEvent *e)
{
	resizeEvent(NULL);
	QTimer::singleShot(0, this, SLOT(addDirectory()));
}

//===================================================================
// Slots
//===================================================================

void DirectoriesDialog::addDirectory(void)
{
	const QString path = QFileDialog::getExistingDirectory(this, tr("Choose Directory"), m_lastPath);

	if(!path.isEmpty())
	{
		QListWidgetItem *item = new QListWidgetItem(QIcon(":/res/Icon_Duplicate.png"), path, ui->listWidget);
		ui->listWidget->addItem(item);
		
		QDir lastPath(path);
		lastPath.cdUp();
		m_lastPath = lastPath.path();
	}

	ui->buttonOkay->setEnabled(ui->listWidget->count() > 0);
}

void DirectoriesDialog::removeDirectory(void)
{
	qDeleteAll(ui->listWidget->selectedItems());
	ui->buttonOkay->setEnabled(ui->listWidget->count() > 0);
}

void DirectoriesDialog::clearDirectories(void)
{
	ui->listWidget->clear();
	ui->buttonOkay->setEnabled(ui->listWidget->count() > 0);
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

bool DirectoriesDialog::getRecursive(void)
{
	return ui->checkBoxRecursive->isChecked();
}
