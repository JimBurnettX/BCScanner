#include "appsettings.h"
#include <QSettings>
#include <QStandardPaths>

AppSettings& AppSettings::instance() {
    static AppSettings inst;
    return inst;
}

AppSettings::AppSettings(QObject* parent) : QObject(parent) {}

static QSettings& s() {
    static QSettings settings("BCScan", "BCScan");
    return settings;
}

QString AppSettings::serialPort() const {
    return s().value("serial/port", "").toString();
}
void AppSettings::setSerialPort(const QString& port) {
    s().setValue("serial/port", port);
}

QString AppSettings::logDirectory() const {
    QString def = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return s().value("log/directory", def).toString();
}
void AppSettings::setLogDirectory(const QString& dir) {
    s().setValue("log/directory", dir);
}

int AppSettings::autoSkipSeconds() const {
    return s().value("scan/autoSkipSeconds", 45).toInt();
}
void AppSettings::setAutoSkipSeconds(int seconds) {
    s().setValue("scan/autoSkipSeconds", seconds);
}

int AppSettings::minTransmissionSeconds() const {
    return s().value("scan/minTransmissionSeconds", 3).toInt();
}
void AppSettings::setMinTransmissionSeconds(int seconds) {
    s().setValue("scan/minTransmissionSeconds", seconds);
}

QString AppSettings::serviceSearchBits() const {
    return s().value("scan/serviceSearchBits", "0000000000").toString();
}
void AppSettings::setServiceSearchBits(const QString& bits) {
    s().setValue("scan/serviceSearchBits", bits);
}

bool AppSettings::autoConnectOnStart() const {
    return s().value("serial/autoConnect", false).toBool();
}
void AppSettings::setAutoConnectOnStart(bool enabled) {
    s().setValue("serial/autoConnect", enabled);
}

bool AppSettings::minimizeToTray() const {
    return s().value("ui/minimizeToTray", false).toBool();
}
void AppSettings::setMinimizeToTray(bool enabled) {
    s().setValue("ui/minimizeToTray", enabled);
}

int AppSettings::windowWidth() const  { return s().value("ui/width",  820).toInt(); }
int AppSettings::windowHeight() const { return s().value("ui/height", 700).toInt(); }
void AppSettings::setWindowSize(int w, int h) {
    s().setValue("ui/width", w);
    s().setValue("ui/height", h);
}
