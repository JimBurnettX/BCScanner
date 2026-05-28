#include "programwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QDoubleValidator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTimer>

ProgramWindow::ProgramWindow(ScannerSerial* scanner, QWidget* parent)
    : QDialog(parent), m_scanner(scanner)
{
    setWindowTitle("BC125AT Channel Programming");
    setMinimumSize(750, 560);
    buildUi();
    onBankChanged(1);
}

void ProgramWindow::buildUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // === Top: Bank selector ===
    auto* topLayout = new QHBoxLayout;
    topLayout->addWidget(new QLabel("Bank:"));
    m_bankCombo = new QComboBox;
    for (int i = 1; i <= 10; ++i)
        m_bankCombo->addItem(QString("Bank %1  (CH %2-%3)")
            .arg(i == 10 ? 0 : i)
            .arg((i-1)*50+1)
            .arg(i*50), i);
    m_bankCombo->setFixedWidth(200);
    topLayout->addWidget(m_bankCombo);

    m_readAllBtn = new QPushButton("Read All Channels");
    m_readAllBtn->setToolTip("Read all 50 channels in this bank from scanner");
    topLayout->addWidget(m_readAllBtn);

    m_writeAllBtn = new QPushButton("Write All Channels");
    m_writeAllBtn->setToolTip("Write all changed channels in this bank to scanner");
    topLayout->addWidget(m_writeAllBtn);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

    // === Middle: Channel table + edit form side by side ===
    auto* midLayout = new QHBoxLayout;

    // Channel table
    auto* tableBox = new QGroupBox("Channels");
    auto* tableLayout = new QVBoxLayout(tableBox);
    m_channelTable = new QTableWidget(50, 3);
    m_channelTable->setHorizontalHeaderLabels({"CH", "Label", "Frequency"});
    m_channelTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_channelTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_channelTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_channelTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_channelTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_channelTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_channelTable->setMinimumWidth(220);
    tableLayout->addWidget(m_channelTable);
    midLayout->addWidget(tableBox);

    // Edit form
    auto* formBox = new QGroupBox("Channel Details");
    auto* grid = new QGridLayout(formBox);
    grid->setSpacing(8);

    int row = 0;
    grid->addWidget(new QLabel("Frequency (MHz):"), row, 0);
    m_freqEdit = new QLineEdit;
    m_freqEdit->setPlaceholderText("e.g. 462.5625");
    auto* freqVal = new QRegularExpressionValidator(
        QRegularExpression(R"(\d{1,4}(\.\d{0,4})?)"), m_freqEdit);
    m_freqEdit->setValidator(freqVal);
    grid->addWidget(m_freqEdit, row++, 1);

    grid->addWidget(new QLabel("Channel Label:"), row, 0);
    m_nameEdit = new QLineEdit;
    m_nameEdit->setMaxLength(16);
    m_nameEdit->setPlaceholderText("Max 16 characters");
    grid->addWidget(m_nameEdit, row++, 1);

    grid->addWidget(new QLabel("Modulation:"), row, 0);
    m_modCombo = new QComboBox;
    for (const char* m : {"AUTO","AM","FM","NFM"}) m_modCombo->addItem(m);
    grid->addWidget(m_modCombo, row++, 1);

    grid->addWidget(new QLabel("CTCSS / DCS:"), row, 0);
    m_ctcssDcsCombo = new QComboBox;
    populateCtcssDcsCombo();
    grid->addWidget(m_ctcssDcsCombo, row++, 1);

    grid->addWidget(new QLabel("Scan Delay:"), row, 0);
    m_delayCombo = new QComboBox;
    for (int d : delayValues())
        m_delayCombo->addItem(delayLabel(d), d);
    m_delayCombo->setCurrentIndex(2); // 0s default
    grid->addWidget(m_delayCombo, row++, 1);

    m_lockoutCheck = new QCheckBox("Lockout (skip during scan)");
    grid->addWidget(m_lockoutCheck, row++, 0, 1, 2);

    m_priorityCheck = new QCheckBox("Priority channel");
    grid->addWidget(m_priorityCheck, row++, 0, 1, 2);

    grid->setRowStretch(row, 1);
    ++row;

    // Channel action buttons
    auto* btnLayout = new QHBoxLayout;
    m_readBtn   = new QPushButton("Read from Scanner");
    m_writeBtn  = new QPushButton("Write to Scanner");
    m_deleteBtn = new QPushButton("Delete Channel");
    m_writeBtn->setDefault(true);
    btnLayout->addWidget(m_readBtn);
    btnLayout->addWidget(m_writeBtn);
    btnLayout->addWidget(m_deleteBtn);
    grid->addLayout(btnLayout, row++, 0, 1, 2);

    formBox->setMinimumWidth(300);
    midLayout->addWidget(formBox);
    mainLayout->addLayout(midLayout);

    // === Bottom: status ===
    auto* botLayout = new QHBoxLayout;
    m_statusLabel = new QLabel("Select a channel to edit.");
    m_progress    = new QProgressBar;
    m_progress->setRange(0, 50);
    m_progress->setTextVisible(true);
    m_progress->hide();
    botLayout->addWidget(m_statusLabel, 1);
    botLayout->addWidget(m_progress);
    mainLayout->addLayout(botLayout);

    // Connections
    connect(m_bankCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx){ onBankChanged(m_bankCombo->itemData(idx).toInt()); });
    connect(m_channelTable, &QTableWidget::cellClicked, this, &ProgramWindow::onChannelSelected);
    connect(m_readBtn,     &QPushButton::clicked, this, &ProgramWindow::onReadChannel);
    connect(m_writeBtn,    &QPushButton::clicked, this, &ProgramWindow::onWriteChannel);
    connect(m_deleteBtn,   &QPushButton::clicked, this, &ProgramWindow::onDeleteChannel);
    connect(m_readAllBtn,  &QPushButton::clicked, this, &ProgramWindow::onReadAllChannels);
    connect(m_writeAllBtn, &QPushButton::clicked, this, &ProgramWindow::onWriteAllChannels);

    setupChannelTable();
    setFormEnabled(false);
}

