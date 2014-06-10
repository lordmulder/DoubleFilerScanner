#pragma once

#include "MainWindow.h"
#include "UIC_MainWindow.h"

#include "Config.h"

MainWindow::MainWindow(void)
:
	ui(new Ui::MainWindow())
{
	//Setup window flags
	setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMaximizeButtonHint);

	//Setup UI
	ui->setupUi(this);

	//Setup title
	setWindowTitle(tr("Double File Scanner %1").arg(QString().sprintf("v%u.%02u-%u", DOUBLESCANNER_VERSION_MAJOR, DOUBLESCANNER_VERSION_MINOR, DOUBLESCANNER_VERSION_PATCH)));

	//Setup size
	setMinimumSize(size());
}

MainWindow::~MainWindow(void)
{
	delete ui;
}
