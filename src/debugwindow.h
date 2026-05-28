#pragma once
#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include "stspoller.h"

class ScannerSerial;

class DebugWindow : public QDialog {
    Q_OBJECT
public:
    explicit DebugWindow(ScannerSerial* scanner, StsPoller* stsPoller,
                         QWidget* parent = nullptr);

private slots:
    void onAllCommandSent(const QString& cmd);
    void onAllResponseReceived(const QString& resp);
    void onStsStatus(const QString& raw);
    void onStsUpdated(const StsStatus& status);
    void onSendClicked();
    void onQuickKey(const QString& key);

private:
    void buildSerialTab();
    void buildConsoleTab();
    void appendSerial(const QString& line, const QString& color = "#d0d0d0");
    void appendConsole(const QString& line, const QString& color = "#d0d0d0");
    void applyStyle();

    ScannerSerial* m_scanner;
    StsPoller*     m_stsPoller;

    // Serial monitor
    QPlainTextEdit* m_serialLog   = nullptr;
    QCheckBox*      m_hideSts     = nullptr;
    QCheckBox*      m_hideSql     = nullptr;
    QLabel*         m_stsFields   = nullptr;

    // Command console
    QLineEdit*      m_cmdInput    = nullptr;
    QPlainTextEdit* m_consoleLog  = nullptr;
};
