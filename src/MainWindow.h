#pragma once

#include <QMainWindow>

//UIC forward declartion
namespace Ui {
	class MainWindow;
}

//MainWindow class
class MainWindow: public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(void);
	virtual ~MainWindow(void);

protected:
	Ui::MainWindow *const ui;
};
