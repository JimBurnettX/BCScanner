#include "settingswindow.h"
#include "appsettings.h"
#include "ctcssdcsdata.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QDoubleValidator>
#include <QScrollArea>

static const char* svcSearchLabels[10] = {
    "Police","Fire/Emergency","Ham Radio","Marine","Railroad",
    "Civil Air","Military Air","CB Radio","FRS/GMRS/MURS","Racing"
};
static const char* ccBandLabels[5] = {
    "VHF Low (25-54 MHz)", "Air Band (108-137 MHz)",
    "VHF High1 (137-174 MHz)", "VHF High2 (174-225 MHz)", "UHF (400-512 MHz)"
};

SettingsWindow::SettingsWindow(ScannerSerial* scanner, QWidget* parent)
    : QDialog(parent), m_scanner(scanner)
{
    setWindowTitle("BC125AT Settings");
    setMinimumSize(600, 500);
    buildUi();
}

void SettingsWindow::buildUi() {
    auto* mainLayout = new QVBoxLayout(this);

    m_tabs = new QTabWidget;
    m_tabs->addTab(buildScannerTab(),  "Scanner");
    m_tabs->addTab(buildSearchTab(),   "Search");
    m_tabs->addTab(buildCloseCallTab(),"Close Call");
    m_tabs->addTab(buildLockoutTab(),  "Lockout Freqs");
    m_tabs->addTab(buildAppTab(),      "Application");
    mainLayout->addWidget(m_tabs);

    // Bottom bar
    auto* botLayout = new QHBoxLayout;
    m_readAllBtn  = new QPushButton("Read All from Scanner");
    m_applyBtn    = new QPushButton("Apply to Scanner");
    m_applyBtn->setDefault(true);
    auto* clearBtn = new QPushButton("Clear All Memory...");
    m_statusLabel  = new QLabel;
    botLayout->addWidget(m_readAllBtn);
    botLayout->addWidget(m_applyBtn);
    botLayout->addWidget(clearBtn);
    botLayout->addStretch();
    botLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(botLayout);

    connect(m_readAllBtn, &QPushButton::clicked, this, &SettingsWindow::onReadAll);
    connect(m_applyBtn,   &QPushButton::clicked, this, &SettingsWindow::onApply);
    connect(clearBtn,     &QPushButton::clicked, this, &SettingsWindow::onClearAllMemory);
}

