#pragma once
#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include "scannerserial.h"
#include "ctcssdcsdata.h"

class ProgramWindow : public QDialog {
    Q_OBJECT
public:
    explicit ProgramWindow(ScannerSerial* scanner, QWidget* parent = nullptr);

private slots:
    void onBankChanged(int bank);
    void onChannelSelected(int row, int column);
    void onReadChannel();
    void onWriteChannel();
    void onDeleteChannel();
    void onReadAllChannels();
    void onWriteAllChannels();
    void onChannelTableChanged();

private:
    void buildUi();
    void setupChannelTable();
    void populateCtcssDcsCombo();
    void loadChannelToForm(const ChannelInfo& ch);
    ChannelInfo channelFromForm() const;
    void updateChannelTableRow(int row, const ChannelInfo& ch);
    void setFormEnabled(bool enabled);
    void setStatus(const QString& msg, bool ok = true);
    int  currentChannelIndex() const;
    void readChannelByIndex(int index, std::function<void(const ChannelInfo&)> cb);

    ScannerSerial* m_scanner;

    QComboBox*    m_bankCombo;
    QTableWidget* m_channelTable;

    QLineEdit*    m_freqEdit;
    QLineEdit*    m_nameEdit;
    QComboBox*    m_modCombo;
    QComboBox*    m_ctcssDcsCombo;
    QComboBox*    m_delayCombo;
    QCheckBox*    m_lockoutCheck;
    QCheckBox*    m_priorityCheck;

    QPushButton*  m_readBtn;
    QPushButton*  m_writeBtn;
    QPushButton*  m_deleteBtn;
    QPushButton*  m_readAllBtn;
    QPushButton*  m_writeAllBtn;

    QLabel*       m_statusLabel;
    QProgressBar* m_progress;

    int m_currentBank    = 1;
    int m_currentChannel = 1;

    // Cached channel data for current bank
    ChannelInfo m_bankChannels[50];
    bool m_channelLoaded[50] = {};
};
