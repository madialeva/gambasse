#pragma once

// Helpers shared by the QML screen tests for the zoom of the history and
// consultation windows (FormWindow).

#include <QQuickItem>
#include <QQuickWindow>
#include <QRect>
#include <QtTest>

#include <ui/InterfaceSettings.h>
#include <ui/controller/ZoomController.h>

namespace gambasse::zoomtest {

// Available areas the tests give the windows: the offscreen screen (800x800)
// is smaller than any of them.
inline const QRect wideArea(0, 0, 1920, 1040);
inline const QRect laptopArea(0, 0, 1366, 728);

inline ZoomController* zoomOf(QObject* window) {
    return window != nullptr
               ? window->findChild<ZoomController*>(QStringLiteral("zoomController"))
               : nullptr;
}

// The item every control of the window lives in, scaled by the zoom.
inline QQuickItem* stageOf(QObject* window) {
    return window != nullptr ? window->findChild<QQuickItem*>(QStringLiteral("stage")) : nullptr;
}

// Where an item is drawn, in window coordinates.
inline QRectF sceneRect(const QQuickItem* item) {
    return item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
}

// `scaled` is `base` scaled by `factor` from the window's top left corner.
inline bool sameScaled(const QRectF& base, const QRectF& scaled, qreal factor) {
    const auto near = [](qreal a, qreal b) { return qAbs(a - b) < 0.01; };
    return near(scaled.x(), base.x() * factor) && near(scaled.y(), base.y() * factor)
           && near(scaled.width(), base.width() * factor)
           && near(scaled.height(), base.height() * factor);
}

inline QString describe(const QRectF& rect) {
    return QStringLiteral("(%1, %2, %3x%4)")
        .arg(rect.x())
        .arg(rect.y())
        .arg(rect.width())
        .arg(rect.height());
}

// The centre of `child` over the centre of `main`, to the pixel.
inline bool centredOver(const QWindow* child, const QWindow* main) {
    const QPoint delta = child->geometry().center() - main->geometry().center();
    return qAbs(delta.x()) <= 1 && qAbs(delta.y()) <= 1;
}

// Moves a window as a drag of its title bar would, and waits until Qt has
// taken the new position as the window's own (the platform reports it later,
// and a window closed before that would forget it).
inline bool dragTo(QWindow* window, QPoint position) {
    window->setPosition(position);
    if (!QTest::qWaitFor([window, position] { return window->position() == position; }))
        return false;
    QTest::qWait(50);
    return window->position() == position;
}

// Gives every child window of the shell `area` as its available area. Done
// before a window opens, so its first placement already uses it.
inline void setArea(const QList<QObject*>& windows, const QRect& area) {
    for (QObject* window : windows) {
        if (ZoomController* zoom = zoomOf(window))
            zoom->setAvailableArea(area);
    }
}

} // namespace gambasse::zoomtest
