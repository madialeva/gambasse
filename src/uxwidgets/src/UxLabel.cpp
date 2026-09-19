#include <UxWidgets/UxLabel.h>

#include <QEnterEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>

namespace {
// Width reserved for the optional left image (15 px) plus its spacing.
constexpr int kImageSpacing = 19;
} // namespace

UxLabel::UxLabel(QWidget *parent)
    : QLabel(parent)
{
}

UxLabel::UxLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
}

void UxLabel::setMultiline(bool multiline)
{
    m_multiline = multiline;
    setWordWrap(multiline);
    if (multiline)
        m_fillWithDots = false;
    updateGeometry();
    update();
}

void UxLabel::setHighlight(bool highlight)
{
    if (m_highlight == highlight)
        return;
    m_highlight = highlight;
    if (!highlight && m_hovered) {
        m_hovered = false;
        unsetCursor();
        setFont(m_savedFont);
        setPalette(m_savedPalette);
    }
    update();
}

void UxLabel::setHighlightColor(const QColor &color)
{
    m_highlightColor = color;
    if (m_hovered) {
        QPalette palette = this->palette();
        palette.setColor(QPalette::WindowText, m_highlightColor);
        setPalette(palette);
    }
}

void UxLabel::setFillWithDots(bool fill)
{
    m_fillWithDots = fill && !m_multiline;
    update();
}

void UxLabel::setImage(const QPixmap &image)
{
    m_image = image;
    updateGeometry();
    update();
}

QSize UxLabel::sizeHint() const
{
    QSize size = QLabel::sizeHint();
    if (!m_image.isNull())
        size.rwidth() += kImageSpacing;
    return size;
}

QSize UxLabel::minimumSizeHint() const
{
    QSize size = QLabel::minimumSizeHint();
    if (!m_image.isNull())
        size.rwidth() += kImageSpacing;
    return size;
}

void UxLabel::enterEvent(QEnterEvent *event)
{
    if (m_highlight && isEnabled() && !m_hovered) {
        m_hovered = true;
        m_savedFont = font();
        m_savedPalette = palette();
        setCursor(Qt::PointingHandCursor);
        QFont hoverFont = font();
        hoverFont.setUnderline(true);
        setFont(hoverFont);
        QPalette hoverPalette = palette();
        hoverPalette.setColor(QPalette::WindowText, m_highlightColor);
        setPalette(hoverPalette);
    }
    QLabel::enterEvent(event);
}

void UxLabel::leaveEvent(QEvent *event)
{
    if (m_hovered) {
        m_hovered = false;
        unsetCursor();
        setFont(m_savedFont);
        setPalette(m_savedPalette);
    }
    QLabel::leaveEvent(event);
}

void UxLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_highlight && isEnabled() && event->button() == Qt::LeftButton)
        emit clicked();
    QLabel::mouseReleaseEvent(event);
}

void UxLabel::paintEvent(QPaintEvent *event)
{
    const bool drawDots = m_fillWithDots && !m_multiline;
    if (m_image.isNull() && !drawDots) {
        QLabel::paintEvent(event);
        return;
    }

    QPainter painter(this);
    QRect content = rect();

    if (!m_image.isNull()) {
        const int side = qMin(15, qMax(0, content.height()));
        const QPixmap scaled =
            m_image.scaled(side, side, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int y = content.top();
        if (alignment().testFlag(Qt::AlignVCenter))
            y = content.top() + (content.height() - scaled.height()) / 2;
        else if (alignment().testFlag(Qt::AlignBottom))
            y = content.bottom() - scaled.height() + 1;
        painter.drawPixmap(content.left(), y, scaled);
        content.setLeft(content.left() + kImageSpacing);
    }

    const QColor textColor = isEnabled()
        ? palette().color(QPalette::WindowText)
        : palette().color(QPalette::Disabled, QPalette::WindowText);
    painter.setPen(textColor);
    painter.setFont(font());

    const QString text = drawDots ? dottedText(content.width()) : QLabel::text();
    const int flags = static_cast<int>(alignment())
        | (m_multiline ? Qt::TextWordWrap : Qt::TextSingleLine);
    painter.drawText(content, flags, text);
}

QString UxLabel::dottedText(int availableWidth) const
{
    const QString text = QLabel::text();
    const QFontMetrics metrics(font());
    const int textWidth = metrics.horizontalAdvance(text);
    if (availableWidth <= textWidth)
        return text;

    const int dotWidth = metrics.horizontalAdvance(QLatin1Char('.'));
    if (dotWidth <= 0)
        return text;

    const int dots = qMax(0, (availableWidth - textWidth) / dotWidth - 1);
    return text + QString(dots, QLatin1Char('.'));
}
