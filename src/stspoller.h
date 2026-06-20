#pragma once
#include <QObject>
#include <QTimer>
#include <QString>

class ScannerSerial;

// STS response mirrors the physical LCD — comma-separated fields.
// Field layout (0-based after splitting on ','):
//   [0]  "STS"
//   [1]  flags  ("011000" = normal scan/active  |  "1111" = menu/programming)
//   [2]  LCD row 0 (animation / signal chars)
//   [3]  empty separator
//   [4]  LCD row 1 — channel label / bank / search-bank info
//   [5]  empty separator
//   [6]  LCD row 2 — main info: " SCAN ", "CH027 467.6875 ", " 25.4750", etc.
//   [7+] signal bars, bank indicators, numeric flags
//
// Examples:
//   Scanning: STS,011000, ,,BANK1 ,, SCAN ,,, ,,ÍÎÏ ,,0,1,0,0,,,0,,3
//   Active:   STS,011000, ■■■■ ,,FRS Ch13 ,,CH027 467.6875 ,, ■■■■ ,, ,,ÍÎÏ1 ,,0,1,0,0,,,0,,3
//   Search:   STS,011000, ■■ ,,SEARCH BANK1 ,, 25.4750,,, ,,ÅÆÇ1234567890 ,,1,0,0,0,,,5,,3
//   Menu:     STS,1111,Enter Frequency ,***,Edit Tag ,,Set CTCSS/DCS ,,...

struct StsStatus {
    enum class State { Unknown, Scanning, Active, Search, Menu };

    State   state         = State::Unknown;
    QString channelLabel;   // e.g. "FRS Ch13" or "BANK1" or "SEARCH BANK1"
    QString frequency;      // MHz string  e.g. "467.6875" — set when Active or Search
    int     channelNumber = 0; // 1-500, parsed from "CH027" — set when Active
    bool    squelchOpen     = false; // true when state == Active or Search with signal
    int     signalStrength  = 0;     // 0-5, from field[20] of STS response
    quint16 bankMask        = 0;     // bit 0='1'…bit 8='9', bit 9='0'; from field[12]
    QString ctcssDcs;               // live tone from field[8]: "C67.0", "DCS032", or empty
    QString serviceLabel;   // non-empty when state==Search and scanner is in service search mode
                            // e.g. "Police", "Fire/Emergency", "HAM Radio", etc.
    QString raw;            // full raw STS line
};

class StsPoller : public QObject {
    Q_OBJECT
public:
    explicit StsPoller(ScannerSerial* scanner, QObject* parent = nullptr);

    void start();
    void stop();
    bool isRunning() const { return m_running; }

    void setPollIntervalMs(int ms) { m_pollIntervalMs = ms; }

    bool          squelchIsOpen()    const { return m_squelchOpen; }
    QString       currentFrequency() const { return m_currentFreq; }
    StsStatus     lastStatus()       const { return m_lastStatus; }

    static StsStatus parse(const QString& raw);

signals:
    void statusUpdated(const StsStatus& status);  // every successful poll
    void squelchOpened(const StsStatus& status);  // transition: not-active → active
    void squelchClosed();                         // transition: active → not-active
    void statusReceived(const QString& raw);      // raw line (for debug window)

private slots:
    void onPollTimer();

private:
    static QString   cleanAscii(const QString& s);

    ScannerSerial* m_scanner;
    QTimer*        m_pollTimer;

    bool      m_running     = false;
    bool      m_squelchOpen = false;
    bool      m_cmdPending  = false;
    QString   m_currentFreq;
    StsStatus m_lastStatus;

    int m_pollIntervalMs = 250;   // 4× per second
};
