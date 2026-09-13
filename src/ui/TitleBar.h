#pragma once

#include <QPoint>
#include <QWidget>

#include <QList>

class QLabel;
class QToolButton;
class QHBoxLayout;
class QMouseEvent;
class QEvent;

namespace gambasse {

// Custom title bar for the frameless main window: application logo and name
// on the left, window controls (minimize / maximize-restore / close) on the
// right. Extra controls (language, theme) can be inserted before the window
// buttons with insertControl(). Dragging the bar moves the window;
// double-clicking toggles maximize/restore. Edge resizing is handled by the
// owning window through an event filter.
class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget* parent = nullptr);

    // Inserts a control (language/theme buttons) just before the window
    // buttons, keeping them rightmost.
    void insertControl(QWidget* control);

    QList<QToolButton*> windowButtons() const;
    QToolButton* closeButton() const { return m_closeButton; }
    void refreshMaximizeGlyph();
    void retranslateUi();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void toggleMaximize();

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_logo = nullptr;
    QLabel* m_name = nullptr;
    QToolButton* m_minimizeButton = nullptr;
    QToolButton* m_maximizeButton = nullptr;
    QToolButton* m_closeButton = nullptr;

    bool m_dragging = false;
    QPoint m_dragOffset;
};

} // namespace gambasse
