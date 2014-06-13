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
}

//===================================================================
// Slots
//===================================================================

//===================================================================
// Private Functions
//===================================================================
