#pragma once
#include <QDialog>
#include <QTabWidget>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include "scannerserial.h"

class SettingsWindow : public QDialog {
    Q_OBJECT
public:
    explicit SettingsWindow(ScannerSerial* scanner, QWidget* parent = nullptr);

private slots:
    void onReadAll();
    void onApply();
    void onClearAllMemory();
    void onAddLockoutFreq();
    void onRemoveLockoutFreq();
    void onLoadLockoutList();
    void onBrowseLogDir();

private:
    void buildUi();
    QWidget* buildScannerTab();
    QWidget* buildSearchTab();
    QWidget* buildCloseCallTab();
    QWidget* buildLockoutTab();
    QWidget* buildAppTab();
    void setStatus(const QString& msg, bool ok = true);
    void setAllEnabled(bool enabled);

    ScannerSerial* m_scanner;
    QTabWidget*    m_tabs;

    // Scanner tab
    QSlider*   m_volSlider;
    QLabel*    m_volLabel;
    QSlider*   m_sqlSlider;
    QLabel*    m_sqlLabel;
    QComboBox* m_backlightCombo;
    QComboBox* m_keyBeepCombo;
    QCheckBox* m_keyLockCheck;
    QComboBox* m_priorityCombo;
    QSpinBox*  m_contrastSpin;
    QSpinBox*  m_chargeTimeSpin;
    QCheckBox* m_wxAlertCheck;

    // Bank scan group
    QCheckBox* m_bankChecks[10];

    // Search tab
    QComboBox* m_searchDelayCombo;
    QCheckBox* m_codeSearchCheck;

    // Service search group
    QCheckBox* m_svcSearchChecks[10];

    // Custom search group
    QCheckBox* m_custSearchChecks[10];
    QTableWidget* m_custSearchTable;

    // Close call tab
    QComboBox* m_ccModeCombo;
    QCheckBox* m_ccAlertBeepCheck;
    QCheckBox* m_ccAlertLightCheck;
    QCheckBox* m_ccBandChecks[5];
    QCheckBox* m_ccLockoutCheck;

    // Lockout tab
    QTableWidget* m_lockoutTable;
    QLineEdit*    m_lockoutFreqEdit;

    // App tab
    QSpinBox*  m_autoSkipSpin;
    QSpinBox*  m_minTransSpin;
    QLineEdit* m_logDirEdit;
    QCheckBox* m_autoConnectCheck;

    QPushButton* m_readAllBtn;
    QPushButton* m_applyBtn;
    QLabel*      m_statusLabel;
};
