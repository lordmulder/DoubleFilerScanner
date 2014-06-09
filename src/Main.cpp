// ConsoleApplication1.cpp : Defines the entry point for the console application.

#include <QApplication>

#include "DirectoryScanner.h"

int main(int argc, char* argv[])
{
	qDebug("DoubleFileScanner [%s]", __DATE__);
	qDebug("Copyright (C) 2014 LoRd_MuldeR <mulder2@gmx.de>.");
	qDebug("All rights reserved.\n");

	QApplication application(argc, argv);
	
	DirectoryScanner *scanner = new DirectoryScanner("C:/");
	scanner->moveToThread(scanner);
	
	scanner->start();
	scanner->wait();

	const QStringList &files = scanner->getFiles();
	for(QStringList::ConstIterator iter = files.constBegin(); iter != files.constEnd(); iter++)
	{
		qDebug("File: %s", iter->toUtf8().constData());
	}

	delete scanner;
	return application.exec();
}
