#include <ui/window/TitleBar.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QToolButton>

namespace gambasse {

namespace {

// Same height as the toolbar below, so the custom bar replaces it visually.
constexpr int kBarHeight = 32;
constexpr int kLogoHeight = 24;
constexpr int kWindowButtonWidth = 44;

} // namespace

TitleBar::TitleBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(kBarHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(8, 0, 2, 0);
    m_layout->setSpacing(6);

    const QPixmap logo(QStringLiteral(":/img/logo.svg"));
    m_logo = new QLabel(this);
    if (!logo.isNull())
        m_logo->setPixmap(logo.scaledToHeight(kLogoHeight, Qt::SmoothTransformation));
    m_layout->addWidget(m_logo);

    // Application name is fixed in every language.
    m_name = new QLabel(QStringLiteral("Gambasse"), this);
    QFont nameFont = m_name->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 1);
    m_name->setFont(nameFont);
    m_layout->addWidget(m_name);

    m_layout->addStretch(1);

    auto makeWindowButton = [this](const QString& glyph) {
        auto* b = new QToolButton(this);
        b->setText(glyph);
        b->setFixedSize(kWindowButtonWidth, kBarHeight - 4);
        QFont buttonFont = b->font();
        buttonFont.setBold(true);
        b->setFont(buttonFont);
        b->setFocusPolicy(Qt::NoFocus);
        m_layout->addWidget(b);
        return b;
    };
    m_minimizeButton = makeWindowButton(QStringLiteral("\u2014")); // em dash
    m_maximizeButton = makeWindowButton(QStringLiteral("\u25A1")); // white square
    m_closeButton = makeWindowButton(QStringLiteral("\u2715"));    // ballot X

    connect(m_minimizeButton, &QToolButton::clicked, this, [this]() {
        if (QWidget* w = window())
            w->showMinimized();
    });
    connect(m_maximizeButton, &QToolButton::clicked, this, &TitleBar::toggleMaximize);
    connect(m_closeButton, &QToolButton::clicked, this, [this]() {
        if (QWidget* w = window())
            w->close();
    });

    retranslateUi();
}

void TitleBar::insertControl(QWidget* control) {
    m_layout->insertWidget(m_layout->indexOf(m_minimizeButton), control);
}

void TitleBar::setTitle(const QString& title) {
    if (m_name)
        m_name->setText(title);
}

QList<QToolButton*> TitleBar::windowButtons() const {
    return {m_minimizeButton, m_maximizeButton, m_closeButton};
}

void TitleBar::setCloseOnly(bool closeOnly) {
    m_closeOnly = closeOnly;
    if (m_minimizeButton)
        m_minimizeButton->setVisible(!closeOnly);
    if (m_maximizeButton)
        m_maximizeButton->setVisible(!closeOnly);
}

void TitleBar::refreshMaximizeGlyph() {
    if (!m_maximizeButton)
        return;
    const bool maximized = window() && window()->isMaximized();
    // White square = maximize; shadowed square = restore.
    m_maximizeButton->setText(maximized ? QStringLiteral("\u2750") : QStringLiteral("\u25A1"));
}

void TitleBar::retranslateUi() {
    if (m_minimizeButton)
        m_minimizeButton->setToolTip(tr("Minimize"));
    if (m_maximizeButton) {
        const bool maximized = window() && window()->isMaximized();
        m_maximizeButton->setToolTip(maximized ? tr("Restore") : tr("Maximize"));
    }
    if (m_closeButton)
        m_closeButton->setToolTip(tr("Close"));
}

void TitleBar::toggleMaximize() {
    QWidget* w = window();
    if (!w)
        return;
    if (w->isMaximized())
        w->showNormal();
    else
        w->showMaximized();
    refreshMaximizeGlyph();
}

void TitleBar::mousePressEvent(QMouseEvent* event) {
    if (event && event->button() == Qt::LeftButton && window() && !window()->isMaximized()) {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && event && window())
        window()->move(event->globalPosition().toPoint() - m_dragOffset);
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent* event) {
    m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event && event->button() == Qt::LeftButton && !m_closeOnly)
        toggleMaximize();
    QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::changeEvent(QEvent* event) {
    if (event && event->type() == QEvent::LanguageChange)
        retranslateUi();
    QWidget::changeEvent(event);
}

} // namespace gambasse
