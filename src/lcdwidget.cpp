#include "lcdwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QTimer>
#include <cmath>

static const QColor LCD_BG       = QColor(0x0a, 0x14, 0x0a);
static const QColor LCD_BEZEL    = QColor(0x1c, 0x22, 0x1c);
static const QColor LCD_ACTIVE   = QColor(0x33, 0xff, 0x33);
static const QColor LCD_GHOST    = QColor(0x11, 0x28, 0x11);
static const QColor LCD_DIM      = QColor(0x1a, 0x88, 0x1a);
static const QColor LCD_HOLD     = QColor(0xff, 0xaa, 0x00);
static const QColor LCD_PROG     = QColor(0x00, 0xcc, 0xff);
static const QColor LCD_ERROR    = QColor(0xff, 0x44, 0x44);
static const QColor BEZEL_OUTER  = QColor(0x2a, 0x2a, 0x2a);

LcdWidget::LcdWidget(QWidget* parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumSize(400, 180);

    for (int i = 0; i < 10; ++i) m_banks[i] = true;

    m_blinkTimer = new QTimer(this);
    m_blinkTimer->setInterval(500);
    connect(m_blinkTimer, &QTimer::timeout, this, [this](){
        m_blinkOn = !m_blinkOn;
        update();
    });
    m_blinkTimer->start();
}

QSize LcdWidget::sizeHint() const { return {500, 200}; }

void LcdWidget::setState(ScanState state) {
    m_state = state;
    switch (state) {
    case ScanState::Disconnected: m_status = "DISCONNECTED"; break;
    case ScanState::Scanning:     m_status = "SCANNING";     break;
    case ScanState::Hold:         m_status = "HOLD";         break;
    case ScanState::Programming:  m_status = "PROGRAMMING";  break;
    case ScanState::Error:        m_status = "ERROR";        break;
    }
    update();
}
void LcdWidget::setFrequency(const QString& mhz)      { m_frequency  = mhz;  update(); }
void LcdWidget::setChannelLabel(const QString& label)  { m_label      = label.isEmpty() ? "--------" : label; update(); }
void LcdWidget::setChannelNumber(int bank, int ch)     { m_bank = bank; m_channel = ch; update(); }
void LcdWidget::setModulation(const QString& mod)      { m_modulation = mod;  update(); }
void LcdWidget::setCtcssDcs(const QString& desc)       { m_ctcssDcs   = desc; update(); }
void LcdWidget::setSignalStrength(int level)           { m_signal     = level; update(); }
void LcdWidget::setStatusMessage(const QString& msg)   { m_status     = msg;  update(); }
void LcdWidget::setHoldTimer(int seconds)              { m_holdTimer  = seconds; update(); }
void LcdWidget::setModel(const QString& model)         { m_model      = model; update(); }

void LcdWidget::setBankEnabled(int bank, bool enabled) {
    int idx = (bank == 10) ? 9 : bank - 1;
    if (idx >= 0 && idx < 10) { m_banks[idx] = enabled; update(); }
}

void LcdWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

void LcdWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    QRectF screen(0, 0, width(), height());

    // Outer bezel
    p.setBrush(BEZEL_OUTER);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(screen, 12, 12);

    // Inner LCD area
    QRectF lcd = screen.adjusted(8, 8, -8, -8);
    drawBackground(p, lcd);

    double margin = lcd.width() * 0.03;
    QRectF inner = lcd.adjusted(margin, margin*0.5, -margin, -margin*0.5);

    double totalH = inner.height();
    double bankH  = totalH * 0.18;
    double freqH  = totalH * 0.38;
    double infoH  = totalH * 0.24;
    double statH  = totalH * 0.20;

    QRectF bankRect(inner.left(), inner.top(), inner.width(), bankH);
    QRectF freqRect(inner.left(), bankRect.bottom(), inner.width(), freqH);
    QRectF infoRect(inner.left(), freqRect.bottom(), inner.width(), infoH);
    QRectF statRect(inner.left(), infoRect.bottom(), inner.width(), statH);

    drawBankRow(p, bankRect);
    drawFrequency(p, freqRect);
    drawInfoRow(p, infoRect);
    drawStatusRow(p, statRect);
}