QWidget* SettingsWindow::buildScannerTab() {
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto* w = new QWidget;
    auto* layout = new QVBoxLayout(w);

    // Volume & Squelch
    auto* avBox = new QGroupBox("Audio");
    auto* avGrid = new QGridLayout(avBox);
    avGrid->addWidget(new QLabel("Volume (0-15):"), 0, 0);
    m_volSlider = new QSlider(Qt::Horizontal);
    m_volSlider->setRange(0, 15);
    m_volSlider->setValue(5);
    m_volLabel = new QLabel("5");
    m_volLabel->setFixedWidth(25);
    avGrid->addWidget(m_volSlider, 0, 1);
    avGrid->addWidget(m_volLabel, 0, 2);

    avGrid->addWidget(new QLabel("Squelch (0=Open, 15=Closed):"), 1, 0);
    m_sqlSlider = new QSlider(Qt::Horizontal);
    m_sqlSlider->setRange(0, 15);
    m_sqlSlider->setValue(5);
    m_sqlLabel = new QLabel("5");
    m_sqlLabel->setFixedWidth(25);
    avGrid->addWidget(m_sqlSlider, 1, 1);
    avGrid->addWidget(m_sqlLabel, 1, 2);
    layout->addWidget(avBox);

    connect(m_volSlider, &QSlider::valueChanged, [this](int v){ m_volLabel->setText(QString::number(v)); });
    connect(m_sqlSlider, &QSlider::valueChanged, [this](int v){ m_sqlLabel->setText(QString::number(v)); });

    // Display
    auto* dispBox = new QGroupBox("Display");
    auto* dispForm = new QFormLayout(dispBox);
    m_backlightCombo = new QComboBox;
    m_backlightCombo->addItem("Always On",    "AO");
    m_backlightCombo->addItem("Always Off",   "AF");
    m_backlightCombo->addItem("Keypress",      "KY");
    m_backlightCombo->addItem("Squelch",       "SQ");
    m_backlightCombo->addItem("Key + Squelch", "KS");
    dispForm->addRow("Backlight:", m_backlightCombo);

    m_contrastSpin = new QSpinBox;
    m_contrastSpin->setRange(1, 15);
    m_contrastSpin->setValue(8);
    dispForm->addRow("LCD Contrast (1-15):", m_contrastSpin);
    layout->addWidget(dispBox);

    // Keypad
    auto* kbBox = new QGroupBox("Keypad");
    auto* kbForm = new QFormLayout(kbBox);
    m_keyBeepCombo = new QComboBox;
    m_keyBeepCombo->addItem("Auto", 0);
    m_keyBeepCombo->addItem("Off",  99);
    kbForm->addRow("Key Beep:", m_keyBeepCombo);
    m_keyLockCheck = new QCheckBox("Enable Key Lock");
    kbForm->addRow(m_keyLockCheck);
    layout->addWidget(kbBox);

    // Scan settings
    auto* scanBox = new QGroupBox("Scan Settings");
    auto* scanForm = new QFormLayout(scanBox);
    m_priorityCombo = new QComboBox;
    m_priorityCombo->addItem("Off",      0);
    m_priorityCombo->addItem("On",       1);
    m_priorityCombo->addItem("Plus On",  2);
    m_priorityCombo->addItem("Do Not Disturb", 3);
    scanForm->addRow("Priority Mode:", m_priorityCombo);
    layout->addWidget(scanBox);

    // Battery
    auto* batBox = new QGroupBox("Battery");
    auto* batForm = new QFormLayout(batBox);
    m_chargeTimeSpin = new QSpinBox;
    m_chargeTimeSpin->setRange(1, 16);
    m_chargeTimeSpin->setValue(16);
    m_chargeTimeSpin->setSuffix(" hours");
    batForm->addRow("Charge Time:", m_chargeTimeSpin);
    layout->addWidget(batBox);

    // Weather
    auto* wxBox = new QGroupBox("Weather");
    auto* wxLayout2 = new QVBoxLayout(wxBox);
    m_wxAlertCheck = new QCheckBox("Weather Alert Priority");
    wxLayout2->addWidget(m_wxAlertCheck);
    layout->addWidget(wxBox);

    // Bank scan group
    auto* bankBox = new QGroupBox("Active Scan Banks");
    auto* bankLayout = new QHBoxLayout(bankBox);
    for (int i = 0; i < 10; ++i) {
        int dispNum = (i == 9) ? 0 : i + 1;
        m_bankChecks[i] = new QCheckBox(QString("Bank %1").arg(dispNum));
        m_bankChecks[i]->setChecked(true);
        bankLayout->addWidget(m_bankChecks[i]);
    }
    layout->addWidget(bankBox);
    layout->addStretch();

    scroll->setWidget(w);
    return scroll;
}

