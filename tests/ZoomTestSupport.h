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

// A child window cannot be resized: its minimum and maximum sizes are its
// size. A window manager enforces them, which the offscreen platform does not,
// so a limit left behind would only show on a real desktop, as a window larger
// than its contents.
inline bool isFixedAt(const QWindow* window, QSize size) {
    return window->size() == size && window->minimumSize() == size
           && window->maximumSize() == size;
}

inline QString describeLimits(const QWindow* window) {
    const auto text = [](QSize size) {
        return QStringLiteral("%1x%2").arg(size.width()).arg(size.height());
    };
    return QStringLiteral("size %1, minimum %2, maximum %3")
        .arg(text(window->size()), text(window->minimumSize()), text(window->maximumSize()));
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

// The item a popup draws: its background fills it.
inline QQuickItem* drawnItemOf(QObject* popup) {
    return popup->property("background").value<QQuickItem*>();
}

// What is wrong with a popup drawn against `anchor` (a combo or a date field)
// in a zoomed window: it must touch the anchor, below or above it, measure
// `baseWidth` scaled by `factor`, and lie whole inside the window.
inline QString popupProblems(QObject* popup, QQuickItem* anchor, qreal baseWidth,
                             qreal factor) {
    QQuickItem* drawn = drawnItemOf(popup);
    if (drawn == nullptr)
        return QStringLiteral("no background");
    const QRectF rect = sceneRect(drawn);
    const QRectF field = sceneRect(anchor);
    const QSizeF window = anchor->window()->size();
    QStringList problems;
    const auto near = [](qreal a, qreal b) { return qAbs(a - b) < 0.6; };
    if (!near(rect.top(), field.bottom()) && !near(rect.bottom(), field.top()))
        problems << QStringLiteral("not against its field: %1 for %2")
                        .arg(describe(rect), describe(field));
    if (!near(rect.width(), baseWidth * factor))
        problems << QStringLiteral("width %1 instead of %2")
                        .arg(rect.width())
                        .arg(baseWidth * factor);
    if (rect.left() < -0.5 || rect.top() < -0.5 || rect.right() > window.width() + 0.5
        || rect.bottom() > window.height() + 0.5)
        problems << QStringLiteral("outside the window: %1").arg(describe(rect));
    return problems.join(QStringLiteral("; "));
}

// Whether a popup opened under its field, rather than above it.
inline bool isBelow(QObject* popup, QQuickItem* anchor) {
    return sceneRect(drawnItemOf(popup)).top() >= sceneRect(anchor).bottom() - 0.6;
}

} // namespace gambasse::zoomtest