void LcdWidget::drawBackground(QPainter& p, const QRectF& r) {
    QLinearGradient grad(r.topLeft(), r.bottomLeft());
    grad.setColorAt(0.0, QColor(0x0c, 0x18, 0x0c));
    grad.setColorAt(1.0, LCD_BG);
    p.setBrush(grad);
    p.setPen(QPen(QColor(0x00, 0x44, 0x00), 1.5));
    p.drawRoundedRect(r, 8, 8);

    // Subtle scan-line texture
    p.setPen(QPen(QColor(0,0,0,15), 1));
    for (double y = r.top(); y < r.bottom(); y += 3) {
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
    }
}

void LcdWidget::drawBankRow(QPainter& p, const QRectF& r) {
    // Bank indicators: 1 2 3 4 5 6 7 8 9 0
    const int numBanks = 10;
    double cellW = r.width() / (numBanks + 4.0); // extra space for signal meter
    double cellH = r.height() * 0.75;
    double yc = r.center().y();

    QFont bankFont("Courier New", std::max(6.0, cellH * 0.7));
    bankFont.setBold(true);
    p.setFont(bankFont);

    // Labels in display order: 1,2,3,4,5,6,7,8,9,0
    const char* labels[10] = {"1","2","3","4","5","6","7","8","9","0"};

    for (int i = 0; i < numBanks; ++i) {
        double x = r.left() + i * cellW;
        QRectF cell(x, yc - cellH/2, cellW * 0.9, cellH);

        bool active = m_banks[i];
        QColor bg   = active ? QColor(0x00, 0x33, 0x00) : LCD_BG;
        QColor fg   = active ? LCD_ACTIVE : LCD_GHOST;

        p.setBrush(bg);
        p.setPen(QPen(active ? QColor(0x00, 0x88, 0x00) : QColor(0x11, 0x22, 0x11), 0.5));
        p.drawRoundedRect(cell, 2, 2);
        p.setPen(fg);
        p.drawText(cell, Qt::AlignCenter, labels[i]);
    }

    // Signal meter on the right
    double sigX = r.left() + numBanks * cellW + cellW * 0.3;
    double sigW = r.right() - sigX - 2;
    double barH = cellH * 0.6;
    double barW = sigW / 6.0;
    for (int i = 0; i < 5; ++i) {
        double h  = barH * (0.3 + 0.14 * i);
        double bx = sigX + i * barW;
        double by = yc + barH * 0.3 - h;
        QRectF bar(bx, by, barW * 0.7, h);
        QColor c = (i < m_signal) ? QColor(0x00, 0xdd + i*6, 0x00) : LCD_GHOST;
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawRect(bar);
    }

    // "SIG" label
    QFont sigFont("Courier New", std::max(5.0, cellH * 0.45));
    p.setFont(sigFont);
    p.setPen(LCD_DIM);
    p.drawText(QRectF(sigX, r.top(), sigW, r.height()*0.45), Qt::AlignCenter, "SIG");
}

void LcdWidget::drawFrequency(QPainter& p, const QRectF& r) {
    // Large LCD-style frequency display
    double fontSize = r.height() * 0.62;
    QFont freqFont("Courier New", std::max(10.0, fontSize));
    freqFont.setBold(true);

    QString display = m_frequency.isEmpty() ? "--- . ----" : m_frequency;
    if (display.length() > 4 && !display.contains(' ') && !display.contains('-')) {
        // Insert space around decimal for readability if it's a plain number
        // e.g. "462.5625" -> already has dot
    }
    display = display + " MHz";

    QColor textColor;
    switch (m_state) {
    case ScanState::Hold:        textColor = LCD_HOLD;   break;
    case ScanState::Programming: textColor = LCD_PROG;   break;
    case ScanState::Error:       textColor = LCD_ERROR;  break;
    case ScanState::Scanning:
        textColor = LCD_ACTIVE;
        break;
    default: textColor = LCD_GHOST; break;
    }

    drawLcdText(p, r, display, freqFont, Qt::AlignCenter);

    // Override color
    p.setFont(freqFont);
    p.setPen(textColor);
    // Draw ghost segments first
    p.setPen(QColor(LCD_GHOST.red(), LCD_GHOST.green(), LCD_GHOST.blue(), 60));
    p.drawText(r, Qt::AlignCenter, "888.8888 MHz");
    p.setPen(textColor);
    p.drawText(r, Qt::AlignCenter, display);
}

