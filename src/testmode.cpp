#include "testmode.h"
#include "stspoller.h"
#include "transmissionlogger.h"
#include "appsettings.h"
#include "ctcssdcsdata.h"
#include "lcdwidget.h"
#include "mainwindow.h"

#include <QDir>
#include <QDateTime>
#include <QStringList>
#include <cstdio>

// ---------------------------------------------------------------------------
// Infrastructure
// ---------------------------------------------------------------------------

static int s_pass = 0;
static int s_fail = 0;

static void section(const char* name) {
    printf("\n[%s]\n", name);
}

static void check(const char* name, bool ok) {
    if (ok) { printf("  [PASS] %s\n", name); ++s_pass; }
    else    { printf("  [FAIL] %s\n", name); ++s_fail; }
}

static void checkStr(const char* name, const QString& got, const QString& expected) {
    bool ok = (got == expected);
    if (ok) {
        printf("  [PASS] %s\n", name);
        ++s_pass;
    } else {
        printf("  [FAIL] %s  (got '%s', expected '%s')\n",
               name, got.toUtf8().constData(), expected.toUtf8().constData());
        ++s_fail;
    }
}

static void checkInt(const char* name, int got, int expected) {
    checkStr(name, QString::number(got), QString::number(expected));
}

// ---------------------------------------------------------------------------
// STS string builders — produce exactly 21 comma-separated fields so that
// field[20] (signal strength) is always present.
// ---------------------------------------------------------------------------

static QStringList stsFields() {
    QStringList f;
    for (int i = 0; i < 21; ++i) f << QString{};
    return f;
}

static QString stsScanning(const QString& bankLabel = "BANK1") {
    QStringList f = stsFields();
    f[0] = "STS"; f[1] = "011000"; f[4] = bankLabel; f[6] = " SCAN ";
    return f.join(',');
}

static QString stsActive(const QString& chLabel, const QString& mainDisplay,
                          const QString& ctcss = {}, int sig = 3) {
    QStringList f = stsFields();
    f[0] = "STS"; f[1] = "011000";
    f[4] = chLabel; f[6] = mainDisplay; f[8] = ctcss;
    f[20] = QString::number(sig);
    return f.join(',');
}

static QString stsSearch(const QString& label, const QString& freq, int sig = 3) {
    QStringList f = stsFields();
    f[0] = "STS"; f[1] = "011000";
    f[4] = label; f[6] = freq;
    f[20] = QString::number(sig);
    return f.join(',');
}

static QString stsMenu() {
    // flags field all '1' → menu state; size only needs to be >= 7
    QStringList f;
    f << "STS" << "1111" << "Enter Frequency" << "***"
      << "Edit Tag" << "" << "Set CTCSS/DCS";
    return f.join(',');
}

// ---------------------------------------------------------------------------
// 1. STS Parsing
// ---------------------------------------------------------------------------

