TEMPLATE = app

TARGET = laser-controller

INCLUDEPATH += \
	include \
	src

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QT += core gui widgets xml

SOURCES += \
	src/main.cpp \
	src/mainwindow.cpp \
	src/SerialPort.cpp \
	src/worker.cpp

HEADERS += \
	include/mainwindow.h \
	include/CppLinuxSerial/SerialPort.hpp \
	include/CppLinuxSerial/Exception.hpp \
	include/worker.h \
	include/parse.h

FORMS += \
	mainwindow.ui

CONFIG += release debug

release: DESTDIR = build/release
debug:   DESTDIR = build/debug

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui