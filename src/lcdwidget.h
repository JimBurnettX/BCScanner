#pragma once
#include <QWidget>
#include <QString>
#include <QTimer>

class LcdWidget : public QWidget {
    Q_OBJECT
public:
    enum class ScanState { Disconnected, Scanning, Hold, Programming, Error };

    explicit LcdWidget(QWidget* parent = nullptr);

    void setState(ScanState state);
    void setFrequency(const QString& mhz);
    void setChannelLabel(const QString& label);
    void setChannelNumber(int bank, int ch);
    void setModulation(const QString& mod);
    void setCtcssDcs(const QString& desc);
    void setSignalStrength(int level);    // 0-5
    void setBankEnabled(int bank, bool enabled); // bank 1-10 (10=displayed as 0)
    void setStatusMessage(const QString& msg);
    void setHoldTimer(int seconds);
    void setModel(const QString& model);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void drawBackground(QPainter& p, const QRectF& r);
    void drawBankRow(QPainter& p, const QRectF& r);
    void drawFrequency(QPainter& p, const QRectF& r);
    void drawInfoRow(QPainter& p, const QRectF& r);
    void drawStatusRow(QPainter& p, const QRectF& r);
    void drawSignalMeter(QPainter& p, const QRectF& r);

    // Draws a "ghost" text (dimmed inactive segments) then bright text on top
    void drawLcdText(QPainter& p, const QRectF& rect, const QString& text,
                     const QFont& font, Qt::Alignment align = Qt::AlignCenter);

    ScanState m_state = ScanState::Disconnected;
    QString   m_frequency  = "--- . ----";
    QString   m_label      = "NO CHANNEL";
    QString   m_modulation = "---";
    QString   m_ctcssDcs   = "";
    QString   m_status     = "DISCONNECTED";
    QString   m_model      = "";
    int       m_bank       = 1;
    int       m_channel    = 1;
    int       m_signal     = 0;
    int       m_holdTimer  = 0;
    bool      m_banks[10]  = {};

    QTimer* m_blinkTimer;
    bool    m_blinkOn = true;
};
