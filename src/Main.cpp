// ConsoleApplication1.cpp : Defines the entry point for the console application.

#include <QApplication>
#include <QLibraryInfo>
#include <QPlastiqueStyle>
#include <QTextCodec>
#include <QFileDialog>
#include <QProgressDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QImageReader>

#include "Config.h"
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

static QApplication *init_qt(int argc, char* argv[])
{
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf-8"));
	qDebug("Using Qt v%s-%s [%s], compiled with Qt v%s [%s]\n", qVersion(), (qSharedBuild() ? "DLL" : "Static"), QLibraryInfo::buildDate().toString(Qt::ISODate).toLatin1().constData(), QT_VERSION_STR, QT_PACKAGEDATE_STR);

	QApplication *application = new QApplication(argc, argv);

	QApplication::setLibraryPaths(QStringList() << QApplication::applicationDirPath());
	qDebug("Library Path:\n%s\n", QApplication::libraryPaths().first().toUtf8().constData());

	QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
	static const char *imageformats[] = {"bmp", "png", "jpg", "gif", "ico", "xpm", NULL};
	for(int i = 0; imageformats[i]; i++)
	{
		if(!supportedFormats.contains(imageformats[i]))
		{
			qFatal("Qt initialization error: QImageIOHandler for '%s' missing!", imageformats[i]);
			return false;
		}
	}
	
	application->setWindowIcon(QIcon(":/res/DoubleFileScanner.png"));
	application->setStyle(new QPlastiqueStyle());
	
	return application;
}

static int double_file_scanner(int argc, char* argv[])
{
	qDebug("Double File Scanner, Version %u.%02u-%u", DOUBLESCANNER_VERSION_MAJOR, DOUBLESCANNER_VERSION_MINOR, DOUBLESCANNER_VERSION_PATCH);
	qDebug("Copyright (c) 2004-2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.");
	qDebug("Built on %s at %s with %s for %s.\n", DOUBLESCANNER_BUILD_DATE, DOUBLESCANNER_BUILD_TIME, DOUBLESCANNER_COMPILER, DOUBLESCANNER_ARCH);

	qDebug("This program is free software: you can redistribute it and/or modify");
	qDebug("it under the terms of the GNU General Public License <http://www.gnu.org/>.");
	qDebug("Note that this program is distributed with ABSOLUTELY NO WARRANTY.\n");
	
	QApplication *application = init_qt(argc, argv);
	
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

int main_ex(int argc, char* argv[])
{
	try
	{
		initConsole();
		return double_file_scanner(argc, argv);
	}
	catch(std::exception &e)
	{
		fprintf(stderr, "\nGURU MEDITATION !!!\n\nUnhandeled C++ exception error: %s\n\n", e.what());
		crashHandler(e.what());
	}
	catch(...)
	{
		fprintf(stderr, "\nGURU MEDITATION !!!\n\nUnhandeled unknown C++ exception error!\n\n");
		crashHandler("Unhandeled unknown C++ exception, application will exit!");
	}
}

int main(int argc, char* argv[])
{
	if(DOUBLESCANNER_DEBUG)
	{
		initConsole();
		return double_file_scanner(argc, argv);
	}
	else
	{
		__try
		{
			initErrorHandlers();
			return main_ex(argc, argv);
		}
		__except(1)
		{
			fprintf(stderr, "\nGURU MEDITATION !!!\n\nUnhandeled structured exception error!\n\n");
			crashHandler("Unhandeled structured exception, application will exit!");
		}
	}
}