QWidget* SettingsWindow::buildSearchTab() {
    auto* w = new QWidget;
    auto* layout = new QVBoxLayout(w);

    auto* searchBox = new QGroupBox("Search / Close Call Settings");
    auto* searchForm = new QFormLayout(searchBox);

    m_searchDelayCombo = new QComboBox;
    for (int d : delayValues())
        m_searchDelayCombo->addItem(delayLabel(d), d);
    m_searchDelayCombo->setCurrentIndex(2);
    searchForm->addRow("Scan Delay:", m_searchDelayCombo);

    m_codeSearchCheck = new QCheckBox("Search for CTCSS/DCS codes");
    searchForm->addRow(m_codeSearchCheck);
    layout->addWidget(searchBox);

    // Service search groups
    auto* svcBox = new QGroupBox("Service Search Banks");
    auto* svcGrid = new QGridLayout(svcBox);
    for (int i = 0; i < 10; ++i) {
        // Service search order from protocol (bit 0=Police at rightmost, bit 9=Racing at leftmost)
        // Display order in the SSG bits (left to right): Racing,FRS,CB,MilAir,CivAir,Railroad,Marine,Ham,Fire,Police
        m_svcSearchChecks[i] = new QCheckBox(svcSearchLabels[i]);
        m_svcSearchChecks[i]->setChecked(true);
        svcGrid->addWidget(m_svcSearchChecks[i], i/2, i%2);
    }
    layout->addWidget(svcBox);

    // Custom search group enable/disable
    auto* custGrpBox = new QGroupBox("Custom Search Ranges (Enable/Disable)");
    auto* custGrpGrid = new QGridLayout(custGrpBox);
    for (int i = 0; i < 10; ++i) {
        m_custSearchChecks[i] = new QCheckBox(QString("Range %1").arg(i+1));
        m_custSearchChecks[i]->setChecked(true);
        custGrpGrid->addWidget(m_custSearchChecks[i], i/5, i%5);
    }
    layout->addWidget(custGrpBox);

    // Custom search ranges
    auto* custBox = new QGroupBox("Custom Search Range Frequencies");
    auto* custLayout = new QVBoxLayout(custBox);
    m_custSearchTable = new QTableWidget(10, 3);
    m_custSearchTable->setHorizontalHeaderLabels({"Range", "Lower (MHz)", "Upper (MHz)"});
    m_custSearchTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_custSearchTable->setFixedHeight(200);
    for (int i = 0; i < 10; ++i) {
        auto* numItem = new QTableWidgetItem(QString::number(i+1));
        numItem->setFlags(numItem->flags() & ~Qt::ItemIsEditable);
        numItem->setTextAlignment(Qt::AlignCenter);
        m_custSearchTable->setItem(i, 0, numItem);
        m_custSearchTable->setItem(i, 1, new QTableWidgetItem("25.0000"));
        m_custSearchTable->setItem(i, 2, new QTableWidgetItem("512.0000"));
    }
    custLayout->addWidget(m_custSearchTable);
    layout->addWidget(custBox);
    layout->addStretch();
    return w;
}

QWidget* SettingsWindow::buildCloseCallTab() {
    auto* w = new QWidget;
    auto* layout = new QVBoxLayout(w);

    auto* ccBox = new QGroupBox("Close Call Settings");
    auto* ccForm = new QFormLayout(ccBox);

    m_ccModeCombo = new QComboBox;
    m_ccModeCombo->addItem("Off",          0);
    m_ccModeCombo->addItem("CC Priority",  1);
    m_ccModeCombo->addItem("CC Do Not Disturb", 2);
    ccForm->addRow("Close Call Mode:", m_ccModeCombo);

    m_ccAlertBeepCheck  = new QCheckBox("Alert Beep");
    m_ccAlertLightCheck = new QCheckBox("Alert Light");
    m_ccLockoutCheck    = new QCheckBox("Lockout CC Hits with Scan");
    ccForm->addRow(m_ccAlertBeepCheck);
    ccForm->addRow(m_ccAlertLightCheck);
    ccForm->addRow(m_ccLockoutCheck);
    layout->addWidget(ccBox);

    auto* bandBox = new QGroupBox("Close Call Bands");
    auto* bandLayout = new QVBoxLayout(bandBox);
    // CC_BAND bit order (from protocol): bit0=VHF LOW1, bit1=AIR BAND, bit2=VHF HIGH1, bit3=VHF HIGH2, bit4=UHF
    for (int i = 0; i < 5; ++i) {
        m_ccBandChecks[i] = new QCheckBox(ccBandLabels[i]);
        m_ccBandChecks[i]->setChecked(true);
        bandLayout->addWidget(m_ccBandChecks[i]);
    }
    layout->addWidget(bandBox);
    layout->addStretch();
    return w;
}

