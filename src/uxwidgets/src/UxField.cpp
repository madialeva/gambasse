#include "UxWidgets/UxField.h"

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QFocusEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPoint>

namespace {

// The default selector-button icon is an ellipsis. It is painted with the
// palette text color so it remains visible in light and dark themes.
QIcon defaultEllipsisIcon()
{
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QColor color = QApplication::palette().color(QPalette::Text);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    for (int i = 0; i < 3; ++i)
        p.drawEllipse(QPoint(3 + i * 5, 11), 1, 1);

    return QIcon(pm);
}

} // namespace

UxField::UxField(QWidget *parent)
    : QLineEdit(parent)
{
    connect(this, &QLineEdit::textChanged, this, [this] { updateBackground(); });
}

UxField::~UxField() = default;

void UxField::setRequired(bool required)
{
    m_required = required;
    updateBackground();
}

void UxField::ensureAction()
{
    if (m_action)
        return;

    m_action = new QAction(this);
    m_action->setIcon(m_icon.isNull() ? defaultEllipsisIcon() : m_icon);
    connect(m_action, &QAction::triggered, this, [this] { onActionTriggered(); });
}

void UxField::setShowSelector(bool show)
{
    m_showSelector = show;
    if (show) {
        ensureAction();
        removeAction(m_action);
        addAction(m_action, QLineEdit::TrailingPosition);
    } else if (m_action) {
        removeAction(m_action);
    }
}

void UxField::setIcon(const QIcon &icon)
{
    m_icon = icon;
    if (m_action)
        m_action->setIcon(icon.isNull() ? defaultEllipsisIcon() : icon);
}

void UxField::onActionTriggered()
{
    emit selectionRequested();
}

void UxField::setLightFocusColor(const QColor &color)
{
    m_lightFocusColor = color;
    if (hasFocus())
        updateBackground();
}

void UxField::setDarkFocusColor(const QColor &color)
{
    m_darkFocusColor = color;
    if (hasFocus())
        updateBackground();
}

void UxField::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    updateBackground();
}

void UxField::focusOutEvent(QFocusEvent *event)
{
    QLineEdit::focusOutEvent(event);
    updateBackground();
}

void UxField::changeEvent(QEvent *event)
{
    if ((event->type() == QEvent::ApplicationPaletteChange
         || event->type() == QEvent::PaletteChange)
        && !m_updating) {
        updateBackground();
    }
    QLineEdit::changeEvent(event);
}

bool UxField::isDarkTheme() const
{
    return palette().color(QPalette::Text).lightness() > 128;
}

void UxField::updateBackground()
{
    QColor background;
    if (hasFocus())
        background = isDarkTheme() ? m_darkFocusColor : m_lightFocusColor;
    else if (m_required && text().trimmed().isEmpty())
        background = QColor(0xFF, 0xE4, 0xC4);
    else
        background = QApplication::palette().color(QPalette::Base);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Base, background);

    m_updating = true;
    setPalette(palette);
    m_updating = false;
}
