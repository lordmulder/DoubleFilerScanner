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

#pragma once

#include <QDialog>

//UIC forward declartion
namespace Ui {
	class DirectoriesDialog;
}

//DirectoriesDialog class
class DirectoriesDialog: public QDialog
{
	Q_OBJECT

public:
	DirectoriesDialog(QWidget *const parent);
	virtual ~DirectoriesDialog(void);

	QStringList getDirectories(void);
	bool getRecursive(void);

private slots:
	void addDirectory(void);
	void removeDirectory(void);
	void clearDirectories(void);

protected:
	virtual void showEvent(QShowEvent *e);

	QString m_lastPath;
	Ui::DirectoriesDialog *const ui;
};