QWidget* SettingsWindow::buildLockoutTab() {
    auto* w = new QWidget;
    auto* layout = new QVBoxLayout(w);

    layout->addWidget(new QLabel(
        "Global lockout frequencies are skipped during all search/scan modes."));

    m_lockoutTable = new QTableWidget(0, 1);
    m_lockoutTable->setHorizontalHeaderLabels({"Locked Out Frequency (MHz)"});
    m_lockoutTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_lockoutTable->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_lockoutTable);

    auto* addLayout = new QHBoxLayout;
    m_lockoutFreqEdit = new QLineEdit;
    m_lockoutFreqEdit->setPlaceholderText("Frequency in MHz, e.g. 462.5625");
    m_lockoutFreqEdit->setValidator(new QDoubleValidator(25.0, 512.0, 4, m_lockoutFreqEdit));
    auto* addBtn    = new QPushButton("Add Lockout");
    auto* removeBtn = new QPushButton("Remove Selected");
    auto* loadBtn   = new QPushButton("Load from Scanner");
    addLayout->addWidget(m_lockoutFreqEdit);
    addLayout->addWidget(addBtn);
    addLayout->addWidget(removeBtn);
    addLayout->addWidget(loadBtn);
    layout->addLayout(addLayout);

    connect(addBtn,    &QPushButton::clicked, this, &SettingsWindow::onAddLockoutFreq);
    connect(removeBtn, &QPushButton::clicked, this, &SettingsWindow::onRemoveLockoutFreq);
    connect(loadBtn,   &QPushButton::clicked, this, &SettingsWindow::onLoadLockoutList);
    return w;
}

QWidget* SettingsWindow::buildAppTab() {
    auto* w = new QWidget;
    auto* layout = new QFormLayout(w);

    m_minTransSpin = new QSpinBox;
    m_minTransSpin->setRange(1, 60);
    m_minTransSpin->setValue(AppSettings::instance().minTransmissionSeconds());
    m_minTransSpin->setSuffix(" seconds");
    layout->addRow("Minimum Transmission Duration to Log:", m_minTransSpin);

    m_autoSkipSpin = new QSpinBox;
    m_autoSkipSpin->setRange(10, 300);
    m_autoSkipSpin->setValue(AppSettings::instance().autoSkipSeconds());
    m_autoSkipSpin->setSuffix(" seconds");
    layout->addRow("Skip & Resume Scanning/Search After:", m_autoSkipSpin);

    auto* logLayout = new QHBoxLayout;
    m_logDirEdit = new QLineEdit(AppSettings::instance().logDirectory());
    auto* browseBtn = new QPushButton("Browse...");
    logLayout->addWidget(m_logDirEdit);
    logLayout->addWidget(browseBtn);
    layout->addRow("Transmission Log Directory:", logLayout);

    m_autoConnectCheck = new QCheckBox("Auto-connect to last port on startup");
    m_autoConnectCheck->setChecked(AppSettings::instance().autoConnectOnStart());
    layout->addRow(m_autoConnectCheck);

    connect(browseBtn, &QPushButton::clicked, this, &SettingsWindow::onBrowseLogDir);

    layout->addRow(new QLabel(
        "<i>Note: App settings are saved automatically when you click Apply.</i>"));
    return w;
}

