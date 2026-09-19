#include <UxWidgets/UxCheck.h>

#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPalette>

namespace {

// Focus background tints (powder blue).
const char *kFocusBackgroundLight = "#D9ECFA";
const char *kFocusBackgroundDark = "#1E3A55";

// Checked text colors (maroon).
const char *kCheckedTextLight = "#800000";
const char *kCheckedTextDark = "#E89A9A";

bool isDarkTheme(const QWidget *widget)
{
    return widget->palette().color(QPalette::Text).lightness() > 128;
}

} // namespace

UxCheck::UxCheck(QWidget *parent)
    : QCheckBox(parent)
{
    connect(this, &QCheckBox::toggled, this, [this] { updateStyle(); });
    updateStyle();
}

UxCheck::UxCheck(const QString &text, QWidget *parent)
    : QCheckBox(text, parent)
{
    connect(this, &QCheckBox::toggled, this, [this] { updateStyle(); });
    updateStyle();
}

void UxCheck::setReadOnly(bool readOnly)
{
    if (m_readOnly == readOnly)
        return;
    m_readOnly = readOnly;
    // Keep the normal look: never use the disabled state for view mode.
    setFocusPolicy(readOnly ? Qt::NoFocus : Qt::StrongFocus);
    if (readOnly)
        clearFocus();
    updateStyle();
}

void UxCheck::focusInEvent(QFocusEvent *event)
{
    QCheckBox::focusInEvent(event);
    updateStyle();
}

void UxCheck::focusOutEvent(QFocusEvent *event)
{
    QCheckBox::focusOutEvent(event);
    updateStyle();
}

void UxCheck::mousePressEvent(QMouseEvent *event)
{
    if (m_readOnly) {
        event->accept();
        return;
    }
    QCheckBox::mousePressEvent(event);
}

void UxCheck::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_readOnly) {
        event->accept();
        return;
    }
    QCheckBox::mouseReleaseEvent(event);
}

void UxCheck::keyPressEvent(QKeyEvent *event)
{
    if (m_readOnly && (event->key() == Qt::Key_Space || event->key() == Qt::Key_Select)) {
        event->accept();
        return;
    }
    QCheckBox::keyPressEvent(event);
}

void UxCheck::keyReleaseEvent(QKeyEvent *event)
{
    // QAbstractButton toggles on key release: block it as well, otherwise
    // Space would still flip a read-only check box.
    if (m_readOnly && (event->key() == Qt::Key_Space || event->key() == Qt::Key_Select)) {
        event->accept();
        return;
    }
    QCheckBox::keyReleaseEvent(event);
}

void UxCheck::changeEvent(QEvent *event)
{
    if (event
        && (event->type() == QEvent::ApplicationPaletteChange
            || event->type() == QEvent::PaletteChange))
        updateStyle();
    QCheckBox::changeEvent(event);
}

void UxCheck::updateStyle()
{
    const bool dark = isDarkTheme(this);
    QString style;
    if (hasFocus() && !m_readOnly)
        style += QStringLiteral("UxCheck { background:%1; }")
                     .arg(dark ? QLatin1String(kFocusBackgroundDark)
                               : QLatin1String(kFocusBackgroundLight));
    if (checkState() != Qt::Unchecked)
        style += QStringLiteral("UxCheck { color:%1; }")
                     .arg(dark ? QLatin1String(kCheckedTextDark)
                               : QLatin1String(kCheckedTextLight));
    // Setting the style sheet re-polishes the widget, which emits a palette
    // change back to us: only apply when it actually changed to avoid
    // recursing through changeEvent().
    if (style != styleSheet())
        setStyleSheet(style);
}
