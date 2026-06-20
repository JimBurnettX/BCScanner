#include "scannerserial.h"
#include <QDebug>

static constexpr int CMD_TIMEOUT_MS = 3000;

ScannerSerial::ScannerSerial(QObject* parent) : QObject(parent) {
    m_timeout = new QTimer(this);
    m_timeout->setSingleShot(true);
    connect(m_timeout, &QTimer::timeout, this, &ScannerSerial::onTimeout);
}

ScannerSerial::~ScannerSerial() {
    QObject::disconnect(this, nullptr, nullptr, nullptr);
    disconnectPort();
}

bool ScannerSerial::connectPort(const QString& portName) {
    if (m_port && m_port->isOpen()) {
        m_port->close();
        delete m_port;
    }
    m_port = new QSerialPort(this);
    m_port->setPortName(portName);
    m_port->setBaudRate(QSerialPort::Baud9600);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port->open(QIODevice::ReadWrite)) {
        emit serialError(m_port->errorString());
        delete m_port;
        m_port = nullptr;
        return false;
    }

    connect(m_port, &QSerialPort::readyRead, this, &ScannerSerial::onReadyRead);
    m_state = State::Scanning;
    m_buffer.clear();
    m_queue.clear();
    m_busy = false;
    emit connected();
    return true;
}

void ScannerSerial::disconnectPort() {
    m_timeout->stop();
    if (m_port) {
        m_port->close();
        delete m_port;
        m_port = nullptr;
    }
    m_queue.clear();
    m_current.callback = nullptr;  // prevent in-flight callback firing on a dangling object
    m_busy = false;
    m_state = State::Disconnected;
    emit disconnected();
}

bool ScannerSerial::isConnected() const {
    return m_port && m_port->isOpen();
}

void ScannerSerial::enqueue(const QString& cmd, CommandCallback cb) {
    m_queue.enqueue({cmd, cb});
    if (!m_busy) sendNext();
}

void ScannerSerial::sendNext() {
    if (m_queue.isEmpty() || !isConnected()) { m_busy = false; return; }
    m_busy = true;
    m_current = m_queue.dequeue();
    QByteArray data = m_current.command.toLatin1() + "\r";
    m_port->write(data);
    m_timeout->start(CMD_TIMEOUT_MS);
    emit commandSent(m_current.command);
}

void ScannerSerial::onReadyRead() {
    m_buffer += m_port->readAll();
    while (true) {
        int idx = m_buffer.indexOf('\r');
        if (idx < 0) break;
        QString response = QString::fromLatin1(m_buffer.left(idx)).trimmed();
        m_buffer.remove(0, idx + 1);
        if (!response.isEmpty()) {
            handleResponse(response);
        }
    }
}

void ScannerSerial::onTimeout() {
    qWarning() << "Command timed out:" << m_current.command;
    if (m_current.callback) m_current.callback(false, "TIMEOUT");
    m_busy = false;
    sendNext();
}

void ScannerSerial::handleResponse(const QString& response) {
    m_timeout->stop();
    emit responseReceived(response);

    bool ok = !response.startsWith("ERR") && !response.startsWith("NG");

    if (m_current.callback) {
        m_current.callback(ok, response);
    }
    m_busy = false;
    sendNext();
}

void ScannerSerial::sendCommand(const QString& cmd, CommandCallback cb) {
    enqueue(cmd, cb);
}

void ScannerSerial::getModelInfo(CommandCallback cb) {
    enqueue("MDL", cb);
}
void ScannerSerial::getFirmwareVersion(CommandCallback cb) {
    enqueue("VER", cb);
}
void ScannerSerial::enterProgramMode(CommandCallback cb) {
    enqueue("PRG", [this, cb](bool ok, const QString& resp){
        if (ok) m_state = State::Programming;
        if (cb) cb(ok, resp);
    });
}
void ScannerSerial::exitProgramMode(CommandCallback cb) {
    enqueue("EPG", [this, cb](bool ok, const QString& resp){
        if (ok) m_state = State::Scanning;
        if (cb) cb(ok, resp);
    });
}

