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
#include "Window_Main.h"

//Initialize static Qt plugins
#ifdef QT_NODLL
Q_IMPORT_PLUGIN(qico)
Q_IMPORT_PLUGIN(qsvg)
Q_IMPORT_PLUGIN(qtga)
#endif

//Message handler routine
static void msg_handler(QtMsgType type, const char *msg)
{
	switch(type)
	{
	case QtWarningMsg:
		printConsole(msg, 1);
		break;
	case QtCriticalMsg:
	case QtFatalMsg:
		printConsole(msg, 2);
		break;
	default:
		printConsole(msg, 0);
		break;
	}
}

//Initialize Qt runtime
static QApplication *init_qt(int argc, char* argv[])
{
	qDebug("Using Qt v%s-%s [%s], compiled with Qt v%s [%s]\n", qVersion(), (qSharedBuild() ? "DLL" : "Static"), QLibraryInfo::buildDate().toString(Qt::ISODate).toLatin1().constData(), QT_VERSION_STR, QT_PACKAGEDATE_STR);
	
	//Create QApplication
	QApplication *application = new QApplication(argc, argv);

	//Setup library path
	QApplication::setLibraryPaths(QStringList() << QApplication::applicationDirPath());
	qDebug("Library Path:\n%s\n", QApplication::libraryPaths().first().toUtf8().constData());

	//Check image formats
	QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
	static const char *imageformats[] = {"bmp", "png", "jpg", "gif", "ico", "xpm", NULL};
	for(int i = 0; imageformats[i]; i++)
	{
		if(!supportedFormats.contains(imageformats[i]))
		{
			crashHandler("Qt initialization error: Failed to load image format plugins!");
		}
	}
	
	//Setup application
	application->setWindowIcon(QIcon(":/res/DoubleFileScanner.png"));
	application->setStyle(new QPlastiqueStyle());
	
	return application;
}

//Main function
static int double_file_scanner(int argc, char* argv[])
{
	qInstallMsgHandler(msg_handler);
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf-8"));

	qDebug("Double File Scanner, Version %u.%02u-%u", DOUBLESCANNER_VERSION_MAJOR, DOUBLESCANNER_VERSION_MINOR, DOUBLESCANNER_VERSION_PATCH);
	qDebug("Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.");
	qDebug("Built on %s at %s with %s for Win-%s.\n", DOUBLESCANNER_BUILD_DATE, DOUBLESCANNER_BUILD_TIME, DOUBLESCANNER_COMPILER, DOUBLESCANNER_ARCH);

	qDebug("This program is free software: you can redistribute it and/or modify");
	qDebug("it under the terms of the GNU General Public License <http://www.gnu.org/>.");
	qDebug("Note that this program is distributed with ABSOLUTELY NO WARRANTY.\n");
	
	//Create application
	QApplication *application = init_qt(argc, argv);

	//Create main window
	MainWindow *mainWindow = new MainWindow();
	mainWindow->show();

	//Begin event processing
	application->exec();

	//Free memory
	delete mainWindow;
	delete application;

	return EXIT_SUCCESS;
}

int main_ex(int argc, char* argv[])
{
	try
	{
		for(int i = 1; i < argc; i++)
		{
			if(_stricmp(argv[i], "--console") == 0)
			{
				initConsole();
				break;
			}
		}
		
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
	//return EXIT_FAILURE;
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
	//return EXIT_FAILURE;
}
