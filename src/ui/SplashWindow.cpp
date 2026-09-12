#include "SplashWindow.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>

namespace gambasse {

namespace {
constexpr int kSplashWidth = 560;
constexpr int kImageHeight = 430;
constexpr int kCreditsHeight = 40;
const QString kGitHubUrl = QStringLiteral("https://github.com/madialeva/gambasse");
} // namespace

SplashWindow::SplashWindow(QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::SplashScreen) {

    m_background = QPixmap(QStringLiteral(":/img/logo-historias.jpg"));

    setFixedSize(kSplashWidth, kImageHeight + kCreditsHeight);

    setupCreditsStrip();

    setAttribute(Qt::WA_DeleteOnClose, false);

    // Center on the primary screen.
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect g = screen->availableGeometry();
        move(g.center() - rect().center());
    }

    // Close and notify after 3 seconds (equivalent to FrmSplash Timer1).
    QTimer::singleShot(3000, this, [this]() {
        emit finished();
        close();
    });
}

void SplashWindow::setupCreditsStrip() {
    auto* strip = new QWidget(this);
    strip->setGeometry(0, kImageHeight, width(), kCreditsHeight);
    strip->setStyleSheet(QStringLiteral("background-color:#24292E;"));

    m_creditsIcon = new QLabel(strip);
    m_creditsIcon->setPixmap(QIcon(QStringLiteral(":/img/github.svg")).pixmap(16, 16));

    m_creditsText = new QLabel(strip);
    m_creditsText->setTextFormat(Qt::RichText);
    m_creditsText->setOpenExternalLinks(true);
    m_creditsText->setStyleSheet(
        QStringLiteral("background:transparent; color:#FFFFFF; font-size:12px;"));
    m_creditsText->setText(
        tr("Project by Juan Franco, available on "
           "<a href=\"%1\" style=\"color:#58A6FF; text-decoration:none;\">GitHub</a>.")
            .arg(kGitHubUrl));

    auto* layout = new QHBoxLayout(strip);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(6);
    layout->addStretch(1);
    layout->addWidget(m_creditsIcon, 0, Qt::AlignVCenter);
    layout->addWidget(m_creditsText, 0, Qt::AlignVCenter);
    layout->addStretch(1);
}

void SplashWindow::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    if (!m_background.isNull())
        p.drawPixmap(QRect(0, 0, width(), kImageHeight), m_background);
    else
        p.fillRect(rect(), Qt::white);
}

} // namespace gambasse