static void testStsParsing() {
    section("STS Parsing");

    // Scanning
    {
        auto s = StsPoller::parse(stsScanning());
        check("Scanning: state",         s.state == StsStatus::State::Scanning);
        check("Scanning: squelch closed", !s.squelchOpen);
        check("Scanning: no ctcssDcs",    s.ctcssDcs.isEmpty());
    }

    // Active — basic channel hit
    {
        auto s = StsPoller::parse(stsActive("FRS Ch13", "CH027 467.6875"));
        check("Active: state",         s.state == StsStatus::State::Active);
        check("Active: squelch open",  s.squelchOpen);
        checkInt("Active: channel number", s.channelNumber, 27);
        checkStr("Active: frequency",      s.frequency, "467.6875");
        checkStr("Active: channel label",  s.channelLabel, "FRS Ch13");
        check("Active: no ctcssDcs when f[8] empty", s.ctcssDcs.isEmpty());
    }

    // Active with CTCSS tone in field[8]
    {
        auto s = StsPoller::parse(stsActive("FRS Ch13", "CH027 467.6875", "C67.0"));
        check("Active+CTCSS: state", s.state == StsStatus::State::Active);
        checkStr("Active+CTCSS: ctcssDcs", s.ctcssDcs, "C67.0");
    }

    // Active with DCS code in field[8]
    {
        auto s = StsPoller::parse(stsActive("POLICE", "CH015 155.3400", "DCS032"));
        check("Active+DCS: state", s.state == StsStatus::State::Active);
        checkStr("Active+DCS: ctcssDcs", s.ctcssDcs, "DCS032");
    }

    // Search — custom frequency range
    {
        auto s = StsPoller::parse(stsSearch("SEARCH BANK1", "25.4750", 3));
        check("Search: state",                  s.state == StsStatus::State::Search);
        checkStr("Search: frequency",           s.frequency, "25.4750");
        check("Search: squelch open (sig > 0)", s.squelchOpen);
    }

    // Search — no signal (sig = 0) → squelch closed
    {
        auto s = StsPoller::parse(stsSearch("SEARCH BANK1", "25.4750", 0));
        check("Search (no sig): squelch closed", !s.squelchOpen);
    }

    // Service search — police
    {
        auto s = StsPoller::parse(stsSearch("POLICE SRCH", "155.3400", 2));
        check("Service search: state", s.state == StsStatus::State::Search);
        checkStr("Service search: serviceLabel", s.serviceLabel, "Police");
        check("Service search: squelch open", s.squelchOpen);
    }

    // Menu / programming mode
    {
        auto s = StsPoller::parse(stsMenu());
        check("Menu: state", s.state == StsStatus::State::Menu);
    }

    // Garbage input → Unknown
    {
        auto s = StsPoller::parse("GARBAGE,DATA,HERE");
        check("Bad input: Unknown state", s.state == StsStatus::State::Unknown);
    }

    // Empty string → Unknown
    {
        auto s = StsPoller::parse("");
        check("Empty input: Unknown state", s.state == StsStatus::State::Unknown);
    }
}

// ---------------------------------------------------------------------------
// 2. CTCSS/DCS label table
// ---------------------------------------------------------------------------

static void testCtcssDcsLabels() {
    section("CTCSS/DCS Label Table");

    checkStr("Code   0: None / All",    ctcssDcsLabel(0),   "None / All");
    checkStr("Code  64: CTCSS 67.0 Hz", ctcssDcsLabel(64),  "CTCSS 67.0 Hz");
    checkStr("Code  76: CTCSS 100.0 Hz",ctcssDcsLabel(76),  "CTCSS 100.0 Hz");
    checkStr("Code 113: CTCSS 254.1 Hz",ctcssDcsLabel(113), "CTCSS 254.1 Hz");
    checkStr("Code 128: DCS 023",       ctcssDcsLabel(128), "DCS 023");
    checkStr("Code 132: DCS 032",       ctcssDcsLabel(132), "DCS 032");
    checkStr("Code 240: No Tone",       ctcssDcsLabel(240), "No Tone");
    check("Unknown code: fallback string", ctcssDcsLabel(999).startsWith("Code "));
}

// ---------------------------------------------------------------------------
// 3. TransmissionLogger
// ---------------------------------------------------------------------------

