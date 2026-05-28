#pragma once
#include <QObject>
#include <QString>

class AppSettings : public QObject {
    Q_OBJECT
public:
    static AppSettings& instance();

    QString serialPort() const;
    void setSerialPort(const QString& port);

    QString logDirectory() const;
    void setLogDirectory(const QString& dir);

    int autoSkipSeconds() const;
    void setAutoSkipSeconds(int seconds);

    int minTransmissionSeconds() const;
    void setMinTransmissionSeconds(int seconds);

    // 10-char bit string: position 0=Police … 9=Racing; '0'=enabled, '1'=disabled
    QString serviceSearchBits() const;
    void setServiceSearchBits(const QString& bits);

    bool autoConnectOnStart() const;
    void setAutoConnectOnStart(bool enabled);

    bool minimizeToTray() const;
    void setMinimizeToTray(bool enabled);

    int windowWidth() const;
    int windowHeight() const;
    void setWindowSize(int w, int h);

private:
    explicit AppSettings(QObject* parent = nullptr);
};