void ProgramWindow::setupChannelTable() {
    m_channelTable->clearContents();
    m_channelTable->setRowCount(50);
    for (int i = 0; i < 50; ++i) {
        auto* numItem   = new QTableWidgetItem(QString::number(i+1));
        auto* labelItem = new QTableWidgetItem("---");
        auto* freqItem  = new QTableWidgetItem("---");
        numItem->setTextAlignment(Qt::AlignCenter);
        freqItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_channelTable->setItem(i, 0, numItem);
        m_channelTable->setItem(i, 1, labelItem);
        m_channelTable->setItem(i, 2, freqItem);
    }
}

void ProgramWindow::populateCtcssDcsCombo() {
    m_ctcssDcsCombo->clear();
    for (const auto& e : ctcssDcsList()) {
        m_ctcssDcsCombo->addItem(e.label, e.code);
    }
}

void ProgramWindow::onBankChanged(int bank) {
    m_currentBank = bank;
    for (int i = 0; i < 50; ++i) m_channelLoaded[i] = false;
    setupChannelTable();
    setFormEnabled(false);
    setStatus(QString("Bank %1 selected. Click 'Read All' to load channels.").arg(bank == 10 ? 0 : bank));
}

void ProgramWindow::onChannelSelected(int row, int /*column*/) {
    m_currentChannel = row + 1;
    if (m_channelLoaded[row]) {
        loadChannelToForm(m_bankChannels[row]);
        setFormEnabled(true);
    } else {
        setFormEnabled(false);
        setStatus(QString("Channel %1 not loaded. Click 'Read' to load from scanner.").arg(m_currentChannel));
        m_readBtn->setFocus();
    }
}

void ProgramWindow::onReadChannel() {
    int idx = currentChannelIndex();
    setStatus("Reading channel...");
    m_readBtn->setEnabled(false);

    readChannelByIndex(idx, [this](const ChannelInfo& ch){
        m_readBtn->setEnabled(true);
        if (ch.index == 0) {
            setStatus("Failed to read channel.", false);
            return;
        }
        int row = m_currentChannel - 1;
        m_bankChannels[row]  = ch;
        m_channelLoaded[row] = true;
        updateChannelTableRow(row, ch);
        loadChannelToForm(ch);
        setFormEnabled(true);
        setStatus(QString("Channel %1 read successfully.").arg(m_currentChannel));
    });
}

void ProgramWindow::onWriteChannel() {
    ChannelInfo ch = channelFromForm();
    if (ch.frequency.isEmpty() || ch.frequency == "0") {
        QMessageBox::warning(this, "Invalid Frequency", "Please enter a valid frequency.");
        return;
    }

    setStatus("Writing channel...");
    m_writeBtn->setEnabled(false);

    m_scanner->enterProgramMode([this, ch](bool ok, const QString&){
        if (!ok) {
            m_writeBtn->setEnabled(true);
            setStatus("Failed to enter program mode.", false);
            return;
        }
        m_scanner->setChannelInfo(ch, [this, ch](bool ok2, const QString&){
            m_scanner->exitProgramMode([this, ch, ok2](bool, const QString&){
                m_writeBtn->setEnabled(true);
                if (ok2) {
                    int row = m_currentChannel - 1;
                    m_bankChannels[row]  = ch;
                    m_channelLoaded[row] = true;
                    updateChannelTableRow(row, ch);
                    setStatus(QString("Channel %1 written successfully.").arg(m_currentChannel));
                } else {
                    setStatus("Failed to write channel.", false);
                }
            });
        });
    });
}