static void testLogger() {
    section("Transmission Logger");

    // Redirect writes to a temp directory so CI doesn't pollute user Documents
    const QString origDir = AppSettings::instance().logDirectory();
    const QString tmpDir  = QDir::tempPath() + "/bcscan-test-" +
                            QString::number(QDateTime::currentMSecsSinceEpoch());
    AppSettings::instance().setLogDirectory(tmpDir);

    // begin → end: signal fires with correct data
    {
        TransmissionLogger logger;
        bool fired = false;
        TransmissionRecord rec;
        QObject::connect(&logger, &TransmissionLogger::transmissionLogged,
            [&](const TransmissionRecord& r) { fired = true; rec = r; });

        logger.beginTransmission(27, "FRS Ch13", "467.6875", "C67.0");
        check("Logger: isActive after begin",   logger.isActive());
        logger.endTransmission();
        check("Logger: not active after end",   !logger.isActive());
        check("Logger: signal emitted on end",  fired);
        checkInt("Logger: channel index",       rec.channelIndex, 27);
        checkStr("Logger: frequency",           rec.frequency,    "467.6875");
        checkStr("Logger: ctcss label",         rec.ctcssDcsLabel,"C67.0");
    }

    // begin → cancel: no signal
    {
        TransmissionLogger logger;
        bool fired = false;
        QObject::connect(&logger, &TransmissionLogger::transmissionLogged,
            [&](const TransmissionRecord&) { fired = true; });

        logger.beginTransmission(1, "Test", "462.5625", "");
        logger.cancelTransmission();
        check("Logger: not active after cancel", !logger.isActive());
        check("Logger: no signal on cancel",     !fired);
    }

    // updateCtcssDcs mid-transmission overwrites initial label
    {
        TransmissionLogger logger;
        TransmissionRecord rec;
        QObject::connect(&logger, &TransmissionLogger::transmissionLogged,
            [&](const TransmissionRecord& r) { rec = r; });

        logger.beginTransmission(10, "HAM", "146.5200", "");
        logger.updateCtcssDcs("C103.5");
        logger.endTransmission();
        checkStr("Logger: mid-tx ctcss update logged", rec.ctcssDcsLabel, "C103.5");
    }

    // updateCtcssDcs when inactive must not crash or store
    {
        TransmissionLogger logger;
        logger.updateCtcssDcs("C67.0");
        check("Logger: updateCtcssDcs when inactive is safe", !logger.isActive());
    }

    // end when not active must be a no-op
    {
        TransmissionLogger logger;
        bool fired = false;
        QObject::connect(&logger, &TransmissionLogger::transmissionLogged,
            [&](const TransmissionRecord&) { fired = true; });
        logger.endTransmission();
        check("Logger: end when inactive is safe", !fired);
    }

    AppSettings::instance().setLogDirectory(origDir);
    QDir(tmpDir).removeRecursively();
}

// ---------------------------------------------------------------------------
// 4. Settings defaults
// ---------------------------------------------------------------------------

static void testSettings() {
    section("Settings");

    // If the user has never changed these, we see the compiled-in defaults.
    // Either way, verify the stored values are within a sane operating range.
    const int skip = AppSettings::instance().autoSkipSeconds();
    check("autoSkipSeconds in [1, 3600]", skip >= 1 && skip <= 3600);

    const int minSecs = AppSettings::instance().minTransmissionSeconds();
    check("minTransmissionSeconds in [0, 60]", minSecs >= 0 && minSecs <= 60);
}

// ---------------------------------------------------------------------------
// 5. UI smoke tests  (requires QApplication + offscreen platform)
// ---------------------------------------------------------------------------

static void testUi() {
    section("UI Smoke Test");

    // LcdWidget — create, drive all setters, destroy
    {
        LcdWidget w;
        w.show();
        check("LcdWidget: constructed", true);
        w.setFrequency("462.5625");
        w.setChannelLabel("FRS Ch13");
        w.setChannelNumber(1, 27);
        w.setModulation("FM");
        w.setCtcssDcs("CTCSS 67.0 Hz");
        w.setSignalStrength(3);
        w.setState(LcdWidget::ScanState::Scanning);
        w.setStatusMessage("SCANNING");
        w.setHoldTimer(0);
        check("LcdWidget: setters complete", true);
    }
    check("LcdWidget: destroyed cleanly", true);

    // MainWindow — the heaviest test: exercises the full widget tree and the
    // destructor shutdown path fixed in scannerserial.cpp.
    {
        MainWindow w;
        w.show();
        check("MainWindow: constructed and shown", true);
    }
    check("MainWindow: destroyed cleanly", true);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int runTests() {
    printf("BCScan self-test\n");
    printf("================\n");

    testStsParsing();
    testCtcssDcsLabels();
    testLogger();
    testSettings();
    testUi();

    printf("\n================\n");
    printf("Passed: %d  Failed: %d\n", s_pass, s_fail);

    return s_fail == 0 ? 0 : 1;
}
