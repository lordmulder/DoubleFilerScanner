// ConsoleApplication1.cpp : Defines the entry point for the console application.

#include <QApplication>
#include <QPlastiqueStyle>
#include <QTextCodec>
#include <QFileDialog>
#include <QProgressDialog>
#include <QMessageBox>
#include <QCloseEvent>

#include "System.h"
#include "DirectoryScanner.h"
#include "FileComparator.h"

class MyProgressDialog : public QProgressDialog
{
public:
	MyProgressDialog(void) : QProgressDialog()
	{
		setWindowFlags(Qt::WindowStaysOnTopHint);
		setWindowTitle("Double File Scanner");
		setLabelText("Scanning for duplicate files, please be patient...");
		setFixedSize(512, 72);
		setMinimum(0);
		setMaximum(0);
		setCancelButton(NULL);
	}

protected:
	virtual void closeEvent(QCloseEvent *e)
	{
		e->ignore();
	}
};

int main(int argc, char* argv[])
{
	initializeConsole(QString("Double File Scanner [%1]").arg(__DATE__));
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf-8"));

	qDebug("Double File Scanner [%s]", __DATE__);
	qDebug("Copyright (C) 2014 LoRd_MuldeR <mulder2@gmx.de>.");
	qDebug("All rights reserved.\n");

	QApplication application(argc, argv);
	application.setStyle(new QPlastiqueStyle());
	
	//------------------------------------------------------------------------
	// Select Directory
	//------------------------------------------------------------------------
	
	const QString baseDirectory = QFileDialog::getExistingDirectory();
	
	if(baseDirectory.isEmpty())
	{
		qDebug("Aborted by user, exiting!");
		return EXIT_FAILURE;
	}

	qDebug("Selected directory is:\n%s\n", baseDirectory.toUtf8().constData());

	//------------------------------------------------------------------------
	// Create Progress Dialog
	//------------------------------------------------------------------------

	MyProgressDialog *progressDialog = new MyProgressDialog();
	QEventLoop progressLoop;

	//------------------------------------------------------------------------
	// Scan Directory
	//------------------------------------------------------------------------

	DirectoryScanner *scanner = new DirectoryScanner(baseDirectory);
	QObject::connect(scanner, SIGNAL(finished()), &progressLoop, SLOT(quit()), Qt::QueuedConnection);

	scanner->start();
	while(!scanner->wait(500))
	{
		progressDialog->show();
		progressLoop.exec();
	}

	//------------------------------------------------------------------------
	// Analyze Files
	//------------------------------------------------------------------------

	const QStringList &files = scanner->getFiles();
	FileComparator *comparator = new FileComparator(files);
	QObject::connect(comparator, SIGNAL(finished()), &progressLoop, SLOT(quit()), Qt::QueuedConnection);
	
	comparator->start();
	while(!comparator->wait(500))
	{
		progressDialog->show();
		progressLoop.exec();
	}

	//------------------------------------------------------------------------
	// Show Duplicates
	//------------------------------------------------------------------------

	const QHash<QByteArray, QStringList> &duplicates = comparator->getDuplicates();

	if(!duplicates.empty())
	{
		qDebug("[Duplicate Files]");
		for(QHash<QByteArray, QStringList>::ConstIterator iter = duplicates.constBegin(); iter != duplicates.constEnd(); iter++)
		{
			qDebug("%s", iter.key().toHex().constData());
			const QStringList fileNames = iter.value();
			for(QStringList::ConstIterator fileName = fileNames.constBegin(); fileName != fileNames.constEnd(); fileName++)
			{
				qDebug("+ %s", fileName->toUtf8().constData());
			}
		}
		qDebug("");
	}

	progressDialog->hide();
	qDebug("COMPLETED.");

	if(!duplicates.empty())
	{
		QMessageBox::warning(NULL, QString("Double File Scanner"), QString("Completed: %1 duplicate file(s) have been identified!").arg(QString::number(duplicates.count())));
	}
	else
	{
		QMessageBox::information(NULL, QString("Double File Scanner"), QString("Completed: There are no duplicate files in this directory."));
	}

	//------------------------------------------------------------------------
	// Clean Up
	//------------------------------------------------------------------------

	delete progressDialog;
	delete scanner;
	delete comparator;

	return EXIT_SUCCESS;
}
