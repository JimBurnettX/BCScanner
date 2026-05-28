#include "stspoller.h"
#include "scannerserial.h"
#include <QRegularExpression>

StsPoller::StsPoller(ScannerSerial* scanner, QObject* parent)
    : QObject(parent), m_scanner(scanner)
{
    m_pollTimer = new QTimer(this);
    m_pollTimer->setSingleShot(false);
    connect(m_pollTimer, &QTimer::timeout, this, &StsPoller::onPollTimer);
}

void StsPoller::start()
{
    if (m_running) return;
    m_running     = true;
    m_squelchOpen = false;
    m_cmdPending  = false;
    m_currentFreq.clear();
    m_pollTimer->start(m_pollIntervalMs);
}

void StsPoller::stop()
{
    m_pollTimer->stop();
    m_running    = false;
    m_cmdPending = false;
    if (m_squelchOpen) {
        m_squelchOpen = false;
        emit squelchClosed();
    }
    m_currentFreq.clear();
}

void StsPoller::onPollTimer()
{
    if (m_cmdPending) return;
    m_cmdPending = true;
    m_scanner->sendCommand("STS", [this](bool ok, const QString& resp) {
        m_cmdPending = false;
        if (!m_running || !ok || resp.isEmpty()) return;

        emit statusReceived(resp);

        StsStatus s = parse(resp);
        m_lastStatus = s;
        emit statusUpdated(s);

        const bool nowOpen = s.squelchOpen;
        if (nowOpen && !m_squelchOpen) {
            m_squelchOpen = true;
            m_currentFreq = s.frequency;
            emit squelchOpened(s);
        } else if (!nowOpen && m_squelchOpen) {
            m_squelchOpen = false;
            m_currentFreq.clear();
            emit squelchClosed();
        }
    });
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

QString StsPoller::cleanAscii(const QString& s)
{
    QString out;
    out.reserve(s.size());
    for (const QChar c : s) {
        if (c.unicode() >= 0x20 && c.unicode() <= 0x7E)
            out.append(c);
    }
    return out.trimmed();
}

StsStatus StsPoller::parse(const QString& raw)
{
    StsStatus s;
    s.raw = raw;

    // Split on comma — fields can contain spaces, special chars, etc.
    const QStringList f = raw.split(',');
    if (f.size() < 7 || f[0] != "STS")
        return s;

    const QString flags   = f[1].trimmed();
    const QString label   = cleanAscii(f[4]);   // row 1: "FRS Ch13", "BANK1", "SEARCH BANK1"
    const QString main    = cleanAscii(f[6]);   // row 2: "SCAN", "CH027 467.6875", "25.4750"

    s.channelLabel = label;

    // ---- Menu / programming mode ----
    // flags field is all '1' digits when scanner is in a menu
    bool inMenu = !flags.isEmpty();
    for (const QChar c : flags) {
        if (c != '1') { inMenu = false; break; }
    }
    if (inMenu) {
        s.state = StsStatus::State::Menu;
        return s;
    }

    // Signal strength is in field[20] (1-5; 0 or empty = no signal)
    auto parseSig = [&]() -> int {
        if (f.size() <= 20) return 0;
        bool ok = false;
        int v = f[20].trimmed().toInt(&ok);
        return (ok && v >= 1 && v <= 5) ? v : 0;
    };

    // Bank activity mask from field[12]: non-ASCII prefix chars + digit chars of active banks
    // e.g. "ÅÆÇ1234567890" means all banks active; "ÍÎÏ13" means banks 1 and 3 active
    if (f.size() > 12) {
        for (const QChar c : f[12]) {
            if (c >= '1' && c <= '9')
                s.bankMask |= quint16(1u << (c.unicode() - '1'));
            else if (c == '0')
                s.bankMask |= quint16(1u << 9);
        }
    }

    // ---- Active on a specific channel ---- "CH027 467.6875"
    // Also matches scanner-held channels after EPG: "CH005 462.6000"
    static const QRegularExpression reActive(R"(CH(\d+)\s+([\d]+\.[\d]+))");
    const auto mActive = reActive.match(main);
    if (mActive.hasMatch()) {
        s.state          = StsStatus::State::Active;
        s.channelNumber  = mActive.captured(1).toInt();
        s.frequency      = mActive.captured(2);
        s.squelchOpen    = true;
        s.signalStrength = parseSig();
        return s;
    }

    // ---- Scanning ----
    if (main.contains("SCAN", Qt::CaseInsensitive)) {
        s.state = StsStatus::State::Scanning;
        return s;
    }

    // ---- Frequency search ----
    // field[4] contains "SEARCH" (custom/explore) or a service name (service search)
    // field[6] is a plain frequency  e.g. "25.4750"
    {
        // Map LCD label keywords → human-readable service name.
        // Checked in order; first match wins.
        static const struct { const char* keyword; const char* name; } kServices[] = {
            { "POLICE",    "Police"          },
            { "FIRE",      "Fire/Emergency"  },
            { "HAM",       "HAM Radio"       },
            { "AMATEUR",   "HAM Radio"       },
            { "MARINE",    "Marine"          },
            { "RAILROAD",  "Railroad"        },
            { "RAIL RD",   "Railroad"        },
            { "CIVIL AIR", "Civil Air"       },
            { "CIV AIR",   "Civil Air"       },
            { "MIL AIR",   "Military Air"    },
            { "MILITARY",  "Military Air"    },
            { "CB RADIO",  "CB Radio"        },
            { "FRS",       "FRS/GMRS/MURS"  },
            { "GMRS",      "FRS/GMRS/MURS"  },
            { "MURS",      "FRS/GMRS/MURS"  },
            { "RACING",    "Racing"          },
        };

        QString svcName;
        const QString labelUpper = label.toUpper();
        for (const auto& svc : kServices) {
            if (labelUpper.contains(QLatin1String(svc.keyword))) {
                svcName = QLatin1String(svc.name);
                break;
            }
        }

        const bool isSearch = label.contains("SEARCH", Qt::CaseInsensitive)
                              || !svcName.isEmpty();

        if (isSearch) {
            static const QRegularExpression reFreq(R"(([\d]+\.[\d]+))");
            const auto mFreq = reFreq.match(main);
            s.state        = StsStatus::State::Search;
            s.serviceLabel = svcName;
            if (svcName.isEmpty())
                s.channelLabel = label;   // keep "SEARCH BANK1" etc. for custom search
            else
                s.channelLabel = svcName; // show service name as label
            if (mFreq.hasMatch())
                s.frequency = mFreq.captured(1);

            // Squelch is open during search when field[20] has a signal value (1-5).
            s.signalStrength = parseSig();
            s.squelchOpen    = s.signalStrength > 0;

            return s;
        }
    }

    // Unknown state — might be hold/menu transition, volume overlay, etc.
    s.state = StsStatus::State::Unknown;
    return s;
}