void SettingsWindow::onReadAll() {
    if (!m_scanner->isConnected()) {
        setStatus("Not connected to scanner.", false);
        return;
    }
    setStatus("Reading settings from scanner...");
    setAllEnabled(false);

    auto finalize = [this]{ setAllEnabled(true); };

    m_scanner->enterProgramMode([this, finalize](bool ok, const QString&){
        if (!ok) { setStatus("Failed to enter program mode.", false); finalize(); return; }

        // Read all settings sequentially
        m_scanner->getVolume([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 2) { m_volSlider->setValue(p[1].toInt()); }
            }
        });
        m_scanner->getSquelch([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 2) { m_sqlSlider->setValue(p[1].toInt()); }
            }
        });
        m_scanner->getBacklight([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 2) {
                    int i = m_backlightCombo->findData(p[1]);
                    if (i >= 0) m_backlightCombo->setCurrentIndex(i);
                }
            }
        });
        m_scanner->getKeyBeep([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 3) {
                    int lvl = p[1].toInt();
                    m_keyBeepCombo->setCurrentIndex(lvl == 99 ? 1 : 0);
                    m_keyLockCheck->setChecked(p[2].toInt() == 1);
                }
            }
        });
        m_scanner->getPriorityMode([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 2) m_priorityCombo->setCurrentIndex(p[1].toInt());
            }
        });
        m_scanner->getLcdContrast([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 2) m_contrastSpin->setValue(p[1].toInt());
            }
        });
        m_scanner->getBatteryInfo([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 2) m_chargeTimeSpin->setValue(p[1].toInt());
            }
        });
        m_scanner->getWeatherSettings([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 2) m_wxAlertCheck->setChecked(p[1].toInt() == 1);
            }
        });
        m_scanner->getScanGroup([this](bool ok, const QString& r){
            if (ok) {
                ScanGroupStatus sg = ScannerSerial::parseScanGroup(r);
                for (int i = 0; i < 10; ++i) m_bankChecks[i]->setChecked(sg.banks[i]);
            }
        });
        m_scanner->getSearchCloseCall([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 3) {
                    int d = p[1].toInt();
                    for (int i = 0; i < m_searchDelayCombo->count(); ++i) {
                        if (m_searchDelayCombo->itemData(i).toInt() == d) {
                            m_searchDelayCombo->setCurrentIndex(i); break;
                        }
                    }
                    m_codeSearchCheck->setChecked(p[2].toInt() == 1);
                }
            }
        });
        m_scanner->getCloseCallSettings([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 6) {
                    m_ccModeCombo->setCurrentIndex(p[1].toInt());
                    m_ccAlertBeepCheck->setChecked(p[2].toInt() == 1);
                    m_ccAlertLightCheck->setChecked(p[3].toInt() == 1);
                    QString bands = p[4];
                    for (int i = 0; i < 5 && i < bands.size(); ++i)
                        m_ccBandChecks[i]->setChecked(bands[bands.size()-1-i] == '1');
                    m_ccLockoutCheck->setChecked(p[5].toInt() == 1);
                }
            }
        });
        m_scanner->getServiceSearchGroup([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 2) {
                    QString bits = p[1];
                    // Bit order: leftmost = Racing (idx 9), rightmost = Police (idx 0)
                    for (int i = 0; i < 10 && i < bits.size(); ++i)
                        m_svcSearchChecks[9-i]->setChecked(bits[i] == '0');
                }
            }
        });
        m_scanner->getCustomSearchGroup([this](bool ok, const QString& r){
            if (ok) {
                QStringList p = r.split(',');
                if (p.size() >= 2) {
                    QString bits = p[1];
                    for (int i = 0; i < 10 && i < bits.size(); ++i)
                        m_custSearchChecks[i]->setChecked(bits[i] == '0');
                }
            }
        });

        // Read custom search ranges
        for (int i = 1; i <= 10; ++i) {
            m_scanner->getCustomSearchRange(i, [this, i](bool ok, const QString& r){
                if (!ok) return;
                QStringList p = r.split(',');
                if (p.size() >= 3) {
                    double lo = p[1].toLongLong() / 10000.0;
                    double hi = p[2].toLongLong() / 10000.0;
                    if (auto* item = m_custSearchTable->item(i-1, 1))
                        item->setText(QString::number(lo, 'f', 4));
                    if (auto* item = m_custSearchTable->item(i-1, 2))
                        item->setText(QString::number(hi, 'f', 4));
                }
            });
        }

        m_scanner->exitProgramMode([this, finalize](bool, const QString&){
            finalize();
            setStatus("Settings read successfully.");
        });
    });
}

