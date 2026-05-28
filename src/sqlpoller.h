#pragma once
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include "scannerserial.h"

// Detects squelch opens on the BC125AT by polling SQL while the scanner
// is running freely (no PRG mode needed).
//
// How it works:
//   When the BC125AT is receiving a signal the MCU is busy handling audio
//   interrupt routines. Serial responses to SQL are significantly delayed
//   (>400 ms) or don't arrive within the poll window. Closed squelch
//   responds in <150 ms. We use that latency delta as the hit detector.

class SqlPoller : public QObject {
    Q_OBJECT
public:
    explicit SqlPoller(ScannerSerial* scanner, QObject* parent = nullptr);

    void start();
    void stop();
    bool isRunning() const { return m_running; }

    // Tuning — expose as app settings later if needed
    void setPollIntervalMs(int ms)     { m_pollIntervalMs = ms; }
    void setOpenThresholdMs(int ms)    { m_openThresholdMs = ms; }
    void setConfirmCount(int n)        { m_confirmCount = n; }

    bool squelchIsOpen() const         { return m_squelchOpen; }

signals:
    void squelchOpened();               // leading edge of a hit
    void squelchClosed();               // trailing edge — transmission ended
    void pollLatency(qint64 elapsedMs); // emitted each cycle (for debug/display)

private slots:
    void onPollTimer();

private:
    void handleLatency(qint64 elapsedMs, bool timedOut);
    void transition(bool open);

    ScannerSerial* m_scanner;
    QTimer*        m_pollTimer;

    bool m_running       = false;
    bool m_squelchOpen   = false;
    bool m_cmdPending    = false;
    QElapsedTimer m_cmdTimer;

    int m_pollIntervalMs  = 280;   // how often to send SQL
    int m_openThresholdMs = 420;   // response slower than this = squelch open
    int m_confirmCount    = 2;     // consecutive samples needed to change state

    int m_slowCount = 0;
    int m_fastCount = 0;
};
