#include "debugwindow.h"
#include "scannerserial.h"
#include "stspoller.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDateTime>
#include <QScrollBar>
#include <QFont>
#include <QFrame>

DebugWindow::DebugWindow(ScannerSerial* scanner, StsPoller* stsPoller, QWidget* parent)
    : QDialog(parent, Qt::Window), m_scanner(scanner), m_stsPoller(stsPoller)
{
    setWindowTitle("Debug Console");
    setMinimumSize(640, 480);
    resize(720, 560);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);

    auto* tabs = new QTabWidget;
    tabs->setDocumentMode(true);
    layout->addWidget(tabs);

    auto* serialTab  = new QWidget;
    auto* consoleTab = new QWidget;
    tabs->addTab(serialTab,  "Serial Monitor");
    tabs->addTab(consoleTab, "Command Console");

    new QVBoxLayout(serialTab);
    new QVBoxLayout(consoleTab);

    buildSerialTab();
    buildConsoleTab();
    applyStyle();

    connect(m_scanner, &ScannerSerial::commandSent,
            this, &DebugWindow::onAllCommandSent);
    connect(m_scanner, &ScannerSerial::responseReceived,
            this, &DebugWindow::onAllResponseReceived);
    if (m_stsPoller) {
        connect(m_stsPoller, &StsPoller::statusReceived,
                this, &DebugWindow::onStsStatus);
        connect(m_stsPoller, &StsPoller::statusUpdated,
                this, &DebugWindow::onStsUpdated);
    }
}

// ---------------------------------------------------------------------------
// Tab builders
// ---------------------------------------------------------------------------

void DebugWindow::buildSerialTab()
{
    auto* tab    = qobject_cast<QWidget*>(findChildren<QWidget*>().first());
    // Get the actual serial tab from the QTabWidget
    auto* tabs   = findChild<QTabWidget*>();
    auto* serial = tabs->widget(0);
    auto* vbox   = qobject_cast<QVBoxLayout*>(serial->layout());

    m_serialLog = new QPlainTextEdit;
    m_serialLog->setReadOnly(true);
    m_serialLog->setMaximumBlockCount(2000);
    m_serialLog->setFont(QFont("Courier New", 9));
    vbox->addWidget(m_serialLog, 1);

    // STS parsed fields display
    auto* stsBox    = new QGroupBox("Last STS response — parsed fields");
    auto* stsLayout = new QVBoxLayout(stsBox);
    m_stsFields = new QLabel("(no STS response yet)");
    m_stsFields->setFont(QFont("Courier New", 9));
    m_stsFields->setWordWrap(true);
    stsLayout->addWidget(m_stsFields);
    vbox->addWidget(stsBox);

    // Controls row
    auto* ctrlRow = new QHBoxLayout;
    m_hideSts = new QCheckBox("Hide STS polls");
    m_hideSts->setChecked(true);
    m_hideSql = new QCheckBox("Hide SQL polls");
    m_hideSql->setChecked(true);
    auto* clearBtn = new QPushButton("Clear");
    clearBtn->setFixedWidth(70);
    ctrlRow->addWidget(m_hideSts);
    ctrlRow->addWidget(m_hideSql);
    ctrlRow->addStretch();
    ctrlRow->addWidget(clearBtn);
    vbox->addLayout(ctrlRow);

    connect(clearBtn, &QPushButton::clicked, m_serialLog, &QPlainTextEdit::clear);
    Q_UNUSED(tab);
}

