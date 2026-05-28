QT       += core gui widgets serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = bcscan
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/lcdwidget.cpp \
    src/scannerserial.cpp \
    src/programwindow.cpp \
    src/settingswindow.cpp \
    src/appsettings.cpp \
    src/transmissionlogger.cpp \
    src/sqlpoller.cpp \
    src/stspoller.cpp \
    src/debugwindow.cpp

HEADERS += \
    src/mainwindow.h \
    src/lcdwidget.h \
    src/scannerserial.h \
    src/programwindow.h \
    src/settingswindow.h \
    src/appsettings.h \
    src/transmissionlogger.h \
    src/ctcssdcsdata.h \
    src/sqlpoller.h \
    src/stspoller.h \
    src/debugwindow.h

INCLUDEPATH += src
