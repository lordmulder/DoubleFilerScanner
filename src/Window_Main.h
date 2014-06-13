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
	void clearData(void);
	void exportToFile(void);
	void copyToClipboard(void);
	void showHomepage(void);
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
