#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QMutex>
#include <QVector>
#include <QString>

#include "../include/CppLinuxSerial/SerialPort.hpp"

using namespace mn::CppLinuxSerial;

class Worker : public QObject {
	Q_OBJECT

public:
	explicit Worker(QObject *parent = 0);
	void requestWork();
	void abort();
	void stop();
	void pause(bool);
	void addCommand(QString cmd);

private:
	// Controls
	bool _abort;
	bool _working;
	bool _stop;
	bool _paused;

	// States
	bool executing;
	bool laserState;

	QMutex mutex;
	SerialPort serialPort;
	QVector<QString> commands;

signals:
	void workRequested();
	void consoleLog(const QString &value);
	void fileDone();
	void finished();
	void connectionError();

public slots:
	void doWork();
};

#endif // WORKER_H