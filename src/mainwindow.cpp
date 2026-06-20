#include "mainwindow.h"
#include "programwindow.h"
#include "settingswindow.h"
#include "appsettings.h"
#include "ctcssdcsdata.h"
#include "stspoller.h"
#include <QBoxLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSlider>
#include <QPlainTextEdit>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QSerialPortInfo>
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QSplitter>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_scanner = new ScannerSerial(this);
    m_logger  = new TransmissionLogger(this);

    m_autoSkipTimer = new QTimer(this);
    m_autoSkipTimer->setSingleShot(true);

    m_holdPollTimer = new QTimer(this);
    m_holdPollTimer->setInterval(1000);

    m_scanIdleTimer = new QTimer(this);
    m_scanIdleTimer->setSingleShot(true);
    m_scanIdleTimer->setInterval(30000);
    connect(m_scanIdleTimer, &QTimer::timeout, this, [this](){
        m_lcd->setFrequency("--- . ----");
    });

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(3000);

    memset(m_channelCacheValid, 0, sizeof(m_channelCacheValid));

    buildUi();
    applyStyleSheet();
    setConnectedState(false);

    // Connect scanner signals
    connect(m_scanner, &ScannerSerial::connected,         this, &MainWindow::onScannerConnected);
    connect(m_scanner, &ScannerSerial::disconnected,      this, &MainWindow::onScannerDisconnected);
    connect(m_scanner, &ScannerSerial::serialError,       this, &MainWindow::onScannerError);
    connect(m_scanner, &ScannerSerial::commandSent,       this, &MainWindow::onCommandSent);
    connect(m_scanner, &ScannerSerial::responseReceived,  this, &MainWindow::onResponseReceived);

    connect(m_autoSkipTimer, &QTimer::timeout, this, &MainWindow::onAutoSkipTimer);
    connect(m_holdPollTimer, &QTimer::timeout, this, &MainWindow::onHoldPollTimer);

    connect(m_refreshTimer, &QTimer::timeout, this, [this](){
        QString current = m_portCombo->currentText();
        m_portCombo->clear();
        for (const auto& port : QSerialPortInfo::availablePorts()) {
            m_portCombo->addItem(port.portName() + " - " + port.description(), port.portName());
        }
        // Restore selection
        int idx = m_portCombo->findData(current);
        if (idx < 0) idx = m_portCombo->findText(current);
        if (idx >= 0) m_portCombo->setCurrentIndex(idx);
    });
    m_refreshTimer->start();

    connect(m_logger, &TransmissionLogger::transmissionLogged,
            this, &MainWindow::onTransmissionLogged);

    m_stsPoller = new StsPoller(m_scanner, this);
    connect(m_stsPoller, &StsPoller::statusUpdated, this, &MainWindow::onStsStatusUpdated);
    connect(m_stsPoller, &StsPoller::squelchOpened, this, &MainWindow::onSquelchOpened);
    connect(m_stsPoller, &StsPoller::squelchClosed, this, &MainWindow::onSquelchClosed);

    // Restore window size
    resize(AppSettings::instance().windowWidth(),
           AppSettings::instance().windowHeight());

    // Auto-connect
    QString savedPort = AppSettings::instance().serialPort();
    if (AppSettings::instance().autoConnectOnStart() && !savedPort.isEmpty()) {
        int idx = m_portCombo->findData(savedPort);
        if (idx >= 0) {
            m_portCombo->setCurrentIndex(idx);
            QTimer::singleShot(500, this, &MainWindow::onConnectClicked);
        }
    }
}

MainWindow::~MainWindow() {}

void MainWindow::buildUi() {
    setWindowTitle("BCScan - Uniden BC125AT Scanner Controller");
    setMinimumSize(760, 580);

    // Menu bar
    auto* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Settings", this, &MainWindow::onSettingsClicked);
    fileMenu->addSeparator();
    fileMenu->addAction("&Quit", this, &QMainWindow::close, QKeySequence::Quit);

    auto* scanMenu = menuBar()->addMenu("&Scanner");
    scanMenu->addAction("&Scan",    this, &MainWindow::onScanClicked,    QKeySequence("Ctrl+S"));
    scanMenu->addAction("&Hold",    this, &MainWindow::onHoldClicked,    QKeySequence("Ctrl+H"));
    scanMenu->addAction("&Program", this, &MainWindow::onProgramClicked, QKeySequence("Ctrl+P"));
    scanMenu->addSeparator();
    scanMenu->addAction("S&earch",        this, &MainWindow::onSearchClicked, QKeySequence("Ctrl+F"));
    scanMenu->addAction("&Debug Console", this, &MainWindow::onDebugClicked,  QKeySequence("Ctrl+D"));

    auto* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About", this, [this](){
        QMessageBox::about(this, "About BCScan",
            "<b>BCScan</b><br>Uniden BC125AT Scanner Controller<br><br>"
            "Programs and controls the BC125AT via USB serial connection.<br><br>"
            "Serial protocol: 9600 baud, 8N1");
    });

    buildToolbar();

    // Central widget
    auto* central = new QWidget;
    setCentralWidget(central);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8, 4, 8, 4);
    mainLayout->setSpacing(6);

    m_lcd = new LcdWidget;
    mainLayout->addWidget(m_lcd, 2);

    // Channel navigation bar
    buildChannelNavBar(mainLayout);

    // Control buttons
    buildButtonPanel(mainLayout);

    // Audio controls row
    auto* audioWidget = new QWidget;
    auto* audioLayout = new QHBoxLayout(audioWidget);
    audioLayout->setContentsMargins(0, 0, 0, 0);

    auto* volBox = new QGroupBox("Volume");
    auto* volLayout = new QHBoxLayout(volBox);
    m_volSlider = new QSlider(Qt::Horizontal);
    m_volSlider->setRange(0, 15);
    m_volSlider->setValue(5);
    m_volLabel = new QLabel("5");
    m_volLabel->setFixedWidth(20);
    volLayout->addWidget(m_volSlider);
    volLayout->addWidget(m_volLabel);
    audioLayout->addWidget(volBox);

    auto* sqlBox = new QGroupBox("Squelch");
    auto* sqlLayout = new QHBoxLayout(sqlBox);
    m_sqlSlider = new QSlider(Qt::Horizontal);
    m_sqlSlider->setRange(0, 15);
    m_sqlSlider->setValue(5);
    m_sqlLabel = new QLabel("5");
    m_sqlLabel->setFixedWidth(20);
    sqlLayout->addWidget(m_sqlSlider);
    sqlLayout->addWidget(m_sqlLabel);
    audioLayout->addWidget(sqlBox);

    connect(m_volSlider, &QSlider::valueChanged, this, [this](int v){
        m_volLabel->setText(QString::number(v));
        if (m_scanner->isConnected()) m_scanner->setVolume(v);
    });
    connect(m_sqlSlider, &QSlider::valueChanged, this, [this](int v){
        m_sqlLabel->setText(QString::number(v));
        if (m_scanner->isConnected()) m_scanner->setSquelch(v);
    });
    mainLayout->addWidget(audioWidget);

    // Log tabs: Activity + Transmissions
    m_logTabs = new QTabWidget;
    m_logTabs->setDocumentMode(true);

    QFont logFont("Courier New", 9);

    m_activityLog = new QPlainTextEdit;
    m_activityLog->setReadOnly(true);
    m_activityLog->setMaximumBlockCount(500);
    m_activityLog->setFont(logFont);
    m_logTabs->addTab(m_activityLog, "Activity Log");

    m_txLogView = new QPlainTextEdit;
    m_txLogView->setReadOnly(true);
    m_txLogView->setMaximumBlockCount(1000);
    m_txLogView->setFont(logFont);
    m_txLogView->setPlaceholderText(
        "Transmission log — hits are logged automatically via STS polling. "
        "Press HOLD to manually lock and log a channel.");
    m_logTabs->addTab(m_txLogView, "Transmissions (0)");

    mainLayout->addWidget(m_logTabs, 1);

    statusBar()->showMessage("Not connected");
}

