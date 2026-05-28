#pragma once
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QQueue>
#include <functional>

struct ChannelInfo {
    int    index = 0;
    QString name;
    QString frequency;   // raw string as returned, e.g. "4625625" = 462.5625 MHz
    QString modulation;  // AUTO AM FM NFM
    int    ctcssDcs = 0;
    int    delay = 2;
    bool   lockout = false;
    bool   priority = false;

    bool isEmpty() const { return frequency.isEmpty() || frequency == "0"; }

    // Convert raw frequency integer string to MHz display string
    QString freqMhz() const {
        bool ok;
        long long f = frequency.toLongLong(&ok);
        if (!ok || f == 0) return "----.----";
        return QString::number(f / 10000.0, 'f', 4);
    }
};

struct ScanGroupStatus {
    bool banks[10] = {true,true,true,true,true,true,true,true,true,true};
    // banks[0] = bank1 ... banks[8] = bank9, banks[9] = bank0 (per SCG order)
};

using CommandCallback = std::function<void(bool ok, const QString& response)>;

class ScannerSerial : public QObject {
    Q_OBJECT
public:
    enum class State { Disconnected, Scanning, Hold, Programming };

    explicit ScannerSerial(QObject* parent = nullptr);
    ~ScannerSerial() override;

    bool connectPort(const QString& portName);
    void disconnectPort();
    bool isConnected() const;
    State state() const { return m_state; }

    // Raw command send (queued). Callback receives (ok, response).
    void sendCommand(const QString& cmd, CommandCallback cb = nullptr);

    // High-level operations
    void getModelInfo(CommandCallback cb);
    void getFirmwareVersion(CommandCallback cb);
    void enterProgramMode(CommandCallback cb = nullptr);
    void exitProgramMode(CommandCallback cb = nullptr);

    void getVolume(CommandCallback cb);
    void setVolume(int level, CommandCallback cb = nullptr);
    void getSquelch(CommandCallback cb);
    void setSquelch(int level, CommandCallback cb = nullptr);

    void getBacklight(CommandCallback cb);
    void setBacklight(const QString& event, CommandCallback cb = nullptr);

    void getBatteryInfo(CommandCallback cb);
    void setBatteryChargeTime(int hours, CommandCallback cb = nullptr);

    void getKeyBeep(CommandCallback cb);
    void setKeyBeep(int level, bool keyLock, CommandCallback cb = nullptr);

    void getPriorityMode(CommandCallback cb);
    void setPriorityMode(int mode, CommandCallback cb = nullptr);

    void getLcdContrast(CommandCallback cb);
    void setLcdContrast(int contrast, CommandCallback cb = nullptr);

    void getWeatherSettings(CommandCallback cb);
    void setWeatherAlertPriority(bool on, CommandCallback cb = nullptr);

    void getScanGroup(CommandCallback cb);
    void setScanGroup(const ScanGroupStatus& sg, CommandCallback cb = nullptr);

    void getChannelInfo(int index, CommandCallback cb);
    void setChannelInfo(const ChannelInfo& ch, CommandCallback cb = nullptr);
    void deleteChannel(int index, CommandCallback cb = nullptr);

    void getSearchCloseCall(CommandCallback cb);
    void setSearchCloseCall(int delay, bool codeSearch, CommandCallback cb = nullptr);

    void getCloseCallSettings(CommandCallback cb);
    void setCloseCallSettings(int mode, bool alertBeep, bool alertLight,
                              const QString& bands, bool lockout, CommandCallback cb = nullptr);

    void getServiceSearchGroup(CommandCallback cb);
    void setServiceSearchGroup(const QString& bits, CommandCallback cb = nullptr);

    void getCustomSearchGroup(CommandCallback cb);
    void setCustomSearchGroup(const QString& bits, CommandCallback cb = nullptr);

    void getCustomSearchRange(int index, CommandCallback cb);
    void setCustomSearchRange(int index, long long limitLow, long long limitHigh, CommandCallback cb = nullptr);

    void clearAllMemory(CommandCallback cb = nullptr);

    void getGlobalLockoutFreqs(std::function<void(const QStringList&)> cb);
    void lockoutFrequency(long long freq, CommandCallback cb = nullptr);
    void unlockFrequency(long long freq, CommandCallback cb = nullptr);

    static ChannelInfo parseChannelInfo(const QString& response);
    static ScanGroupStatus parseScanGroup(const QString& response);

signals:
    void connected();
    void disconnected();
    void serialError(const QString& msg);
    void commandSent(const QString& cmd);
    void responseReceived(const QString& resp);

private:
    struct PendingCmd {
        QString command;
        CommandCallback callback;
    };

    void enqueue(const QString& cmd, CommandCallback cb);
    void sendNext();
    void onReadyRead();
    void onTimeout();
    void handleResponse(const QString& response);

    QSerialPort* m_port = nullptr;
    QByteArray   m_buffer;
    QQueue<PendingCmd> m_queue;
    bool         m_busy = false;
    QTimer*      m_timeout = nullptr;
    State        m_state = State::Disconnected;
    PendingCmd   m_current;
};