void ScannerSerial::getVolume(CommandCallback cb)             { enqueue("VOL", cb); }
void ScannerSerial::setVolume(int level, CommandCallback cb)  { enqueue(QString("VOL,%1").arg(level), cb); }
void ScannerSerial::getSquelch(CommandCallback cb)            { enqueue("SQL", cb); }
void ScannerSerial::setSquelch(int level, CommandCallback cb) { enqueue(QString("SQL,%1").arg(level), cb); }

void ScannerSerial::getBacklight(CommandCallback cb) { enqueue("BLT", cb); }
void ScannerSerial::setBacklight(const QString& event, CommandCallback cb) {
    enqueue(QString("BLT,%1").arg(event), cb);
}

void ScannerSerial::getBatteryInfo(CommandCallback cb) { enqueue("BSV", cb); }
void ScannerSerial::setBatteryChargeTime(int hours, CommandCallback cb) {
    enqueue(QString("BSV,%1").arg(hours), cb);
}

void ScannerSerial::getKeyBeep(CommandCallback cb) { enqueue("KBP", cb); }
void ScannerSerial::setKeyBeep(int level, bool keyLock, CommandCallback cb) {
    enqueue(QString("KBP,%1,%2").arg(level).arg(keyLock ? 1 : 0), cb);
}

void ScannerSerial::getPriorityMode(CommandCallback cb) { enqueue("PRI", cb); }
void ScannerSerial::setPriorityMode(int mode, CommandCallback cb) {
    enqueue(QString("PRI,%1").arg(mode), cb);
}

void ScannerSerial::getLcdContrast(CommandCallback cb) { enqueue("CNT", cb); }
void ScannerSerial::setLcdContrast(int contrast, CommandCallback cb) {
    enqueue(QString("CNT,%1").arg(contrast), cb);
}

void ScannerSerial::getWeatherSettings(CommandCallback cb) { enqueue("WXS", cb); }
void ScannerSerial::setWeatherAlertPriority(bool on, CommandCallback cb) {
    enqueue(QString("WXS,%1").arg(on ? 1 : 0), cb);
}

void ScannerSerial::getScanGroup(CommandCallback cb) { enqueue("SCG", cb); }
void ScannerSerial::setScanGroup(const ScanGroupStatus& sg, CommandCallback cb) {
    QString bits;
    for (int i = 0; i < 10; ++i) bits += sg.banks[i] ? '0' : '1';
    enqueue(QString("SCG,%1").arg(bits), cb);
}

void ScannerSerial::getChannelInfo(int index, CommandCallback cb) {
    enqueue(QString("CIN,%1").arg(index), cb);
}
void ScannerSerial::setChannelInfo(const ChannelInfo& ch, CommandCallback cb) {
    QString cmd = QString("CIN,%1,%2,%3,%4,%5,%6,%7,%8")
        .arg(ch.index)
        .arg(ch.name)
        .arg(ch.frequency)
        .arg(ch.modulation)
        .arg(ch.ctcssDcs)
        .arg(ch.delay)
        .arg(ch.lockout ? 1 : 0)
        .arg(ch.priority ? 1 : 0);
    enqueue(cmd, cb);
}
void ScannerSerial::deleteChannel(int index, CommandCallback cb) {
    enqueue(QString("DCH,%1").arg(index), cb);
}

void ScannerSerial::getSearchCloseCall(CommandCallback cb) { enqueue("SCO", cb); }
void ScannerSerial::setSearchCloseCall(int delay, bool codeSearch, CommandCallback cb) {
    enqueue(QString("SCO,%1,%2").arg(delay).arg(codeSearch ? 1 : 0), cb);
}

