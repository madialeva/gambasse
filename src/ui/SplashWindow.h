#pragma once

#include <QWidget>
#include <QPixmap>

class QLabel;

namespace gambasse {

// Frameless, centered splash screen with the logo image and a credits strip at
// the bottom. It closes after three seconds and emits finished().
class SplashWindow : public QWidget {
    Q_OBJECT
public:
    explicit SplashWindow(QWidget* parent = nullptr);

signals:
    void finished();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupCreditsStrip();

    QPixmap m_background;
    QLabel* m_creditsIcon = nullptr;
    QLabel* m_creditsText = nullptr;
};

} // namespace gambasse