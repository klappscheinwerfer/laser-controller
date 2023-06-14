#include <QThread>
#include <QDebug>
#include <QMessageBox>

#include <thread>
#include <chrono>
#include <iostream>
#include <string>

#include "../include/worker.h"
#include "../include/CppLinuxSerial/SerialPort.hpp"

using namespace mn::CppLinuxSerial;

Worker::Worker(QObject *parent) :
	QObject(parent) {

	// Controls
	_working = false;
	_abort = false;
	_stop = false;
	_paused = false;

	// States
	executing = false;
	laserState = false;
}

void Worker::requestWork() {
	mutex.lock();
	_working = true;
	_abort = false;
	qDebug() << "Request worker start in Thread " << thread() -> currentThreadId();
	mutex.unlock();

	emit workRequested();
}

void Worker::abort() {
	mutex.lock();
	if (_working) {
		_abort = true;
		qDebug() << "Request worker aborting in Thread " << thread() -> currentThreadId();
	}
	mutex.unlock();
}

void Worker::stop() {
	mutex.lock();
	_stop = true;
	qDebug() << "Request emergency stop";
	mutex.unlock();
}


void Worker::doWork() {
	// Set serial port properties
	serialPort.SetDevice("/dev/ttyACM0");
	serialPort.SetBaudRate(BaudRate::B_115200);
	serialPort.SetNumDataBits(NumDataBits::EIGHT);
	serialPort.SetParity(Parity::NONE);
	serialPort.SetTimeout(100);

	// Open port and wait for the arduino to start
	try {
		serialPort.Open();
	}
	catch (Exception& e) {
		emit connectionError();
		emit finished();
		return;
	}
	std::this_thread::sleep_for(std::chrono::seconds(3));
	emit consoleLog("Connected");

	std::string buffer, newdata, reply;
	while(true) {
		// Process abort check
		mutex.lock();
		bool abort = _abort;
		mutex.unlock();

		if (abort) {
			qDebug() << "Aborting worker process in Thread " << thread() -> currentThreadId();
			break;
		}

		// Read from serial and add to buffer
		serialPort.Read(newdata);
		buffer += newdata;
		newdata.clear();
		
		// Handle serial input
		if (buffer.length() > 0) {
			for (long unsigned i = 0; i < buffer.length(); i++) {
				if (buffer[i] == '\n') {
					reply = buffer.substr(0, i);
					buffer.erase(0, i + 1);
					break;
				}
			}

			if (reply.length() > 0) {
				switch(reply[0]) {
					case 'r':
						// Ready
						executing = false;
						emit consoleLog("Ready for the next command");
						break;
					case 'l':
						// Line
						reply.erase(0, 2);
						emit consoleLog("Line: " + QString::fromStdString(reply));
						break;
					case 'm':
						// Message
						reply.erase(0, 2);
						emit consoleLog("Message: " + QString::fromStdString(reply));
						break;
					case 'd':
						// Debug
						reply.erase(0, 2);
						emit consoleLog("Debug: " + QString::fromStdString(reply));
						break;
					case 'e':
						// Error
						reply.erase(0, 2);
						emit consoleLog("Error: " + QString::fromStdString(reply));
						break;
				}
				reply.clear();
			}
		}

		// Pause check
		while (!executing && _paused) {
			serialPort.Write("M05\n");
			executing = true;

			while (_paused) std::this_thread::sleep_for(std::chrono::milliseconds(50));
			commands.push_front("M03\n");
		}

		// Emergency stop check
		mutex.lock();
		bool stop = _stop;
		_stop = false;
		mutex.unlock();

		if (stop) {
			emit consoleLog("Emergency stop");
			qDebug() << "Stopping current command";
			serialPort.Write("\e"); // Send stop signal
			commands.clear(); // Clear command list
			executing = false;
			stop = false;
			emit fileDone();
		}

		// Command available check
		if (!executing && commands.size() > 0) {
			std::string cmd = commands.takeFirst().toStdString();

			if (cmd == "END") {
				emit fileDone();
				continue;
			}
			else if (cmd == "M03\n") { laserState = true; }
			else if (cmd == "M05\n") { laserState = false; }

			executing = true;
			serialPort.Write(cmd);
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	executing = false;

	// Process can't be aborted anymore
	mutex.lock();
	_working = false;
	mutex.unlock();

	// Make sure the laser is off and close connection
	serialPort.Write("M05\n");
	serialPort.Close();

	qDebug() << "Worker process finished in Thread " << thread() -> currentThreadId();

	emit finished();
}

void Worker::pause(bool p) {
	mutex.lock();
	_paused = p;
	mutex.unlock();
}

void Worker::addCommand(QString cmd) {
	mutex.lock();
	qDebug() << "Command added: "<< cmd;
	commands.append(cmd);
	mutex.unlock();
}