void ScannerSerial::getCloseCallSettings(CommandCallback cb) { enqueue("CLC", cb); }
void ScannerSerial::setCloseCallSettings(int mode, bool alertBeep, bool alertLight,
                                         const QString& bands, bool lockout, CommandCallback cb) {
    enqueue(QString("CLC,%1,%2,%3,%4,%5")
        .arg(mode).arg(alertBeep?1:0).arg(alertLight?1:0).arg(bands).arg(lockout?1:0), cb);
}

void ScannerSerial::getServiceSearchGroup(CommandCallback cb) { enqueue("SSG", cb); }
void ScannerSerial::setServiceSearchGroup(const QString& bits, CommandCallback cb) {
    enqueue(QString("SSG,%1").arg(bits), cb);
}

void ScannerSerial::getCustomSearchGroup(CommandCallback cb) { enqueue("CSG", cb); }
void ScannerSerial::setCustomSearchGroup(const QString& bits, CommandCallback cb) {
    enqueue(QString("CSG,%1").arg(bits), cb);
}

void ScannerSerial::getCustomSearchRange(int index, CommandCallback cb) {
    enqueue(QString("CSP,%1").arg(index), cb);
}
void ScannerSerial::setCustomSearchRange(int index, long long limitLow, long long limitHigh,
                                         CommandCallback cb) {
    enqueue(QString("CSP,%1,%2,%3").arg(index).arg(limitLow).arg(limitHigh), cb);
}

void ScannerSerial::clearAllMemory(CommandCallback cb) {
    enqueue("CLR", cb);
}

void ScannerSerial::getGlobalLockoutFreqs(std::function<void(const QStringList&)> cb) {
    auto list = std::make_shared<QStringList>();
    std::function<void(bool, const QString&)> handler;
    handler = [this, list, cb, &handler](bool ok, const QString& resp) {
        if (!ok || resp == "GLF,-1") {
            if (cb) cb(*list);
            return;
        }
        QStringList parts = resp.split(',');
        if (parts.size() >= 2 && parts[1] != "-1") {
            list->append(parts[1]);
            enqueue("GLF,***", [list, cb, handler](bool ok2, const QString& resp2){
                // This lambda captures by value - simple recursive approach
                Q_UNUSED(ok2); Q_UNUSED(resp2);
            });
        } else {
            if (cb) cb(*list);
        }
    };
    enqueue("GLF,***", handler);
}

void ScannerSerial::lockoutFrequency(long long freq, CommandCallback cb) {
    enqueue(QString("LOF,%1").arg(freq), cb);
}
void ScannerSerial::unlockFrequency(long long freq, CommandCallback cb) {
    enqueue(QString("ULF,%1").arg(freq), cb);
}

ChannelInfo ScannerSerial::parseChannelInfo(const QString& response) {
    ChannelInfo ch;
    QStringList parts = response.split(',');
    // CIN,INDEX,NAME,FRQ,MOD,CTCSS/DCS,DLY,LOUT,PRI
    if (parts.size() < 2) return ch;
    if (parts[0] != "CIN") return ch;
    if (parts.size() >= 2) ch.index      = parts[1].toInt();
    if (parts.size() >= 3) ch.name       = parts[2];
    if (parts.size() >= 4) ch.frequency  = parts[3];
    if (parts.size() >= 5) ch.modulation = parts[4];
    if (parts.size() >= 6) ch.ctcssDcs   = parts[5].toInt();
    if (parts.size() >= 7) ch.delay      = parts[6].toInt();
    if (parts.size() >= 8) ch.lockout    = parts[7].toInt() == 1;
    if (parts.size() >= 9) ch.priority   = parts[8].toInt() == 1;
    return ch;
}

ScanGroupStatus ScannerSerial::parseScanGroup(const QString& response) {
    ScanGroupStatus sg;
    QStringList parts = response.split(',');
    if (parts.size() < 2) return sg;
    QString bits = parts[1];
    for (int i = 0; i < 10 && i < bits.size(); ++i) {
        sg.banks[i] = (bits[i] == '0'); // 0=valid/enabled, 1=invalid/disabled
    }
    return sg;
}
