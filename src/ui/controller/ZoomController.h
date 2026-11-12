#pragma once

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QWindow>
#include <QtQml/qqmlregistration.h>

#include <ui/InterfaceSettings.h>

namespace gambasse {

// Zoom of one history or consultation window (FormWindow).
//
// The letter size the user chooses (InterfaceSettings.letterSize) is one for
// every window; this controller works out the size its window shows: the
// largest, up to the chosen one, at which the window fits whole in the
// available area of its screen. The window is its base layout scaled by
// `factor` and measures `windowSize`.
//
// It also places the window: centred over its transient parent (the main
// window) the first time it is shown and whenever the size it shows changes,
// where it was left otherwise, and always moved just enough to lie whole in
// the available area.
class ZoomController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(InterfaceSettings* settings READ settings WRITE setSettings NOTIFY settingsChanged)
    Q_PROPERTY(QWindow* window READ window WRITE setWindow NOTIFY windowChanged)
    // Size of the window at the base letter size, title bar and frame included.
    Q_PROPERTY(QSize baseSize READ baseSize WRITE setBaseSize NOTIFY baseSizeChanged)
    // The available area of the window's screen (what the task bar leaves);
    // fixed by the tests, whose offscreen screen is smaller than the windows.
    Q_PROPERTY(QRect availableArea READ availableArea WRITE setAvailableArea RESET
                   resetAvailableArea NOTIFY availableAreaChanged)
    Q_PROPERTY(int shownLetterSize READ shownLetterSize NOTIFY zoomChanged)
    Q_PROPERTY(qreal factor READ factor NOTIFY zoomChanged)
    Q_PROPERTY(QSize windowSize READ windowSize NOTIFY zoomChanged)
    Q_PROPERTY(bool canZoomIn READ canZoomIn NOTIFY zoomChanged)
    Q_PROPERTY(bool canZoomOut READ canZoomOut NOTIFY zoomChanged)
public:
    explicit ZoomController(QObject* parent = nullptr);

    InterfaceSettings* settings() const { return m_settings; }
    void setSettings(InterfaceSettings* settings);
    QWindow* window() const { return m_window; }
    void setWindow(QWindow* window);
    QSize baseSize() const { return m_baseSize; }
    void setBaseSize(QSize size);
    QRect availableArea() const;
    void setAvailableArea(QRect area);
    void resetAvailableArea();

    int shownLetterSize() const { return m_shown; }
    qreal factor() const;
    QSize windowSize() const;
    bool canZoomIn() const;
    bool canZoomOut() const;

    // One pixel larger or smaller than the size shown, and back to the base
    // size. They write the chosen size, so every window follows.
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void reset();

    // Places the window before it is shown (nothing while it is visible).
    Q_INVOKABLE void placeForShow();

signals:
    void settingsChanged();
    void windowChanged();
    void baseSizeChanged();
    void availableAreaChanged();
    void zoomChanged();

private:
    void recompute();
    void followScreen();
    // Centred over the transient parent, or over the available area without
    // one.
    QRect centred(QSize size) const;
    // Moved just enough to lie in the available area; aligned with its top
    // left corner when larger.
    QRect inArea(QRect rect) const;
    void applyGeometry(QRect rect);

    QPointer<InterfaceSettings> m_settings;
    QPointer<QWindow> m_window;
    QSize m_baseSize;
    QRect m_fixedArea;
    QRect m_screenArea;
    int m_shown;
    // The size shown when the window was last placed, 0 before the first time.
    int m_placedSize = 0;
    QMetaObject::Connection m_settingsConnection;
    QMetaObject::Connection m_screenConnection;
    QMetaObject::Connection m_areaConnection;
};

} // namespace gambasse
