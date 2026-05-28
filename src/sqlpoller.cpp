#include "sqlpoller.h"
#include "scannerserial.h"

SqlPoller::SqlPoller(ScannerSerial* scanner, QObject* parent)
    : QObject(parent), m_scanner(scanner)
{
    m_pollTimer = new QTimer(this);
    m_pollTimer->setSingleShot(false);
    connect(m_pollTimer, &QTimer::timeout, this, &SqlPoller::onPollTimer);
}

void SqlPoller::start()
{
    if (m_running) return;
    m_running     = true;
    m_squelchOpen = false;
    m_cmdPending  = false;
    m_slowCount   = 0;
    m_fastCount   = 0;
    m_pollTimer->start(m_pollIntervalMs);
}

void SqlPoller::stop()
{
    m_pollTimer->stop();
    m_running    = false;
    m_cmdPending = false;
}

void SqlPoller::onPollTimer()
{
    if (m_cmdPending) {
        // Previous poll never got a response — count it as a timeout
        qint64 elapsed = m_cmdTimer.elapsed();
        m_cmdPending = false;
        handleLatency(elapsed, true);
        return;
    }

    m_cmdTimer.start();
    m_cmdPending = true;

    m_scanner->sendCommand("SQL", [this](bool ok, const QString& resp) {
        if (!m_running) { m_cmdPending = false; return; }
        qint64 elapsed = m_cmdTimer.elapsed();
        m_cmdPending = false;
        handleLatency(elapsed, !ok);
        Q_UNUSED(resp);
    });
}

void SqlPoller::handleLatency(qint64 elapsedMs, bool timedOut)
{
    emit pollLatency(elapsedMs);

    const bool slow = timedOut || elapsedMs >= m_openThresholdMs;
    if (slow) {
        ++m_slowCount;
        m_fastCount = 0;
        if (m_slowCount >= m_confirmCount)
            transition(true);
    } else {
        ++m_fastCount;
        m_slowCount = 0;
        if (m_fastCount >= m_confirmCount)
            transition(false);
    }
}

void SqlPoller::transition(bool open)
{
    if (open == m_squelchOpen) return;
    m_squelchOpen = open;
    if (open)
        emit squelchOpened();
    else
        emit squelchClosed();
}
