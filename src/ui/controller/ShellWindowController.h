#pragma once

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QTimer>
#include <QWindow>
#include <QtQml/qqmlregistration.h>

#include <logic/MainWindowGeometry.h>
#include <ui/InterfaceSettings.h>

namespace gambasse {

// Coordinates the main shell window (Root.qml): shared letter-size zoom over
// its elastic layout, startup maximized by default, and persisted window state
// and preferred normal size in config.ini (ventana/*).
class ShellWindowController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(gambasse::InterfaceSettings* settings READ settings WRITE setSettings NOTIFY
                   settingsChanged)
    Q_PROPERTY(QWindow* window READ window WRITE setWindow NOTIFY windowChanged)
    Q_PROPERTY(QSize baseSize READ baseSize WRITE setBaseSize NOTIFY baseSizeChanged)
    // Available area of the window's screen; tests override it because the
    // offscreen screen (800x800) is smaller than the base window.
    Q_PROPERTY(QRect availableArea READ availableArea WRITE setAvailableArea RESET
                   resetAvailableArea NOTIFY availableAreaChanged)
    Q_PROPERTY(int shownLetterSize READ shownLetterSize NOTIFY zoomChanged)
    Q_PROPERTY(qreal factor READ factor NOTIFY zoomChanged)
    Q_PROPERTY(QSize minimumSize READ minimumSize NOTIFY zoomChanged)
    Q_PROPERTY(bool canZoomIn READ canZoomIn NOTIFY zoomChanged)
    Q_PROPERTY(bool canZoomOut READ canZoomOut NOTIFY zoomChanged)
    Q_PROPERTY(bool maximized READ maximized NOTIFY windowStateChanged)
public:
    explicit ShellWindowController(QObject* parent = nullptr);
    ~ShellWindowController() override;

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
    QSize minimumSize() const { return m_minimumSize; }
    bool canZoomIn() const { return m_canZoomIn; }
    bool canZoomOut() const;
    bool maximized() const { return m_maximized; }

    // One pixel larger or smaller than the size shown, and back to the base
    // size. Writes the shared InterfaceSettings.letterSize.
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void reset();

    // Loads persisted geometry, positions the normal rect centred in the
    // available area and shows the window maximized or normal.
    Q_INVOKABLE void applyInitialGeometry();

    // Writes any debounced normal-size resize to config.ini immediately.
    Q_INVOKABLE void flushPendingSave();

    // Minimizes the window to the taskbar while preserving Qt::WindowMaximized
    // in its window states, so un-minimizing returns to the prior state.
    Q_INVOKABLE void minimize();

signals:
    void settingsChanged();
    void windowChanged();
    void baseSizeChanged();
    void availableAreaChanged();
    void zoomChanged();
    void windowStateChanged();

private:
    void recompute(bool areaChanged = false);
    void followScreen();
    QScreen* hostScreen() const;
    void onVisibilityChanged(QWindow::Visibility visibility);
    void onWindowResized();
    void persistGeometry();

    QPointer<InterfaceSettings> m_settings;
    QPointer<QWindow> m_window;
    QSize m_baseSize = mainwindow::baseSize();
    QRect m_fixedArea;
    QRect m_screenArea;
    int m_shown;
    QSize m_minimumSize;
    bool m_canZoomIn = false;

    bool m_maximized = true;
    bool m_minimized = false;
    bool m_restoringMaximized = false;
    QSize m_preferredNormalSize = mainwindow::baseSize();
    bool m_initialApplied = false;
    bool m_hadNormalPlacement = false;
    bool m_applyingGeometry = false;
    QSize m_lastAppliedSize;
    QTimer m_saveTimer;

    QMetaObject::Connection m_settingsConnection;
    QMetaObject::Connection m_screenConnection;
    QMetaObject::Connection m_visibilityConnection;
    QMetaObject::Connection m_widthConnection;
    QMetaObject::Connection m_heightConnection;
    QMetaObject::Connection m_areaConnection;
};

} // namespace gambasse