void SettingsWindow::onApply() {
    // Save app settings
    AppSettings::instance().setMinTransmissionSeconds(m_minTransSpin->value());
    AppSettings::instance().setAutoSkipSeconds(m_autoSkipSpin->value());
    AppSettings::instance().setLogDirectory(m_logDirEdit->text());
    AppSettings::instance().setAutoConnectOnStart(m_autoConnectCheck->isChecked());

    if (!m_scanner->isConnected()) {
        setStatus("App settings saved. (Not connected to scanner - scanner settings not applied)");
        return;
    }

    setStatus("Applying settings to scanner...");
    setAllEnabled(false);

    // Volume and Squelch don't need program mode
    m_scanner->setVolume(m_volSlider->value());
    m_scanner->setSquelch(m_sqlSlider->value());

    m_scanner->enterProgramMode([this](bool ok, const QString&){
        if (!ok) {
            setStatus("Failed to enter program mode.", false);
            setAllEnabled(true);
            return;
        }

        m_scanner->setBacklight(m_backlightCombo->currentData().toString());
        m_scanner->setKeyBeep(m_keyBeepCombo->currentData().toInt(), m_keyLockCheck->isChecked());
        m_scanner->setPriorityMode(m_priorityCombo->currentData().toInt());
        m_scanner->setLcdContrast(m_contrastSpin->value());
        m_scanner->setBatteryChargeTime(m_chargeTimeSpin->value());
        m_scanner->setWeatherAlertPriority(m_wxAlertCheck->isChecked());

        // Scan group
        ScanGroupStatus sg;
        for (int i = 0; i < 10; ++i) sg.banks[i] = m_bankChecks[i]->isChecked();
        m_scanner->setScanGroup(sg);

        // Search/Close call
        int searchDelay = m_searchDelayCombo->currentData().toInt();
        m_scanner->setSearchCloseCall(searchDelay, m_codeSearchCheck->isChecked());

        // Close call
        QString ccBands;
        for (int i = 4; i >= 0; --i) ccBands += m_ccBandChecks[i]->isChecked() ? '1' : '0';
        m_scanner->setCloseCallSettings(
            m_ccModeCombo->currentIndex(),
            m_ccAlertBeepCheck->isChecked(),
            m_ccAlertLightCheck->isChecked(),
            ccBands,
            m_ccLockoutCheck->isChecked()
        );

        // Service search
        QString svcBits;
        for (int i = 9; i >= 0; --i) svcBits += m_svcSearchChecks[i]->isChecked() ? '0' : '1';
        m_scanner->setServiceSearchGroup(svcBits);

        // Custom search group
        QString custBits;
        for (int i = 0; i < 10; ++i) custBits += m_custSearchChecks[i]->isChecked() ? '0' : '1';
        m_scanner->setCustomSearchGroup(custBits);

        // Custom search ranges
        for (int i = 0; i < 10; ++i) {
            auto* loItem = m_custSearchTable->item(i, 1);
            auto* hiItem = m_custSearchTable->item(i, 2);
            if (loItem && hiItem) {
                long long lo = static_cast<long long>(loItem->text().toDouble() * 10000);
                long long hi = static_cast<long long>(hiItem->text().toDouble() * 10000);
                m_scanner->setCustomSearchRange(i+1, lo, hi);
            }
        }

        m_scanner->exitProgramMode([this](bool, const QString&){
            setAllEnabled(true);
            setStatus("Settings applied successfully.");
        });
    });
}