void ProgramWindow::onDeleteChannel() {
    if (QMessageBox::question(this, "Delete Channel",
        QString("Delete channel %1? This cannot be undone.").arg(m_currentChannel),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    int idx = currentChannelIndex();
    setStatus("Deleting channel...");

    m_scanner->enterProgramMode([this, idx](bool ok, const QString&){
        if (!ok) { setStatus("Failed to enter program mode.", false); return; }
        m_scanner->deleteChannel(idx, [this](bool ok2, const QString&){
            m_scanner->exitProgramMode([this, ok2](bool, const QString&){
                if (ok2) {
                    int row = m_currentChannel - 1;
                    m_bankChannels[row]  = ChannelInfo();
                    m_channelLoaded[row] = true;
                    updateChannelTableRow(row, m_bankChannels[row]);
                    setFormEnabled(false);
                    setStatus(QString("Channel %1 deleted.").arg(m_currentChannel));
                } else {
                    setStatus("Failed to delete channel.", false);
                }
            });
        });
    });
}

void ProgramWindow::onReadAllChannels() {
    m_readAllBtn->setEnabled(false);
    m_writeAllBtn->setEnabled(false);
    m_progress->setValue(0);
    m_progress->show();
    setStatus("Reading all channels...");

    m_scanner->enterProgramMode([this](bool ok, const QString&){
        if (!ok) {
            m_readAllBtn->setEnabled(true);
            m_writeAllBtn->setEnabled(true);
            m_progress->hide();
            setStatus("Failed to enter program mode.", false);
            return;
        }

        // Read channels sequentially using a recursive lambda
        auto readNext = std::make_shared<std::function<void(int)>>();
        *readNext = [this, readNext](int i) {
            if (i >= 50) {
                m_scanner->exitProgramMode([this](bool, const QString&){
                    m_readAllBtn->setEnabled(true);
                    m_writeAllBtn->setEnabled(true);
                    m_progress->hide();
                    setStatus("All channels read successfully.");
                });
                return;
            }
            int idx = (m_currentBank - 1) * 50 + i + 1;
            m_scanner->getChannelInfo(idx, [this, i, readNext](bool ok, const QString& resp){
                m_progress->setValue(i + 1);
                if (ok) {
                    ChannelInfo ch = ScannerSerial::parseChannelInfo(resp);
                    m_bankChannels[i]  = ch;
                    m_channelLoaded[i] = true;
                    updateChannelTableRow(i, ch);
                }
                (*readNext)(i + 1);
            });
        };
        (*readNext)(0);
    });
}

void ProgramWindow::onWriteAllChannels() {
    // Collect channels that are loaded and non-empty
    QVector<int> toWrite;
    for (int i = 0; i < 50; ++i) {
        if (m_channelLoaded[i] && !m_bankChannels[i].isEmpty()) {
            toWrite.append(i);
        }
    }
    if (toWrite.isEmpty()) {
        setStatus("No channels to write (load them first).");
        return;
    }

    if (QMessageBox::question(this, "Write All Channels",
        QString("Write %1 channels to scanner?").arg(toWrite.size()),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    m_readAllBtn->setEnabled(false);
    m_writeAllBtn->setEnabled(false);
    m_progress->setMaximum(toWrite.size());
    m_progress->setValue(0);
    m_progress->show();
    setStatus("Writing channels...");

    m_scanner->enterProgramMode([this, toWrite](bool ok, const QString&){
        if (!ok) {
            m_readAllBtn->setEnabled(true);
            m_writeAllBtn->setEnabled(true);
            m_progress->hide();
            setStatus("Failed to enter program mode.", false);
            return;
        }

        auto writeNext = std::make_shared<std::function<void(int)>>();
        *writeNext = [this, toWrite, writeNext](int pos) {
            if (pos >= toWrite.size()) {
                m_scanner->exitProgramMode([this](bool, const QString&){
                    m_readAllBtn->setEnabled(true);
                    m_writeAllBtn->setEnabled(true);
                    m_progress->hide();
                    setStatus("All channels written successfully.");
                });
                return;
            }
            int i = toWrite[pos];
            m_scanner->setChannelInfo(m_bankChannels[i], [this, pos, writeNext](bool, const QString&){
                m_progress->setValue(pos + 1);
                (*writeNext)(pos + 1);
            });
        };
        (*writeNext)(0);
    });
}

void ProgramWindow::onChannelTableChanged() { /* handled in onChannelSelected */ }

void ProgramWindow::loadChannelToForm(const ChannelInfo& ch) {
    // Convert raw frequency to MHz string
    bool ok;
    long long f = ch.frequency.toLongLong(&ok);
    if (ok && f > 0) {
        m_freqEdit->setText(QString::number(f / 10000.0, 'f', 4));
    } else {
        m_freqEdit->clear();
    }

    m_nameEdit->setText(ch.name);

    int modIdx = m_modCombo->findText(ch.modulation);
    m_modCombo->setCurrentIndex(modIdx >= 0 ? modIdx : 0);

    // Find CTCSS/DCS code in combo
    int codeIdx = -1;
    for (int i = 0; i < m_ctcssDcsCombo->count(); ++i) {
        if (m_ctcssDcsCombo->itemData(i).toInt() == ch.ctcssDcs) {
            codeIdx = i;
            break;
        }
    }
    m_ctcssDcsCombo->setCurrentIndex(codeIdx >= 0 ? codeIdx : 0);

    // Delay
    int delayIdx = -1;
    for (int i = 0; i < m_delayCombo->count(); ++i) {
        if (m_delayCombo->itemData(i).toInt() == ch.delay) { delayIdx = i; break; }
    }
    m_delayCombo->setCurrentIndex(delayIdx >= 0 ? delayIdx : 2);

    m_lockoutCheck->setChecked(ch.lockout);
    m_priorityCheck->setChecked(ch.priority);
}

ChannelInfo ProgramWindow::channelFromForm() const {
    ChannelInfo ch;
    ch.index = currentChannelIndex();
    ch.name  = m_nameEdit->text().trimmed().left(16);

    bool ok;
    double freqMhz = m_freqEdit->text().toDouble(&ok);
    if (ok && freqMhz > 0) {
        ch.frequency = QString::number(static_cast<long long>(freqMhz * 10000));
    }

    ch.modulation = m_modCombo->currentText();
    ch.ctcssDcs   = m_ctcssDcsCombo->currentData().toInt();
    ch.delay      = m_delayCombo->currentData().toInt();
    ch.lockout    = m_lockoutCheck->isChecked();
    ch.priority   = m_priorityCheck->isChecked();
    return ch;
}

void ProgramWindow::updateChannelTableRow(int row, const ChannelInfo& ch) {
    auto setItem = [&](int col, const QString& text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter){
        auto* item = m_channelTable->item(row, col);
        if (!item) { item = new QTableWidgetItem; m_channelTable->setItem(row, col, item); }
        item->setText(text);
        item->setTextAlignment(align);
    };

    setItem(0, QString::number(row + 1), Qt::AlignCenter);
    setItem(1, ch.name.isEmpty() ? "---" : ch.name);

    if (!ch.isEmpty()) {
        setItem(2, ch.freqMhz() + " MHz", Qt::AlignRight | Qt::AlignVCenter);
    } else {
        setItem(2, "---", Qt::AlignRight | Qt::AlignVCenter);
        // Dim the row
        for (int c = 0; c < 3; ++c) {
            if (auto* item = m_channelTable->item(row, c))
                item->setForeground(QColor(0x66, 0x66, 0x66));
        }
    }
}

void ProgramWindow::setFormEnabled(bool enabled) {
    for (QWidget* w : std::initializer_list<QWidget*>{m_freqEdit, m_nameEdit, m_modCombo,
                                                      m_ctcssDcsCombo, m_delayCombo})
        w->setEnabled(enabled);
    m_lockoutCheck->setEnabled(enabled);
    m_priorityCheck->setEnabled(enabled);
    m_writeBtn->setEnabled(enabled);
    m_deleteBtn->setEnabled(enabled);
}

void ProgramWindow::setStatus(const QString& msg, bool ok) {
    m_statusLabel->setText(msg);
    m_statusLabel->setStyleSheet(ok ? "" : "color: #ff4444;");
}

int ProgramWindow::currentChannelIndex() const {
    return (m_currentBank - 1) * 50 + m_currentChannel;
}

void ProgramWindow::readChannelByIndex(int index, std::function<void(const ChannelInfo&)> cb) {
    m_scanner->enterProgramMode([this, index, cb](bool ok, const QString&){
        if (!ok) { cb(ChannelInfo{}); return; }
        m_scanner->getChannelInfo(index, [this, cb](bool ok2, const QString& resp){
            m_scanner->exitProgramMode([cb, ok2, resp](bool, const QString&){
                if (ok2) cb(ScannerSerial::parseChannelInfo(resp));
                else     cb(ChannelInfo{});
            });
        });
    });
}