void LcdWidget::drawInfoRow(QPainter& p, const QRectF& r) {
    double fontSize = r.height() * 0.38;
    QFont infoFont("Courier New", std::max(7.0, fontSize));
    infoFont.setBold(false);

    // Bank/Channel display on left
    int dispBank = (m_bank == 10) ? 0 : m_bank;
    QString chanStr = QString("BNK%1 CH%2").arg(dispBank).arg(m_channel, 2, 10, QChar('0'));

    // Channel label in center
    QString labelStr = m_label.isEmpty() ? "--------" : m_label.toUpper();

    // Modulation on right
    QString modStr = m_modulation.isEmpty() ? "---" : m_modulation;

    double thirdW = r.width() / 3.0;
    QRectF leftR  (r.left(),           r.top(), thirdW, r.height());
    QRectF centerR(r.left() + thirdW,  r.top(), thirdW, r.height());
    QRectF rightR (r.left() + 2*thirdW, r.top(), thirdW, r.height());

    p.setFont(infoFont);
    p.setPen(LCD_DIM);
    p.drawText(leftR,   Qt::AlignVCenter | Qt::AlignLeft,   chanStr);
    p.setPen(LCD_ACTIVE);
    p.drawText(centerR, Qt::AlignCenter,                     labelStr);
    p.setPen(LCD_DIM);
    p.drawText(rightR,  Qt::AlignVCenter | Qt::AlignRight,  modStr);

    // CTCSS/DCS below if present
    if (!m_ctcssDcs.isEmpty() && m_ctcssDcs != "None / All" && m_ctcssDcs != "None") {
        double subSize = fontSize * 0.75;
        QFont subFont("Courier New", std::max(6.0, subSize));
        QRectF subR(r.left(), r.bottom() - r.height()*0.35, r.width(), r.height()*0.35);
        p.setFont(subFont);
        p.setPen(QColor(0x00, 0xcc, 0xcc));
        p.drawText(subR, Qt::AlignCenter, m_ctcssDcs);
    }
}

void LcdWidget::drawStatusRow(QPainter& p, const QRectF& r) {
    double fontSize = r.height() * 0.45;
    QFont statusFont("Courier New", std::max(6.0, fontSize));
    statusFont.setBold(true);

    QColor statusColor;
    switch (m_state) {
    case ScanState::Scanning:     statusColor = QColor(0x00, 0xcc, 0x00); break;
    case ScanState::Hold:         statusColor = LCD_HOLD;  break;
    case ScanState::Programming:  statusColor = LCD_PROG;  break;
    case ScanState::Error:        statusColor = LCD_ERROR; break;
    default:                      statusColor = LCD_GHOST; break;
    }

    // Status indicator background pill
    p.setFont(statusFont);
    QFontMetrics fm(statusFont);
    QRectF textR = QRectF(fm.boundingRect(m_status));
    double pillW = textR.width() + r.height() * 0.8;
    double pillH = r.height() * 0.7;
    QRectF pill(r.left(), r.center().y() - pillH/2, pillW, pillH);
    p.setBrush(QColor(statusColor.red(), statusColor.green(), statusColor.blue(), 30));
    p.setPen(QPen(statusColor, 0.8));
    p.drawRoundedRect(pill, pillH/2, pillH/2);
    p.setPen(statusColor);
    p.drawText(pill, Qt::AlignCenter, m_status);

    // Countdown timer (right side) — show in any state when a transmission is being timed
    QString rightText;
    if (m_holdTimer > 0) {
        rightText = QString("SKIP %1s").arg(m_holdTimer);
    } else if (!m_model.isEmpty()) {
        rightText = m_model;
    }

    if (!rightText.isEmpty()) {
        QFont rFont("Courier New", std::max(6.0, fontSize * 0.8));
        p.setFont(rFont);
        p.setPen(LCD_DIM);
        p.drawText(QRectF(r.left() + pillW + 4, r.top(), r.right() - pillW - r.left() - 4, r.height()),
                   Qt::AlignVCenter | Qt::AlignRight, rightText);
    }
}

void LcdWidget::drawLcdText(QPainter& p, const QRectF& rect, const QString& text,
                             const QFont& font, Qt::Alignment align) {
    p.setFont(font);
    p.setPen(LCD_GHOST);
    p.drawText(rect, align, text);
}
