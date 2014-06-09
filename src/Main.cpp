// ConsoleApplication1.cpp : Defines the entry point for the console application.

#include <QApplication>

#include "DirectoryScanner.h"
#include "FileComparator.h"

int main(int argc, char* argv[])
{
	qDebug("DoubleFileScanner [%s]", __DATE__);
	qDebug("Copyright (C) 2014 LoRd_MuldeR <mulder2@gmx.de>.");
	qDebug("All rights reserved.\n");

	QApplication application(argc, argv);
	DirectoryScanner *scanner = new DirectoryScanner("D:/Gimp-2.8");
	
	scanner->start();
	scanner->wait();

	const QStringList &files = scanner->getFiles();
	FileComparator *comparator = new FileComparator(files);
	delete scanner;

	comparator->start();
	comparator->wait();

	delete comparator;
	
	qDebug("COMPLETED.");

	return application.exec();
}
