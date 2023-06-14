#include <QFileDialog>
#include <QString>
#include <QThread>
#include <QDebug>
#include <QFile>
#include <QMessageBox>

#include <iostream>
#include <thread>
#include <chrono>

#include "../include/mainwindow.h"
#include "../include/worker.h"
#include "../include/parse.h"

//using namespace mn::CppLinuxSerial;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow) {

	// Setup user interface
	ui -> setupUi(this);
	ui -> startButton -> setEnabled(false);
	startButtonState = 0;

	// Call destructor on close
	this->setAttribute(Qt::WA_DeleteOnClose);

	// Create worker thread
	thread = new QThread();
	worker = new Worker();

	worker -> moveToThread(thread);

	connect(worker, SIGNAL(consoleLog(QString)), ui->console, SLOT(append(QString)));
	connect(worker, SIGNAL(fileDone()), this, SLOT(fileDone()));
	connect(worker, SIGNAL(connectionError()), this, SLOT(connectionError()));
	connect(worker, SIGNAL(workRequested()), thread, SLOT(start()));
	connect(thread, SIGNAL(started()), worker, SLOT(doWork()));
	connect(worker, SIGNAL(finished()), thread, SLOT(quit()), Qt::DirectConnection);

	connect(ui -> xmulSpinBox, SIGNAL(valueChanged(double)), this, SLOT(on_xmulSpinBox_valueChanged(double)));
	connect(ui -> ymulSpinBox, SIGNAL(valueChanged(double)), this, SLOT(on_ymulSpinBox_valueChanged(double)));
	connect(ui -> speedSpinBox, SIGNAL(valueChanged(double)), this, SLOT(on_speedSpinBox_valueChanged(double)));

	worker -> requestWork();
}

MainWindow::~MainWindow() {
	worker->abort();
	thread->wait();
	delete thread;
	delete worker;
	
	delete ui;
}

void MainWindow::on_xincButton_clicked() {
	qDebug() << "xinc button clicked";
	worker -> addCommand(moveCmd(10 * xMultiplier, 0, speed));
}

void MainWindow::on_xdecButton_clicked() {
	qDebug() << "xdec button clicked";
	worker -> addCommand(moveCmd(-10 * xMultiplier, 0, speed));
}

void MainWindow::on_yincButton_clicked() {
	qDebug() << "yinc button clicked";
	worker -> addCommand(moveCmd(0, 10 * yMultiplier, speed));
}

void MainWindow::on_ydecButton_clicked() {
	qDebug() << "ydec button clicked";
	worker -> addCommand(moveCmd(0, -10 * yMultiplier, speed));
}

void MainWindow::on_xmulSpinBox_valueChanged(double val) {
	qDebug() << "x multiplier spinbox value changed";
	xMultiplier = val;
}

void MainWindow::on_ymulSpinBox_valueChanged(double val) {
	qDebug() << "y multiplier spinbox value changed";
	yMultiplier = val;
}

void MainWindow::on_speedSpinBox_valueChanged(double val) {
	qDebug() << "speed spinbox value changed";
	speed = val;
}

void MainWindow::on_laserCheckbox_stateChanged(int state) {
	// State 0 - unchecked
	// State 2 - checked
	qDebug() << "Checkbox clicked, state:" << (state ? "checked" : "unchecked");
	state ? worker -> addCommand("M03\n") : worker -> addCommand("M05\n");
}

void MainWindow::on_selectFileButton_clicked() {
	// Create file dialog
	fileName = QFileDialog::getOpenFileName(
		this,
		tr("Open file"),
		"",
		tr("(*.svg *.nc)") // Filters
	);
	
	if (fileName == "") {
		qDebug() << "No file selected";
		return;
	}
	qDebug() << "File" << fileName << "selected";
	ui -> filenameLabel -> setText(fileName);
	ui -> startButton -> setEnabled(true);
}

void MainWindow::on_startButton_clicked() {
	QString buttonNames[3] = { "Start", "Pause", "Resume" };
	qDebug() << buttonNames[startButtonState] << "button clicked";

	switch (startButtonState) {
		case 0: // Start
			ui -> startButton -> setText(buttonNames[1]);
			startButtonState++;
			break;
		case 1: // Pause
			ui -> startButton -> setText(buttonNames[2]);
			worker -> pause(true);
			startButtonState++;
			return;
		case 2:	// Resume
			ui -> startButton -> setText(buttonNames[1]);
			worker -> pause(false);
			startButtonState--;
			return;
	}

	// Get suffix
	QFileInfo fileInfo(fileName);
	QString suffix = fileInfo.suffix();

	// Parse file
	QVector<QString> commands;
	if (suffix == "nc") commands = ncToCmd(fileName);
	else if (suffix == "svg") commands = svgToCmd(fileName);

	blockUi();

	worker -> addCommand("G01 X0 Y0 F" + QString::number(speed) + "\n");
	qDebug() << speed;
	worker -> addCommand("M02\n"); // End previous program, start counting lines from 0 again
	for (QString cmd : commands) {	// Add all commands
		cmd = multiplyAttribute(cmd, 'X', xMultiplier);
		cmd = multiplyAttribute(cmd, 'Y', yMultiplier);
		worker -> addCommand(cmd);
	}
	worker -> addCommand("END"); // Command for unblocking ui when done*
}

void MainWindow::on_stopButton_clicked() {
	qDebug() << "Stop button clicked";
	worker -> stop();
}

void MainWindow::blockUi() {
	ui -> navGroupBox -> setEnabled(false);
	ui -> settingsGroupBox -> setEnabled(false);
	ui -> laserCheckbox -> setEnabled(false);
	ui -> selectFileButton -> setEnabled(false);
}

void MainWindow::unblockUi() {
	ui -> navGroupBox -> setEnabled(true);
	ui -> settingsGroupBox -> setEnabled(true);
	ui -> laserCheckbox -> setEnabled(true);
	ui -> selectFileButton -> setEnabled(true);
	ui -> startButton -> setText("Start");
}

void MainWindow::fileDone() {
	unblockUi();
	startButtonState = 0;
}

void MainWindow::connectionError() {
	qDebug() << "Connection error";
	QMessageBox::information(0, "error", "Connection error");
	close(); // Quit
}