void MainWindow::buildToolbar() {
    auto* tb = addToolBar("Connection");
    tb->setMovable(false);
    tb->setIconSize(QSize(16, 16));

    tb->addWidget(new QLabel("  Port: "));
    m_portCombo = new QComboBox;
    m_portCombo->setMinimumWidth(220);
    for (const auto& port : QSerialPortInfo::availablePorts()) {
        m_portCombo->addItem(port.portName() + " - " + port.description(), port.portName());
    }
    // Pre-select saved port
    QString savedPort = AppSettings::instance().serialPort();
    if (!savedPort.isEmpty()) {
        int idx = m_portCombo->findData(savedPort);
        if (idx >= 0) m_portCombo->setCurrentIndex(idx);
    }
    tb->addWidget(m_portCombo);

    m_connectBtn = new QPushButton("Connect");
    m_connectBtn->setFixedWidth(80);
    tb->addWidget(m_connectBtn);

    m_disconnectBtn = new QPushButton("Disconnect");
    m_disconnectBtn->setFixedWidth(90);
    tb->addWidget(m_disconnectBtn);

    tb->addSeparator();

    m_connStatusLabel = new QLabel("  DISCONNECTED  ");
    m_connStatusLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_connStatusLabel->setAlignment(Qt::AlignCenter);
    m_connStatusLabel->setFixedWidth(130);
    tb->addWidget(m_connStatusLabel);

    m_modelLabel = new QLabel("  ");
    tb->addWidget(m_modelLabel);

    connect(m_connectBtn,    &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
}

void MainWindow::buildChannelNavBar(QBoxLayout* parent) {
    auto* navBox = new QGroupBox("Channel Tracker  —  navigate here to match what the scanner is on, then press HOLD to log");
    auto* navLayout = new QHBoxLayout(navBox);
    navLayout->setSpacing(4);

    auto* bankPrev = new QPushButton("◀ Bank");
    auto* bankNext = new QPushButton("Bank ▶");
    auto* chPrev   = new QPushButton("◀ CH");
    auto* chNext   = new QPushButton("CH ▶");
    bankPrev->setFixedWidth(70); bankNext->setFixedWidth(70);
    chPrev->setFixedWidth(55);   chNext->setFixedWidth(55);

    m_navBankLabel = new QLabel("BNK 1");
    m_navChLabel   = new QLabel("CH 01");
    m_navFreqLabel = new QLabel("--- . ---- MHz");
    m_navNameLabel = new QLabel("---");

    m_navBankLabel->setFixedWidth(55);
    m_navChLabel->setFixedWidth(45);
    m_navFreqLabel->setFixedWidth(130);
    m_navNameLabel->setMinimumWidth(100);

    QFont navFont("Courier New", 10);
    navFont.setBold(true);
    for (QLabel* l : {m_navBankLabel, m_navChLabel, m_navFreqLabel, m_navNameLabel})
        l->setFont(navFont);
    m_navFreqLabel->setStyleSheet("color: #00cc00;");

    m_loadCacheBtn = new QPushButton("Load Channels from Scanner");
    m_loadCacheBtn->setToolTip("Reads all programmed channels into the app (requires briefly entering program mode)");

    navLayout->addWidget(bankPrev);
    navLayout->addWidget(m_navBankLabel);
    navLayout->addWidget(bankNext);
    navLayout->addWidget(new QLabel("|"));
    navLayout->addWidget(chPrev);
    navLayout->addWidget(m_navChLabel);
    navLayout->addWidget(chNext);
    navLayout->addWidget(new QLabel("|"));
    navLayout->addWidget(m_navFreqLabel);
    navLayout->addWidget(m_navNameLabel);
    navLayout->addStretch();
    navLayout->addWidget(m_loadCacheBtn);

    connect(bankPrev,      &QPushButton::clicked, this, &MainWindow::onBankPrev);
    connect(bankNext,      &QPushButton::clicked, this, &MainWindow::onBankNext);
    connect(chPrev,        &QPushButton::clicked, this, &MainWindow::onChannelPrev);
    connect(chNext,        &QPushButton::clicked, this, &MainWindow::onChannelNext);
    connect(m_loadCacheBtn,&QPushButton::clicked, this, &MainWindow::onLoadChannelCache);

    parent->addWidget(navBox);
}

void MainWindow::buildButtonPanel(QBoxLayout* parent) {
    // Number key buttons row — each sends KEY,#,P to the scanner
    auto* bankBox = new QGroupBox("Number Keys");
    auto* bankLayout = new QHBoxLayout(bankBox);
    bankLayout->setSpacing(4);
    const char* bankLabels[10] = {"1","2","3","4","5","6","7","8","9","0"};
    for (int i = 0; i < 10; ++i) {
        m_bankBtns[i] = new QPushButton(bankLabels[i]);
        m_bankBtns[i]->setFixedSize(40, 40);
        m_bankBtns[i]->setToolTip(QString("Send KEY,%1,P to scanner").arg(bankLabels[i]));
        connect(m_bankBtns[i], &QPushButton::clicked, this, [this, i](){ onBankToggled(i + 1); });
        bankLayout->addWidget(m_bankBtns[i]);
    }
    bankLayout->addStretch();
    parent->addWidget(bankBox);

    // Main control buttons
    auto* ctrlLayout = new QHBoxLayout;
    ctrlLayout->setSpacing(8);

    m_scanBtn = makeButton("SCAN", "#1a5c1a",
        "Resume scanning (KEY,S,P)");
    m_scanBtn->setShortcut(QKeySequence("Ctrl+S"));

    m_holdBtn = makeButton("HOLD", "#5c5c1a",
        "Hold on current channel (EPG)");
    m_holdBtn->setShortcut(QKeySequence("Ctrl+H"));

    m_searchBtn = makeButton("SEARCH", "#5c3a1a",
        "Service search — select services and search pre-programmed frequencies");
    m_searchBtn->setShortcut(QKeySequence("Ctrl+F"));

    m_exploreBtn = makeButton("EXPLORE", "#3a1a5c",
        "Custom frequency range search (KEY,R,P)");
    m_exploreBtn->setShortcut(QKeySequence("Ctrl+E"));

    m_programBtn = makeButton("PROGRAM", "#1a2d5c",
        "Open channel programming window");
    m_programBtn->setShortcut(QKeySequence("Ctrl+P"));

    m_settingsBtn = makeButton("SETTINGS", "#3d1a5c",
        "Open scanner settings");

    m_backlightBtn = makeButton("BACKLIGHT", "#1a4a4a",
        "Toggle scanner backlight");

    ctrlLayout->addWidget(m_scanBtn);
    ctrlLayout->addWidget(m_holdBtn);
    ctrlLayout->addWidget(m_searchBtn);
    ctrlLayout->addWidget(m_exploreBtn);
    ctrlLayout->addWidget(m_programBtn);
    ctrlLayout->addWidget(m_settingsBtn);
    ctrlLayout->addWidget(m_backlightBtn);
    parent->addLayout(ctrlLayout);

    connect(m_scanBtn,      &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(m_holdBtn,      &QPushButton::clicked, this, &MainWindow::onHoldClicked);
    connect(m_searchBtn,    &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(m_exploreBtn,   &QPushButton::clicked, this, &MainWindow::onExploreClicked);
    connect(m_programBtn,   &QPushButton::clicked, this, &MainWindow::onProgramClicked);
    connect(m_settingsBtn,  &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(m_backlightBtn, &QPushButton::clicked, this, &MainWindow::onBacklightClicked);
}

QPushButton* MainWindow::makeButton(const QString& label, const QString& color,
                                     const QString& tooltip) {
    auto* btn = new QPushButton(label);
    btn->setFixedHeight(50);
    btn->setMinimumWidth(90);
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background: qlineargradient(y1:0,y2:1, stop:0 %1, stop:1 #111);"
        "  color: #e0e0e0;"
        "  border: 1px solid #444;"
        "  border-radius: 6px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  letter-spacing: 1px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(y1:0,y2:1, stop:0 %2, stop:1 #222);"
        "  border: 1px solid #666;"
        "}"
        "QPushButton:pressed { background: #111; }"
        "QPushButton:disabled { background: #222; color: #444; }"
    ).arg(color).arg(color));
    if (!tooltip.isEmpty()) btn->setToolTip(tooltip);
    return btn;
}

void MainWindow::applyStyleSheet() {
    setStyleSheet(R"(
        QMainWindow, QWidget {
            background-color: #1e1e1e;
            color: #d0d0d0;
        }
        QMenuBar {
            background-color: #252525;
            color: #d0d0d0;
            border-bottom: 1px solid #333;
        }
        QMenuBar::item:selected { background-color: #3a3a3a; }
        QMenu {
            background-color: #252525;
            color: #d0d0d0;
            border: 1px solid #444;
        }
        QMenu::item:selected { background-color: #3a5a8a; }
        QToolBar {
            background-color: #252525;
            border-bottom: 1px solid #333;
            spacing: 4px;
        }
        QGroupBox {
            color: #888;
            border: 1px solid #333;
            border-radius: 4px;
            margin-top: 8px;
            font-size: 11px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            padding: 0 4px;
        }
        QPushButton#bankBtn {
            background: #2a3a2a;
            color: #00cc00;
            border: 1px solid #335533;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton#bankBtn:pressed {
            background: qlineargradient(y1:0,y2:1, stop:0 #1a5c1a, stop:1 #0a2a0a);
            border: 1px solid #00aa00;
        }
        QPushButton#bankBtn[bankActive="false"] {
            background: #1a1a1a;
            color: #333;
            border: 1px solid #1e1e1e;
        }
        QComboBox {
            background-color: #2a2a2a;
            color: #d0d0d0;
            border: 1px solid #444;
            border-radius: 3px;
            padding: 3px 6px;
        }
        QComboBox QAbstractItemView {
            background-color: #2a2a2a;
            color: #d0d0d0;
            selection-background-color: #3a5a8a;
        }
        QLabel { color: #d0d0d0; }
        QSlider::groove:horizontal {
            height: 6px;
            background: #333;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #4a8a4a;
            width: 14px;
            height: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }
        QSlider::sub-page:horizontal { background: #2a5a2a; border-radius: 3px; }
        QPlainTextEdit {
            background-color: #111;
            color: #00cc00;
            border: 1px solid #333;
            font-family: "Courier New", monospace;
        }
        QStatusBar {
            background-color: #252525;
            color: #888;
            border-top: 1px solid #333;
        }
        QScrollBar:vertical {
            background: #1e1e1e;
            width: 10px;
        }
        QScrollBar::handle:vertical {
            background: #444;
            border-radius: 5px;
            min-height: 20px;
        }
        QTableWidget {
            background-color: #1a1a1a;
            alternate-background-color: #222;
            gridline-color: #333;
            color: #d0d0d0;
        }
        QHeaderView::section {
            background-color: #2a2a2a;
            color: #aaa;
            border: none;
            border-bottom: 1px solid #444;
            padding: 4px;
        }
    )");

    for (int i = 0; i < 10; ++i) {
        m_bankBtns[i]->setObjectName("bankBtn");
        m_bankBtns[i]->setProperty("bankActive", false);
    }
}

// ---- Actions ----

void MainWindow::onConnectClicked() {
    QString port = m_portCombo->currentData().toString();
    if (port.isEmpty()) port = m_portCombo->currentText().split(' ')[0];
    if (port.isEmpty()) { statusBar()->showMessage("Select a serial port first."); return; }

    AppSettings::instance().setSerialPort(port);
    statusBar()->showMessage(QString("Connecting to %1...").arg(port));
    m_scanner->connectPort(port);
}

void MainWindow::onDisconnectClicked() {
    if (m_autoTransmission) {
        m_autoTransmission = false;
        m_holdPollTimer->stop();
        m_logger->cancelTransmission();
    }
    m_scanner->disconnectPort();
}

void MainWindow::onScanClicked() {
    if (!m_scanner->isConnected()) return;
    m_holdLocked = false;
    // Cancel any in-progress auto-transmission before resuming
    if (m_autoTransmission) {
        m_autoTransmission = false;
        m_holdPollTimer->stop();
        m_autoSkipTimer->stop();
        m_holdSeconds = 0;
        m_lcd->setHoldTimer(0);
        m_logger->cancelTransmission();
    }
    m_scanner->sendCommand("KEY,S,P");
    m_scanIdleTimer->start();
    updateLog("SCAN: resuming scan (KEY,S,P sent).");
}

void MainWindow::onHoldClicked() {
    if (!m_scanner->isConnected()) return;
    m_holdLocked = true;
    // Cancel any running auto-skip countdown
    if (m_autoTransmission) {
        m_autoSkipTimer->stop();
        m_holdPollTimer->stop();
        m_holdSeconds = 0;
        m_lcd->setHoldTimer(0);
    }
    m_scanner->sendCommand("EPG");
    updateLog("HOLD: scanner held (EPG sent).");
}

void MainWindow::onSearchClicked() {
    if (!m_scanner->isConnected()) return;
    m_holdLocked = false;
    m_scanIdleTimer->stop();

    static const char* serviceNames[10] = {
        "Police", "Fire / Emergency", "HAM Radio", "Marine",
        "Railroad", "Civil Air", "Military Air", "CB Radio",
        "FRS / GMRS / MURS", "Racing"
    };

    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Service Search");
    dlg->setModal(true);
    auto* layout = new QVBoxLayout(dlg);

    // Header label
    auto* hdr = new QLabel("Select services to include in search:");
    hdr->setStyleSheet("font-weight: bold; margin-bottom: 4px;");
    layout->addWidget(hdr);

    // Build checkboxes
    QString saved = AppSettings::instance().serviceSearchBits();
    if (saved.length() != 10) saved = "0000000000";

    QCheckBox* boxes[10];
    for (int i = 0; i < 10; ++i) {
        boxes[i] = new QCheckBox(serviceNames[i]);
        boxes[i]->setChecked(saved[i] == '0'); // '0'=enabled, '1'=disabled
        layout->addWidget(boxes[i]);
    }

    // Select All / Clear All row
    auto* selLayout = new QHBoxLayout;
    auto* selAll   = new QPushButton("Select All");
    auto* clearAll = new QPushButton("Clear All");
    selLayout->addWidget(selAll);
    selLayout->addWidget(clearAll);
    selLayout->addStretch();
    layout->addLayout(selLayout);

    connect(selAll,   &QPushButton::clicked, dlg, [boxes](){
        for (auto* b : boxes) b->setChecked(true);
    });
    connect(clearAll, &QPushButton::clicked, dlg, [boxes](){
        for (auto* b : boxes) b->setChecked(false);
    });

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

    if (dlg->exec() != QDialog::Accepted) { delete dlg; return; }

    // Build bit string: '0'=enabled, '1'=disabled
    QString bits(10, '1');
    for (int i = 0; i < 10; ++i)
        bits[i] = boxes[i]->isChecked() ? '0' : '1';
    delete dlg;

    AppSettings::instance().setServiceSearchBits(bits);

    // Enter PRG → SSG → EPG → KEY,F,P → KEY,R,P
    m_scanner->enterProgramMode([this, bits](bool ok, const QString&){
        if (!ok) { updateLog("SEARCH: failed to enter program mode."); return; }
        m_scanner->setServiceSearchGroup(bits, [this](bool, const QString&){
            m_scanner->exitProgramMode([this](bool, const QString&){
                m_scanner->sendCommand("KEY,F,P", [this](bool, const QString&){
                    m_scanner->sendCommand("KEY,R,P");
                });
            });
        });
    });
    updateLog("SEARCH: service search started.");
}

void MainWindow::onExploreClicked() {
    if (!m_scanner->isConnected()) return;
    m_holdLocked = false;
    m_scanIdleTimer->stop();
    m_scanner->sendCommand("KEY,R,P");
    updateLog("EXPLORE: custom frequency range search started (KEY,R,P sent).");
}

void MainWindow::onProgramClicked() {
    if (!m_scanner->isConnected()) {
        QMessageBox::warning(this, "Not Connected",
            "Please connect to the scanner before programming.");
        return;
    }
    if (m_stsPoller) m_stsPoller->stop();
    if (m_autoTransmission) {
        m_autoTransmission = false;
        m_holdPollTimer->stop();
        m_logger->cancelTransmission();
    }
    auto* dlg = new ProgramWindow(m_scanner, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
    // After programming, exit program mode and go to scan hold
    m_scanner->exitProgramMode([this](bool, const QString&){ setScanState(); });
}

void MainWindow::onSettingsClicked() {
    auto* dlg = new SettingsWindow(m_scanner, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

void MainWindow::onDebugClicked() {
    if (!m_debugWindow) {
        m_debugWindow = new DebugWindow(m_scanner, m_stsPoller, this);
        m_debugWindow->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_debugWindow, &QObject::destroyed, this, [this](){
            m_debugWindow = nullptr;
        });
    }
    m_debugWindow->show();
    m_debugWindow->raise();
    m_debugWindow->activateWindow();
}

void MainWindow::onBankToggled(int bank) {
    if (!m_scanner->isConnected()) return;
    // bank is 1-based (1-9) or 10 for the "0" key
    const char key = (bank == 10) ? '0' : char('0' + bank);
    m_scanner->sendCommand(QString("KEY,%1,P").arg(key));
}

void MainWindow::onBacklightClicked() {
    if (!m_scanner->isConnected()) return;
    m_backlightOn = !m_backlightOn;

    m_scanner->enterProgramMode([this](bool ok, const QString&){
        if (!ok) return;
        m_scanner->setBacklight(m_backlightOn ? "AO" : "AF", [this](bool, const QString&){
            m_scanner->exitProgramMode([this](bool, const QString&){
                updateLog(QString("Backlight %1").arg(m_backlightOn ? "on" : "off"));
            });
        });
    });
}

void MainWindow::onAutoSkipTimer() {
    if (!m_autoTransmission) return;
    const int maxSecs = AppSettings::instance().autoSkipSeconds();

    const QString modeStr = (m_resumeMode == ResumeMode::ServiceSearch) ? "service search"
                          : (m_resumeMode == ResumeMode::Explore)        ? "explore"
                          :                                                 "scan";
    updateLog(QString("Auto-skip: transmission >%1s (likely digital) — not logged, resuming %2.")
              .arg(maxSecs).arg(modeStr));

    m_autoTransmission = false;
    m_holdPollTimer->stop();
    m_holdSeconds = 0;
    m_lcd->setHoldTimer(0);
    m_logger->cancelTransmission();

    switch (m_resumeMode) {
    case ResumeMode::ServiceSearch:
        m_scanner->sendCommand("KEY,F,P", [this](bool, const QString&){
            m_scanner->sendCommand("KEY,R,P");
        });
        break;
    case ResumeMode::Explore:
        m_scanner->sendCommand("KEY,R,P");
        break;
    default:
        m_scanner->sendCommand("KEY,S,P");
        break;
    }
}

void MainWindow::onHoldPollTimer() {
    if (!m_autoTransmission) return;
    m_holdSeconds++;
    const int remaining = AppSettings::instance().autoSkipSeconds() - m_holdSeconds;
    m_lcd->setHoldTimer(remaining > 0 ? remaining : 0);
}

// ---- Scanner signal handlers ----

void MainWindow::onScannerConnected() {
    setConnectedState(true);
    m_lcd->setState(LcdWidget::ScanState::Scanning);
    statusBar()->showMessage("Connected. Fetching scanner info...");

    // Get model and version (no program mode needed)
    m_scanner->getModelInfo([this](bool ok, const QString& resp){
        if (ok) {
            QStringList p = resp.split(',');
            QString model = (p.size() >= 2) ? p[1] : resp;
            m_modelLabel->setText("  " + model + "  ");
            m_lcd->setModel(model);
        }
    });
    m_scanner->getFirmwareVersion([this](bool ok, const QString& resp){
        if (ok) {
            QStringList p = resp.split(',');
            statusBar()->showMessage(p.size() >= 2 ?
                "Connected - " + p[1] : "Connected");
        }
    });

    // Read volume and squelch — these work without entering program mode
    m_scanner->getVolume([this](bool ok, const QString& resp){
        if (ok) {
            QStringList p = resp.split(',');
            if (p.size() >= 2) {
                int v = p[1].toInt();
                m_volSlider->blockSignals(true);
                m_volSlider->setValue(v);
                m_volLabel->setText(QString::number(v));
                m_volSlider->blockSignals(false);
            }
        }
    });
    m_scanner->getSquelch([this](bool ok, const QString& resp){
        if (ok) {
            QStringList p = resp.split(',');
            if (p.size() >= 2) {
                int v = p[1].toInt();
                m_sqlSlider->blockSignals(true);
                m_sqlSlider->setValue(v);
                m_sqlLabel->setText(QString::number(v));
                m_sqlSlider->blockSignals(false);
            }
        }
    });

    // Don't enter program mode on connect — that pauses the scanner.
    // Bank status defaults to all-enabled; user can sync via Settings → Read All.
    setScanState();
    updateLog("Connected. Scanner is scanning freely. Use HOLD to pause and read channel info.");
}

void MainWindow::onScannerDisconnected() {
    if (m_stsPoller) m_stsPoller->stop();
    if (m_autoTransmission) {
        m_autoTransmission = false;
        m_holdPollTimer->stop();
        m_logger->cancelTransmission();
    }
    m_stsState = StsStatus::State::Unknown;
    m_scanIdleTimer->stop();
    setConnectedState(false);
    m_lcd->setState(LcdWidget::ScanState::Disconnected);
    m_lcd->setFrequency("----.----");
    m_lcd->setChannelLabel("DISCONNECTED");
    m_lcd->setModel("");
    m_modelLabel->clear();
    m_autoSkipTimer->stop();
    m_holdPollTimer->stop();
    m_holdSeconds = 0;
    statusBar()->showMessage("Disconnected.");
    updateLog("Disconnected from scanner.");
}

void MainWindow::onScannerError(const QString& msg) {
    statusBar()->showMessage("Error: " + msg);
    updateLog("ERROR: " + msg);
    m_lcd->setState(LcdWidget::ScanState::Error);
}

void MainWindow::onCommandSent(const QString& cmd) {
    if (cmd.startsWith("CIN") || cmd.startsWith("PRG") || cmd.startsWith("EPG"))
        updateLog(">> " + cmd);
}

void MainWindow::onResponseReceived(const QString& resp) {
    if (resp.startsWith("CIN") || resp.startsWith("ERR") || resp.startsWith("NG"))
        updateLog("<< " + resp);
}

// ---- State management ----

void MainWindow::setConnectedState(bool connected) {
    m_connectBtn->setEnabled(!connected);
    m_disconnectBtn->setEnabled(connected);
    m_portCombo->setEnabled(!connected);
    // SCAN / HOLD / SEARCH / EXPLORE are enabled/disabled by STS state — start disabled
    m_scanBtn->setEnabled(false);
    m_holdBtn->setEnabled(false);
    m_searchBtn->setEnabled(false);
    m_exploreBtn->setEnabled(false);
    m_programBtn->setEnabled(connected);
    m_backlightBtn->setEnabled(connected);
    for (auto* btn : m_bankBtns) btn->setEnabled(connected);

    m_connStatusLabel->setText(connected ? "  CONNECTED  " : "  DISCONNECTED  ");
    m_connStatusLabel->setStyleSheet(connected ?
        "background: #0a3a0a; color: #00cc00; border: 1px solid #00aa00;" :
        "background: #3a0a0a; color: #cc0000; border: 1px solid #aa0000;");
}

void MainWindow::setScanState() {
    m_holdPollTimer->stop();
    m_autoSkipTimer->stop();
    m_holdSeconds = 0;
    m_lcd->setHoldTimer(0);
    statusBar()->showMessage("Scanning...");
    if (m_stsPoller && m_scanner->isConnected()) m_stsPoller->start();
}

void MainWindow::setProgramState() {
    m_lcd->setState(LcdWidget::ScanState::Programming);
    statusBar()->showMessage("Remote programming mode…");
}




void MainWindow::updateLog(const QString& msg) {
    m_activityLog->appendPlainText(
        QDateTime::currentDateTime().toString("hh:mm:ss") + "  " + msg);
}

void MainWindow::updateTxLog(const TransmissionRecord& r) {
    QString line = QString("%1  CH%2  %3  %4 MHz  %5  %6s")
        .arg(r.startTime.toString("yyyy-MM-dd hh:mm:ss"))
        .arg(r.channelIndex, 3, 10, QChar('0'))
        .arg(r.channelLabel.leftJustified(16))
        .arg(r.frequency.rightJustified(10))
        .arg(r.ctcssDcsLabel.leftJustified(20))
        .arg(r.durationSeconds);
    m_txLogView->appendPlainText(line);

    // Update tab label with count
    int count = m_txLogView->document()->blockCount();
    m_logTabs->setTabText(1, QString("Transmissions (%1)").arg(count));
    m_logTabs->setCurrentIndex(1);  // switch to tx log on new entry
}

void MainWindow::onTransmissionLogged(const TransmissionRecord& r) {
    updateLog(QString("[TX] CH%1 \"%2\" %3 MHz %4 — %5s")
        .arg(r.channelIndex).arg(r.channelLabel)
        .arg(r.frequency).arg(r.ctcssDcsLabel).arg(r.durationSeconds));
    updateTxLog(r);
}

void MainWindow::onStsStatusUpdated(const StsStatus& status)
{
    // Partial mid-update reads parse as Unknown — ignore them completely.
    // They don't count for or against the debounce.
    if (status.state == StsStatus::State::Unknown) return;

    // Latch CTCSS/DCS code whenever the scanner shows it mid-transmission
    if (m_logger->isActive() && !status.ctcssDcs.isEmpty())
        m_logger->updateCtcssDcs(status.ctcssDcs);

    // Update number-key button appearance from the bank-activity mask every poll
    for (int i = 0; i < 10; ++i) {
        const bool active = (status.bankMask >> i) & 1u;
        if (m_bankBtns[i]->property("bankActive").toBool() != active) {
            m_bankBtns[i]->setProperty("bankActive", active);
            m_bankBtns[i]->style()->unpolish(m_bankBtns[i]);
            m_bankBtns[i]->style()->polish(m_bankBtns[i]);
        }
    }

    // --- Debounce: require 2 consecutive same-state readings before
    //     changing SCAN/HOLD button enable state.
    //     LCD content updates immediately on every valid read.
    const bool stateChanged = (status.state != m_stsState);
    if (stateChanged) {
        ++m_stsStateConsecutive;
        if (m_stsStateConsecutive < 2) {
            // Not yet confirmed — update LCD display but leave buttons alone
            updateLcdFromSts(status);
            return;
        }
        // Confirmed: accept the new state
        m_stsStateConsecutive = 0;
        m_stsState = status.state;
    } else {
        m_stsStateConsecutive = 0;
    }

    // --- Buttons (only updated once state is confirmed) ---
    const bool connected   = m_scanner->isConnected();
    const bool isScanning  = (m_stsState == StsStatus::State::Scanning);
    const bool isSearching = (m_stsState == StsStatus::State::Search);
    m_scanBtn->setEnabled(connected && !isScanning && !isSearching);
    m_holdBtn->setEnabled(connected && (isScanning || isSearching));
    m_searchBtn->setEnabled(connected && !isSearching);
    m_exploreBtn->setEnabled(connected && !isSearching);

    updateLcdFromSts(status);
}

void MainWindow::updateLcdFromSts(const StsStatus& status)
{
    switch (status.state) {

    case StsStatus::State::Scanning:
        m_lcd->setState(LcdWidget::ScanState::Scanning);
        m_lcd->setStatusMessage("SCANNING");
        m_lcd->setSignalStrength(0);
        if (!status.channelLabel.isEmpty())
            m_lcd->setChannelLabel(status.channelLabel);
        statusBar()->showMessage("Scanning…");
        break;

    case StsStatus::State::Active:
        m_lcd->setState(LcdWidget::ScanState::Hold);
        m_lcd->setFrequency(status.frequency);
        m_lcd->setChannelLabel(status.channelLabel.isEmpty() ? "ACTIVE" : status.channelLabel);
        if (status.channelNumber > 0) {
            m_lcd->setChannelNumber(bankFromIndex(status.channelNumber),
                                    chInBankFromIndex(status.channelNumber));
            if (hasCachedChannel(status.channelNumber)) {
                const ChannelInfo& c = m_channelCache[status.channelNumber - 1];
                m_lcd->setModulation(c.modulation.isEmpty() ? "NFM" : c.modulation);
                m_lcd->setCtcssDcs(ctcssDcsLabel(c.ctcssDcs));
            } else {
                m_lcd->setModulation("NFM");
                m_lcd->setCtcssDcs("");
            }
        }
        m_lcd->setSignalStrength(status.signalStrength);
        statusBar()->showMessage(
            QString("Active: %1  %2 MHz").arg(status.channelLabel, status.frequency));
        break;

    case StsStatus::State::Search: {
        const bool isSvc = !status.serviceLabel.isEmpty();
        m_lcd->setState(LcdWidget::ScanState::Scanning);
        m_lcd->setFrequency(status.frequency.isEmpty() ? "----.----" : status.frequency);
        m_lcd->setChannelLabel(isSvc ? status.serviceLabel
                               : (status.channelLabel.isEmpty() ? "SEARCH" : status.channelLabel));
        m_lcd->setStatusMessage(isSvc ? "SVC SEARCH" : "SEARCH");
        m_lcd->setSignalStrength(status.signalStrength);
        statusBar()->showMessage(isSvc
            ? QString("Service search: %1  %2 MHz").arg(status.serviceLabel, status.frequency)
            : QString("Searching: %1 MHz").arg(status.frequency));
        break;
    }

    case StsStatus::State::Menu:
        m_lcd->setState(LcdWidget::ScanState::Programming);
        m_lcd->setStatusMessage("REMOTE");
        statusBar()->showMessage("Scanner in menu / remote programming mode.");
        break;

    default:
        break;
    }
}

void MainWindow::onSquelchOpened(const StsStatus& status)
{
    // Ignore if the user manually held the scanner, or if we haven't yet confirmed
    // a scanning/searching state (e.g. scanner was already in hold at app startup).
    if (m_holdLocked) return;
    if (m_stsState != StsStatus::State::Scanning &&
        m_stsState != StsStatus::State::Search) return;

    m_scanIdleTimer->stop();

    if (m_logger->isActive()) return;

    // Record mode so auto-skip sends the right resume command
    if (m_stsState == StsStatus::State::Search)
        m_resumeMode = status.serviceLabel.isEmpty() ? ResumeMode::Explore : ResumeMode::ServiceSearch;
    else
        m_resumeMode = ResumeMode::Scan;

    // Look up channel in cache by number (STS gives us the channel number directly)
    int     chIdx = status.channelNumber;
    QString label = status.channelLabel;
    QString freq  = status.frequency;
    QString ctcssLabel;

    if (chIdx > 0 && hasCachedChannel(chIdx)) {
        const ChannelInfo& c = cachedChannel(chIdx);
        label = c.name.isEmpty() ? label : c.name;
        freq  = c.freqMhz().isEmpty() ? freq : c.freqMhz();
        ctcssLabel = !status.ctcssDcs.isEmpty() ? status.ctcssDcs : ctcssDcsLabel(c.ctcssDcs);
        setCurrentChannelIndex(chIdx);
    } else {
        ctcssLabel = status.ctcssDcs;
    }

    m_logger->beginTransmission(chIdx, label, freq, ctcssLabel);
    m_autoTransmission = true;
    m_holdSeconds = 0;
    m_holdPollTimer->start();

    const int maxSecs = AppSettings::instance().autoSkipSeconds();
    m_autoSkipTimer->start(maxSecs * 1000);

    updateLog(QString("[HIT] CH%1 \"%2\" %3 MHz%4")
        .arg(chIdx > 0 ? QString::number(chIdx) : "?")
        .arg(label).arg(freq)
        .arg(m_resumeMode == ResumeMode::ServiceSearch ? QString(" (%1)").arg(status.serviceLabel)
           : m_resumeMode == ResumeMode::Explore       ? " (explore)"
           :                                             ""));
}

void MainWindow::onSquelchClosed()
{
    if (!m_autoTransmission) return;
    m_autoTransmission = false;
    m_holdPollTimer->stop();
    m_autoSkipTimer->stop();

    const int elapsed = m_logger->elapsedSeconds();
    const int minSecs = AppSettings::instance().minTransmissionSeconds();

    if (elapsed <= minSecs) {
        m_logger->cancelTransmission();
        updateLog(QString("[HIT] Signal too short (%1s, min %2s) — not logged.")
                  .arg(elapsed).arg(minSecs));
    } else {
        m_logger->endTransmission();
        updateLog(QString("[HIT] Signal ended (%1s) — logged.").arg(elapsed));
    }

    m_holdSeconds = 0;
    m_lcd->setHoldTimer(0);

    // Restart idle timer if we're back in regular scan mode
    if (m_resumeMode == ResumeMode::Scan)
        m_scanIdleTimer->start();
}

// ---- Channel cache ----

ChannelInfo MainWindow::cachedChannel(int index) const {
    if (index >= 1 && index <= 500 && m_channelCacheValid[index - 1])
        return m_channelCache[index - 1];
    ChannelInfo empty;
    empty.index = index;
    return empty;
}

bool MainWindow::hasCachedChannel(int index) const {
    return index >= 1 && index <= 500 && m_channelCacheValid[index - 1];
}

void MainWindow::setCurrentChannelIndex(int index) {
    if (index < 1)   index = 500;
    if (index > 500) index = 1;
    m_currentChannelIndex = index;

    int bank = bankFromIndex(index);
    int ch   = chInBankFromIndex(index);
    int dispBank = (bank == 10) ? 0 : bank;
    m_navBankLabel->setText(QString("BNK %1").arg(dispBank));
    m_navChLabel->setText(QString("CH %1").arg(ch, 2, 10, QChar('0')));

    if (hasCachedChannel(index)) {
        const ChannelInfo& ci = m_channelCache[index - 1];
        m_navFreqLabel->setText(ci.freqMhz() + " MHz");
        m_navNameLabel->setText(ci.name.isEmpty() ? "---" : ci.name);
    } else {
        m_navFreqLabel->setText("--- . ---- MHz");
        m_navNameLabel->setText("---");
    }

    // Update LCD if scanning (not in hold/program mode)
    if (m_stsState != StsStatus::State::Active) updateLcdFromCache();
}

void MainWindow::updateLcdFromCache() {
    if (hasCachedChannel(m_currentChannelIndex)) {
        const ChannelInfo& ch = m_channelCache[m_currentChannelIndex - 1];
        m_lcd->setFrequency(ch.freqMhz());
        m_lcd->setChannelLabel(ch.name.isEmpty() ? "--------" : ch.name);
        m_lcd->setChannelNumber(bankFromIndex(ch.index), chInBankFromIndex(ch.index));
        m_lcd->setModulation(ch.modulation.isEmpty() ? "NFM" : ch.modulation);
        m_lcd->setCtcssDcs(ctcssDcsLabel(ch.ctcssDcs));
    } else {
        m_lcd->setFrequency("--- . ----");
        m_lcd->setChannelLabel("NO DATA");
        m_lcd->setChannelNumber(bankFromIndex(m_currentChannelIndex),
                                chInBankFromIndex(m_currentChannelIndex));
        m_lcd->setCtcssDcs("");
    }
}

void MainWindow::updateChannelCache(const QVector<ChannelInfo>& channels) {
    for (const ChannelInfo& ch : channels) {
        if (ch.index >= 1 && ch.index <= 500) {
            m_channelCache[ch.index - 1]      = ch;
            m_channelCacheValid[ch.index - 1] = true;
        }
    }
    setCurrentChannelIndex(m_currentChannelIndex); // refresh nav bar display
    updateLcdFromCache();
    updateLog(QString("Channel cache updated: %1 channels loaded.").arg(channels.size()));
}

// ---- Channel navigation slots ----

void MainWindow::onChannelPrev() { setCurrentChannelIndex(m_currentChannelIndex - 1); }
void MainWindow::onChannelNext() { setCurrentChannelIndex(m_currentChannelIndex + 1); }

void MainWindow::onBankPrev() {
    int bank = bankFromIndex(m_currentChannelIndex);
    int newBank = (bank <= 1) ? 10 : bank - 1;
    setCurrentChannelIndex((newBank - 1) * 50 + 1);
}

void MainWindow::onBankNext() {
    int bank = bankFromIndex(m_currentChannelIndex);
    int newBank = (bank >= 10) ? 1 : bank + 1;
    setCurrentChannelIndex((newBank - 1) * 50 + 1);
}

void MainWindow::onLoadChannelCache() {
    if (!m_scanner->isConnected()) {
        statusBar()->showMessage("Not connected — cannot load channels.");
        return;
    }

    m_loadCacheBtn->setEnabled(false);
    m_loadCacheBtn->setText("Loading…");
    updateLog("Loading all 500 channels from scanner (this pauses scanning briefly)…");

    if (m_stsPoller) m_stsPoller->stop();

    m_scanner->enterProgramMode([this](bool ok, const QString&){
        if (!ok) {
            m_loadCacheBtn->setEnabled(true);
            m_loadCacheBtn->setText("Load Channels from Scanner");
            updateLog("Failed to enter program mode for channel load.");
            return;
        }

        auto loadNext = std::make_shared<std::function<void(int)>>();
        auto loaded   = std::make_shared<int>(0);

        *loadNext = [this, loadNext, loaded](int idx) {
            if (idx > 500) {
                m_scanner->exitProgramMode([this, loaded](bool, const QString&){
                    m_loadCacheBtn->setEnabled(true);
                    m_loadCacheBtn->setText("Load Channels from Scanner");
                    setCurrentChannelIndex(m_currentChannelIndex);
                    updateLcdFromCache();
                    updateLog(QString("Channel cache loaded: %1 programmed channels found.").arg(*loaded));
                    statusBar()->showMessage("Channel cache loaded. Scanner resuming (press physical SCAN if needed).");
                    if (m_stsPoller) m_stsPoller->start();
                });
                return;
            }
            m_scanner->getChannelInfo(idx, [this, idx, loadNext, loaded](bool ok, const QString& resp){
                if (ok) {
                    ChannelInfo ch = ScannerSerial::parseChannelInfo(resp);
                    m_channelCache[idx - 1]      = ch;
                    m_channelCacheValid[idx - 1] = true;
                    if (!ch.isEmpty()) (*loaded)++;
                }
                (*loadNext)(idx + 1);
            });
        };
        (*loadNext)(1);
    });
}

void MainWindow::closeEvent(QCloseEvent* event) {
    AppSettings::instance().setWindowSize(width(), height());
    if (m_stsPoller) m_stsPoller->stop();
    if (m_autoTransmission) {
        m_autoTransmission = false;
        m_logger->cancelTransmission();
    }
    if (m_scanner->isConnected()) m_scanner->disconnectPort();
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
}
