#include "transmissionlogger.h"
#include "appsettings.h"
#include <QFile>
#include <QTextStream>
#include <QDir>

TransmissionLogger::TransmissionLogger(QObject* parent) : QObject(parent) {}

void TransmissionLogger::beginTransmission(int channelIndex, const QString& label,
                                           const QString& frequency, const QString& ctcssDcsLabel) {
    m_current.startTime      = QDateTime::currentDateTime();
    m_current.channelIndex   = channelIndex;
    m_current.channelLabel   = label;
    m_current.frequency      = frequency;
    m_current.ctcssDcsLabel  = ctcssDcsLabel;
    m_current.durationSeconds = 0;
    m_active = true;
}

void TransmissionLogger::updateCtcssDcs(const QString& label) {
    if (m_active && !label.isEmpty())
        m_current.ctcssDcsLabel = label;
}

void TransmissionLogger::endTransmission() {
    if (!m_active) return;
    m_current.durationSeconds = static_cast<int>(
        m_current.startTime.secsTo(QDateTime::currentDateTime()));
    m_active = false;
    writeLog(m_current);
    m_last = m_current;
    emit transmissionLogged(m_last);
}

void TransmissionLogger::cancelTransmission() {
    m_active = false;
}

int TransmissionLogger::elapsedSeconds() const {
    if (!m_active) return 0;
    return static_cast<int>(m_current.startTime.secsTo(QDateTime::currentDateTime()));
}

QString TransmissionLogger::logFilePath() const {
    QString dir = AppSettings::instance().logDirectory();
    QDir().mkpath(dir);
    QString date = QDate::currentDate().toString("yyyy-MM-dd");
    return dir + QDir::separator() + "transmissions-" + date + ".txt";
}

void TransmissionLogger::writeLog(const TransmissionRecord& r) {
    QFile file(logFilePath());
    if (!file.open(QIODevice::Append | QIODevice::Text)) return;

    QTextStream out(&file);
    out << r.startTime.toString("yyyy-MM-dd hh:mm:ss")
        << " | CH " << QString::number(r.channelIndex).rightJustified(3, ' ')
        << " | " << r.channelLabel.leftJustified(16, ' ')
        << " | " << r.frequency.rightJustified(10, ' ') << " MHz"
        << " | " << r.ctcssDcsLabel.leftJustified(20, ' ')
        << " | " << r.durationSeconds << "s"
        << "\n";
}
