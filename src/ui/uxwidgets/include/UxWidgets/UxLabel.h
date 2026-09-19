#pragma once

#include <QColor>
#include <QFont>
#include <QLabel>
#include <QPalette>
#include <QPixmap>

/**
 * Non-editable label specialized for the application's forms. It builds on
 * QLabel (text, alignment and word wrap) and adds:
 *  - an optional image drawn to the left of the text;
 *  - an optional fill-with-dots mode that pads the text up to the control
 *    width when it is not multiline;
 *  - an optional hover highlight (underline + configurable colour + hand
 *    cursor) that also emits clicked() on a left click.
 */
class UxLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(bool multiline READ multiline WRITE setMultiline)
    Q_PROPERTY(bool highlight READ highlight WRITE setHighlight)
    Q_PROPERTY(QColor highlightColor READ highlightColor WRITE setHighlightColor)
    Q_PROPERTY(bool fillWithDots READ fillWithDots WRITE setFillWithDots)
    Q_PROPERTY(QPixmap image READ image WRITE setImage)

public:
    explicit UxLabel(QWidget *parent = nullptr);
    explicit UxLabel(const QString &text, QWidget *parent = nullptr);

    bool multiline() const { return m_multiline; }
    void setMultiline(bool multiline);

    bool highlight() const { return m_highlight; }
    void setHighlight(bool highlight);

    QColor highlightColor() const { return m_highlightColor; }
    void setHighlightColor(const QColor &color);

    bool fillWithDots() const { return m_fillWithDots; }
    void setFillWithDots(bool fill);

    QPixmap image() const { return m_image; }
    void setImage(const QPixmap &image);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    /// Emitted on a left click while the hover highlight is active.
    void clicked();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QString dottedText(int availableWidth) const;

    bool m_multiline = false;
    bool m_highlight = false;
    bool m_fillWithDots = false;
    QColor m_highlightColor{0x1C, 0x3A, 0x75};
    QPixmap m_image;

    bool m_hovered = false;
    QFont m_savedFont;
    QPalette m_savedPalette;
};
