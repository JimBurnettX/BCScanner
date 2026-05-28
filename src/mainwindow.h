#pragma once
#include <QMainWindow>
#include <QBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QTabWidget>
#include <QTimer>
#include <QAction>
#include "scannerserial.h"
#include "lcdwidget.h"
#include "transmissionlogger.h"
#include "stspoller.h"
#include "debugwindow.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Called by ProgramWindow after it loads channels so we cache them here
    void updateChannelCache(const QVector<ChannelInfo>& channels);

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onScanClicked();
    void onHoldClicked();
    void onProgramClicked();
    void onSettingsClicked();
    void onBankToggled(int bank);
    void onBacklightClicked();
    void onLoadChannelCache();
    void onChannelPrev();
    void onChannelNext();
    void onBankPrev();
    void onBankNext();

    void onScannerConnected();
    void onScannerDisconnected();
    void onScannerError(const QString& msg);
    void onCommandSent(const QString& cmd);
    void onResponseReceived(const QString& resp);

    void onAutoSkipTimer();
    void onHoldPollTimer();
    void onTransmissionLogged(const TransmissionRecord& record);

    void onStsStatusUpdated(const StsStatus& status);
    void onSquelchOpened(const StsStatus& status);
    void onSquelchClosed();
    void onSearchClicked();
    void onExploreClicked();
    void onDebugClicked();

private:
    void buildUi();
    void buildToolbar();
    void buildChannelNavBar(QBoxLayout* parent);
    void buildButtonPanel(QBoxLayout* parent);
    QPushButton* makeButton(const QString& label, const QString& color,
                            const QString& tooltip = {});
    void setConnectedState(bool connected);
    void setScanState();
    void setProgramState();
    void updateLcdFromSts(const StsStatus& status);
    void updateLog(const QString& msg);
    void updateTxLog(const TransmissionRecord& r);
    void applyStyleSheet();
    void refreshScannerInfo() {}

    // Channel navigation helpers
    void setCurrentChannelIndex(int index);   // 1-500
    void updateLcdFromCache();
    ChannelInfo cachedChannel(int index) const;
    bool hasCachedChannel(int index) const;

    ScannerSerial*      m_scanner;
    TransmissionLogger* m_logger;
    StsPoller*          m_stsPoller   = nullptr;
    DebugWindow*        m_debugWindow = nullptr;

    // Toolbar
    QComboBox*   m_portCombo;
    QPushButton* m_connectBtn;
    QPushButton* m_disconnectBtn;
    QLabel*      m_connStatusLabel;
    QLabel*      m_modelLabel;

    // LCD
    LcdWidget*   m_lcd;

    // Channel navigation bar
    QLabel*      m_navBankLabel;
    QLabel*      m_navChLabel;
    QLabel*      m_navFreqLabel;
    QLabel*      m_navNameLabel;
    QPushButton* m_loadCacheBtn;

    // Main control buttons
    QPushButton* m_scanBtn;
    QPushButton* m_holdBtn;
    QPushButton* m_searchBtn;   // service search
    QPushButton* m_exploreBtn;  // custom frequency range search
    QPushButton* m_programBtn;
    QPushButton* m_settingsBtn;
    QPushButton* m_backlightBtn;

    // Bank buttons (index 0 = key "1" … index 9 = key "0")
    QPushButton* m_bankBtns[10];

    // Volume / Squelch sliders
    QSlider* m_volSlider;
    QSlider* m_sqlSlider;
    QLabel*  m_volLabel;
    QLabel*  m_sqlLabel;

    // Logs
    QTabWidget*     m_logTabs;
    QPlainTextEdit* m_activityLog;
    QPlainTextEdit* m_txLogView;

    // Timers
    QTimer* m_autoSkipTimer;
    QTimer* m_holdPollTimer;
    QTimer* m_refreshTimer;
    QTimer* m_scanIdleTimer;  // clears MHz display after 30s of scanning with no hit

    // State
    bool             m_backlightOn      = false;
    bool             m_autoTransmission = false;
    bool             m_holdLocked       = false; // true after EPG; suppresses auto-skip
    enum class ResumeMode { Scan, ServiceSearch, Explore };
    ResumeMode       m_resumeMode  = ResumeMode::Scan;
    int              m_holdSeconds = 0;
    StsStatus::State m_stsState           = StsStatus::State::Unknown;
    int              m_stsStateConsecutive = 0; // debounce counter

    // Channel cache — indexed [0..499] for channels 1-500
    ChannelInfo m_channelCache[500];
    bool        m_channelCacheValid[500];
    int         m_currentChannelIndex = 1;   // 1-500
};
