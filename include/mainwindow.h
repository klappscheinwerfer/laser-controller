#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QString>

#include "ui_mainwindow.h"
#include "../include/CppLinuxSerial/SerialPort.hpp"
#include "../include/worker.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

	private:
		Ui::MainWindow *ui;

		QThread *thread;
		Worker *worker;

		QString fileName;

		int startButtonState;

		float xMultiplier = 1;
		float yMultiplier = 1;
		float speed = 500;

		// Ui
		void blockUi();
		void unblockUi();

	private slots:
		// Positioning
		void on_xincButton_clicked();
		void on_xdecButton_clicked();
		void on_yincButton_clicked();
		void on_ydecButton_clicked();

		// Laser
		void on_laserCheckbox_stateChanged(int state);

		// Select file
		void on_selectFileButton_clicked();

		// Start cutting
		void on_startButton_clicked();
		void on_stopButton_clicked();

		// Settings
		void on_xmulSpinBox_valueChanged(double val);
		void on_ymulSpinBox_valueChanged(double val);
		void on_speedSpinBox_valueChanged(double val);

		// Signals
		void fileDone();
		void connectionError();

	public:
		explicit MainWindow(QWidget *parent = 0);
		~MainWindow();
};

#endif // MAINWINDOW_H