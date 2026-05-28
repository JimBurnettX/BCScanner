#pragma once
#include <QObject>
#include <QDateTime>
#include <QString>

struct TransmissionRecord {
    QDateTime startTime;
    int channelIndex = 0;
    QString channelLabel;
    QString frequency;   // e.g. "462.5625"
    int ctcssDcsCode = 0;
    QString ctcssDcsLabel;
    int durationSeconds = 0;
};

class TransmissionLogger : public QObject {
    Q_OBJECT
public:
    explicit TransmissionLogger(QObject* parent = nullptr);

    void beginTransmission(int channelIndex, const QString& label,
                           const QString& frequency, int ctcssDcsCode);
    void endTransmission();
    void cancelTransmission();

    bool isActive() const { return m_active; }
    int elapsedSeconds() const;
    const TransmissionRecord& lastRecord() const { return m_last; }

signals:
    void transmissionLogged(const TransmissionRecord& record);

private:
    void writeLog(const TransmissionRecord& record);
    QString logFilePath() const;

    bool m_active = false;
    TransmissionRecord m_current;
    TransmissionRecord m_last;
};