void DebugWindow::buildConsoleTab()
{
    auto* tabs    = findChild<QTabWidget*>();
    auto* console = tabs->widget(1);
    auto* vbox    = qobject_cast<QVBoxLayout*>(console->layout());

    // Quick-access buttons for undocumented / useful commands
    auto* quickBox    = new QGroupBox("Quick Commands");
    auto* quickLayout = new QHBoxLayout(quickBox);
    quickLayout->setSpacing(4);

    const QStringList quickCmds = {
        "STS", "KEY,S,P", "KEY,H,P", "KEY,L,P", "KEY,F,P",
        "KEY,1,P", "KEY,2,P", "KEY,3,P", "KEY,4,P", "KEY,5,P",
        "KEY,6,P", "KEY,7,P", "KEY,8,P", "KEY,9,P", "KEY,0,P"
    };
    for (const QString& cmd : quickCmds) {
        auto* btn = new QPushButton(cmd);
        btn->setFixedHeight(28);
        btn->setMinimumWidth(50);
        btn->setStyleSheet(
            "QPushButton { background:#1a3a1a; color:#00cc00; border:1px solid #336633;"
            "              border-radius:3px; font-size:11px; padding:0 6px; }"
            "QPushButton:hover { background:#2a5a2a; }"
            "QPushButton:pressed { background:#0a1a0a; }");
        connect(btn, &QPushButton::clicked, this, [this, cmd](){ onQuickKey(cmd); });
        quickLayout->addWidget(btn);
    }
    quickLayout->addStretch();
    vbox->addWidget(quickBox);

    // Command input row
    auto* inputRow = new QHBoxLayout;
    auto* cmdLabel = new QLabel("Command:");
    m_cmdInput = new QLineEdit;
    m_cmdInput->setPlaceholderText("e.g.  STS  or  KEY,H  or  CIN,001");
    m_cmdInput->setFont(QFont("Courier New", 10));
    auto* sendBtn = new QPushButton("Send");
    sendBtn->setFixedWidth(70);
    sendBtn->setDefault(true);
    inputRow->addWidget(cmdLabel);
    inputRow->addWidget(m_cmdInput, 1);
    inputRow->addWidget(sendBtn);
    vbox->addLayout(inputRow);

    // Response log
    m_consoleLog = new QPlainTextEdit;
    m_consoleLog->setReadOnly(true);
    m_consoleLog->setMaximumBlockCount(1000);
    m_consoleLog->setFont(QFont("Courier New", 9));
    vbox->addWidget(m_consoleLog, 1);

    auto* clearBtn2 = new QPushButton("Clear Log");
    clearBtn2->setFixedWidth(80);
    auto* row2 = new QHBoxLayout;
    row2->addStretch();
    row2->addWidget(clearBtn2);
    vbox->addLayout(row2);

    connect(sendBtn,    &QPushButton::clicked,   this, &DebugWindow::onSendClicked);
    connect(m_cmdInput, &QLineEdit::returnPressed, this, &DebugWindow::onSendClicked);
    connect(clearBtn2,  &QPushButton::clicked,   m_consoleLog, &QPlainTextEdit::clear);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void DebugWindow::onAllCommandSent(const QString& cmd)
{
    if (m_hideSts && m_hideSts->isChecked() && cmd.startsWith("STS")) return;
    if (m_hideSql && m_hideSql->isChecked() && cmd.startsWith("SQL")) return;
    const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    appendSerial(ts + "  >> " + cmd, "#88bbff");
}

void DebugWindow::onAllResponseReceived(const QString& resp)
{
    if (m_hideSts && m_hideSts->isChecked() && resp.startsWith("STS")) return;
    if (m_hideSql && m_hideSql->isChecked() && resp.startsWith("SQL")) return;
    const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    const QString color = resp.startsWith("NG") || resp.startsWith("ERR")
                          ? "#ff6666" : "#d0d0d0";
    appendSerial(ts + "  << " + resp, color);
}

void DebugWindow::onStsStatus(const QString& raw)
{
    // Raw breakdown — show every comma-separated field with its index
    if (!m_stsFields) return;
    const QStringList f = raw.split(',');
    QString parsed = "<b>Raw:</b> " + raw.left(120).toHtmlEscaped() + "<br>";
    for (int i = 0; i < f.size(); ++i)
        parsed += QString("  [%1] = <tt>%2</tt><br>").arg(i).arg(f[i].toHtmlEscaped());
    m_stsFields->setText(parsed);
}

void DebugWindow::onStsUpdated(const StsStatus& status)
{
    if (!m_stsFields) return;

    static const char* stateNames[] = { "Unknown", "Scanning", "Active", "Search", "Menu" };
    const int si = static_cast<int>(status.state);
    const char* stateName = (si >= 0 && si <= 4) ? stateNames[si] : "?";

    QString color = "#aaaaaa";
    if (status.state == StsStatus::State::Active)   color = "#00ff00";
    if (status.state == StsStatus::State::Scanning) color = "#88ddff";
    if (status.state == StsStatus::State::Menu)     color = "#ffaa00";

    QString html = QString("<b>State:</b> <span style='color:%1'>%2</span><br>").arg(color, stateName);
    if (!status.channelLabel.isEmpty())
        html += QString("<b>Label:</b> %1<br>").arg(status.channelLabel.toHtmlEscaped());
    if (!status.frequency.isEmpty())
        html += QString("<b>Freq:</b> %1 MHz<br>").arg(status.frequency.toHtmlEscaped());
    if (status.channelNumber > 0)
        html += QString("<b>Channel:</b> %1<br>").arg(status.channelNumber);
    html += QString("<b>Squelch open:</b> %1<br>").arg(status.squelchOpen ? "YES" : "no");
    html += "<hr style='border:1px solid #333'>";
    const QStringList f = status.raw.split(',');
    for (int i = 0; i < f.size(); ++i)
        html += QString("  [%1] = <tt>%2</tt><br>").arg(i).arg(f[i].left(30).toHtmlEscaped());

    m_stsFields->setText(html);
}

void DebugWindow::onSendClicked()
{
    QString cmd = m_cmdInput->text().trimmed();
    if (cmd.isEmpty() || !m_scanner->isConnected()) return;

    const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    appendConsole(ts + "  >> " + cmd, "#88bbff");

    m_scanner->sendCommand(cmd, [this, ts](bool ok, const QString& resp) {
        const QString color = !ok || resp.startsWith("NG") || resp.startsWith("ERR")
                              ? "#ff6666" : "#00cc00";
        appendConsole(ts + "  << " + resp, color);
    });
    m_cmdInput->clear();
}

void DebugWindow::onQuickKey(const QString& key)
{
    m_cmdInput->setText(key);
    onSendClicked();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void DebugWindow::appendSerial(const QString& line, const QString& color)
{
    if (!m_serialLog) return;
    m_serialLog->appendHtml(
        QString("<span style='color:%1'>%2</span>").arg(color, line.toHtmlEscaped()));
    m_serialLog->verticalScrollBar()->setValue(
        m_serialLog->verticalScrollBar()->maximum());
}

void DebugWindow::appendConsole(const QString& line, const QString& color)
{
    if (!m_consoleLog) return;
    m_consoleLog->appendHtml(
        QString("<span style='color:%1'>%2</span>").arg(color, line.toHtmlEscaped()));
    m_consoleLog->verticalScrollBar()->setValue(
        m_consoleLog->verticalScrollBar()->maximum());
}

void DebugWindow::applyStyle()
{
    setStyleSheet(R"(
        QDialog, QWidget {
            background-color: #1e1e1e;
            color: #d0d0d0;
        }
        QTabWidget::pane { border: 1px solid #333; }
        QTabBar::tab {
            background: #252525;
            color: #888;
            border: 1px solid #333;
            padding: 4px 12px;
        }
        QTabBar::tab:selected { background: #1e1e1e; color: #d0d0d0; }
        QGroupBox {
            color: #888;
            border: 1px solid #333;
            border-radius: 4px;
            margin-top: 8px;
            font-size: 11px;
        }
        QGroupBox::title { subcontrol-origin: margin; padding: 0 4px; }
        QPlainTextEdit {
            background: #111;
            color: #d0d0d0;
            border: 1px solid #333;
        }
        QLineEdit {
            background: #111;
            color: #00cc00;
            border: 1px solid #444;
            border-radius: 3px;
            padding: 4px 6px;
        }
        QLabel { color: #d0d0d0; }
        QCheckBox { color: #aaa; }
        QPushButton {
            background: #2a2a2a;
            color: #d0d0d0;
            border: 1px solid #444;
            border-radius: 3px;
            padding: 4px 10px;
        }
        QPushButton:hover { background: #3a3a3a; }
        QPushButton:pressed { background: #111; }
    )");
}