void SettingsWindow::onClearAllMemory() {
    if (QMessageBox::question(this, "Clear All Memory",
        "WARNING: This will erase ALL programmed channels and reset to factory defaults.\n\nAre you sure?",
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    if (QMessageBox::question(this, "Clear All Memory - Confirm",
        "This operation takes several seconds and cannot be undone.\nProceed?",
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    setStatus("Clearing all memory...");
    setAllEnabled(false);

    m_scanner->enterProgramMode([this](bool ok, const QString&){
        if (!ok) { setStatus("Failed to enter program mode.", false); setAllEnabled(true); return; }
        m_scanner->clearAllMemory([this](bool ok2, const QString&){
            m_scanner->exitProgramMode([this, ok2](bool, const QString&){
                setAllEnabled(true);
                if (ok2) setStatus("All memory cleared.");
                else     setStatus("Failed to clear memory.", false);
            });
        });
    });
}

void SettingsWindow::onAddLockoutFreq() {
    QString freqStr = m_lockoutFreqEdit->text().trimmed();
    if (freqStr.isEmpty()) return;
    bool ok;
    double mhz = freqStr.toDouble(&ok);
    if (!ok || mhz < 25.0 || mhz > 512.0) {
        setStatus("Invalid frequency (25-512 MHz).", false); return;
    }
    long long raw = static_cast<long long>(mhz * 10000);

    m_scanner->enterProgramMode([this, raw, freqStr](bool ok, const QString&){
        if (!ok) { setStatus("Failed to enter program mode.", false); return; }
        m_scanner->lockoutFrequency(raw, [this, freqStr](bool ok2, const QString&){
            m_scanner->exitProgramMode([this, ok2, freqStr](bool, const QString&){
                if (ok2) {
                    int row = m_lockoutTable->rowCount();
                    m_lockoutTable->insertRow(row);
                    m_lockoutTable->setItem(row, 0, new QTableWidgetItem(freqStr + " MHz"));
                    setStatus(QString("Locked out %1 MHz.").arg(freqStr));
                    m_lockoutFreqEdit->clear();
                } else {
                    setStatus("Failed to add lockout frequency.", false);
                }
            });
        });
    });
}

void SettingsWindow::onRemoveLockoutFreq() {
    int row = m_lockoutTable->currentRow();
    if (row < 0) return;
    QString text = m_lockoutTable->item(row, 0)->text();
    double mhz = text.split(' ')[0].toDouble();
    long long raw = static_cast<long long>(mhz * 10000);

    m_scanner->enterProgramMode([this, raw, row](bool ok, const QString&){
        if (!ok) { setStatus("Failed to enter program mode.", false); return; }
        m_scanner->unlockFrequency(raw, [this, row](bool ok2, const QString&){
            m_scanner->exitProgramMode([this, ok2, row](bool, const QString&){
                if (ok2) {
                    m_lockoutTable->removeRow(row);
                    setStatus("Frequency unlocked.");
                } else {
                    setStatus("Failed to remove lockout.", false);
                }
            });
        });
    });
}

void SettingsWindow::onLoadLockoutList() {
    // GLF command reads lockout frequencies iteratively
    setStatus("Loading lockout frequencies...");
    m_scanner->enterProgramMode([this](bool ok, const QString&){
        if (!ok) { setStatus("Failed to enter program mode.", false); return; }

        m_lockoutTable->setRowCount(0);
        auto loadNext = std::make_shared<std::function<void()>>();
        *loadNext = [this, loadNext](){
            m_scanner->sendCommand("GLF,***", [this, loadNext](bool ok, const QString& resp){
                if (!ok || resp == "GLF,-1") {
                    m_scanner->exitProgramMode([this](bool, const QString&){
                        setStatus("Lockout frequencies loaded.");
                    });
                    return;
                }
                QStringList p = resp.split(',');
                if (p.size() >= 2 && p[1] != "-1") {
                    double mhz = p[1].toLongLong() / 10000.0;
                    int row = m_lockoutTable->rowCount();
                    m_lockoutTable->insertRow(row);
                    m_lockoutTable->setItem(row, 0,
                        new QTableWidgetItem(QString::number(mhz, 'f', 4) + " MHz"));
                    (*loadNext)();
                } else {
                    m_scanner->exitProgramMode([this](bool, const QString&){
                        setStatus("Lockout frequencies loaded.");
                    });
                }
            });
        };
        (*loadNext)();
    });
}

void SettingsWindow::onBrowseLogDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Log Directory",
                                                     m_logDirEdit->text());
    if (!dir.isEmpty()) m_logDirEdit->setText(dir);
}

void SettingsWindow::setStatus(const QString& msg, bool ok) {
    m_statusLabel->setText(msg);
    m_statusLabel->setStyleSheet(ok ? "" : "color: #ff4444;");
}

void SettingsWindow::setAllEnabled(bool enabled) {
    m_readAllBtn->setEnabled(enabled);
    m_applyBtn->setEnabled(enabled);
}
