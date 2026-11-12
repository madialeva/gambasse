#pragma once

// Helper shared by the QML screen tests: which texts of a window cannot be read
// whole in the current language.

#include <QFont>
#include <QGuiApplication>
#include <QQuickItem>
#include <QRectF>
#include <QStringList>
#include <QtTest>
#include <functional>

namespace gambasse {

namespace visibletext {

struct Piece {
    QString text;
    QRectF rect;
    QQuickItem* item;
};

inline bool isAncestor(const QQuickItem* ancestor, const QQuickItem* item) {
    for (const QQuickItem* up = item; up != nullptr; up = up->parentItem()) {
        if (up == ancestor)
            return true;
    }
    return false;
}

inline bool isTextInput(const QQuickItem* item) {
    return item->inherits("QQuickTextInput");
}

inline bool isButton(const QQuickItem* item) {
    return item->inherits("QQuickAbstractButton");
}

inline bool isControl(const QQuickItem* item) {
    return isTextInput(item) || isButton(item) || item->inherits("QQuickComboBox");
}

// Where a control is drawn, in the coordinates of `top`: the rows of check
// boxes are closer than their height, but their paddings are empty. Measured in
// `top`, not in the window, so a zoomed window (whose stage is scaled) is
// checked with the same measures as at its base size.
inline QRectF drawnRect(const QQuickItem* control, const QQuickItem* top) {
    const qreal padTop = isButton(control) ? control->property("topPadding").toReal() : 0;
    const qreal padBottom = isButton(control) ? control->property("bottomPadding").toReal() : 0;
    return control->mapRectToItem(
        top, QRectF(0, padTop, control->width(), control->height() - padTop - padBottom));
}

// Where a text is drawn, in the coordinates of `top`: its natural width from
// where it starts, around its vertical centre. Its own item may be narrower.
inline QRectF drawnRect(QQuickItem* item, qreal wanted, const QQuickItem* top) {
    const qreal padding = item->property("leftPadding").toReal();
    const qreal height = item->property("contentHeight").toReal();
    return item->mapRectToItem(
        top, QRectF(padding, (item->height() - height) / 2.0, wanted - padding, height));
}

} // namespace visibletext

// The texts under `top` that cannot be read whole: squeezed below their width,
// running past a group that clips them, or lying under another control. For a
// zoomed history or consultation window, `top` is its stage.
inline QStringList cutTexts(QQuickItem* top) {
    using namespace visibletext;
    QStringList cut;
    QList<Piece> texts;
    QList<QQuickItem*> controls;
    const auto report = [&cut](const QString& text) {
        if (!cut.contains(text))
            cut.append(text);
    };
    const std::function<void(QQuickItem*)> walk = [&](QQuickItem* item) {
        // The grid headers and filters scroll with the grid: what their row
        // leaves out is reached by scrolling.
        if (!item->isVisible() || item->objectName() == QLatin1String("headerCell")
            || item->objectName() == QLatin1String("filterBox"))
            return;
        if (isControl(item))
            controls.append(item);
        const QString text = item->property("text").toString();
        // Measured with another font, the rest of the checks would prove nothing.
        const QVariant font = item->property("font");
        if (!text.isEmpty() && font.metaType() == QMetaType::fromType<QFont>()
            && font.value<QFont>().family() != QGuiApplication::font().family())
            report(QStringLiteral("%1 (in %2)").arg(text, font.value<QFont>().family()));
        if (!text.isEmpty() && isTextInput(item)) {
            const qreal room = item->width() - item->property("leftPadding").toReal()
                               - item->property("rightPadding").toReal();
            if (item->property("contentWidth").toReal() > room + 0.5)
                report(text);
        } else if (!text.isEmpty() && item->inherits("QQuickText")) {
            // A wrapped text needs height, any other its natural width.
            const bool wraps = item->property("wrapMode").toInt() != 0;
            const qreal wanted =
                wraps ? item->width() : item->property("implicitWidth").toReal();
            bool whole = wraps ? item->property("contentHeight").toReal() <= item->height() + 0.5
                               : wanted <= item->width() + 0.5;
            // What a scrolling view leaves out is reached by scrolling.
            for (QQuickItem* up = item->parentItem();
                 whole && up != top->parentItem() && !up->inherits("QQuickFlickable");
                 up = up->parentItem()) {
                if (up->clip())
                    whole = item->mapToItem(up, QPointF(wanted, 0)).x() <= up->width() + 0.5;
            }
            if (!whole)
                report(text);
            // A title sits on the border of its group on purpose (the badge
            // test of each form looks after it).
            if (item->objectName() != QLatin1String("groupTitle"))
                texts.append({text, drawnRect(item, wanted, top), item});
        }
        for (QQuickItem* kid : item->childItems())
            walk(kid);
    };
    walk(top);

    const auto overlap = [](const QRectF& a, const QRectF& b) {
        const QRectF both = a.intersected(b);
        return both.width() > 1.0 && both.height() > 1.0;
    };
    for (const Piece& piece : texts) {
        for (QQuickItem* control : controls) {
            if (isAncestor(control, piece.item))
                continue;
            if (overlap(piece.rect, drawnRect(control, top)))
                report(piece.text);
        }
        for (const Piece& other : texts) {
            if (other.item != piece.item && overlap(piece.rect, other.rect))
                report(piece.text);
        }
    }
    return cut;
}

// cutTexts() on every page of every tab view under `top`, each language being
// the caller's: pages that are not current are not visible.
inline QStringList cutTextsOnEveryTab(QQuickItem* top) {
    QList<QQuickItem*> tabViews;
    const std::function<void(QQuickItem*)> find = [&](QQuickItem* item) {
        if (item->property("titles").isValid() && item->property("currentIndex").isValid())
            tabViews.append(item);
        for (QQuickItem* kid : item->childItems())
            find(kid);
    };
    find(top);

    QStringList cut = cutTexts(top);
    const auto add = [&cut](const QStringList& more) {
        for (const QString& text : more) {
            if (!cut.contains(text))
                cut.append(text);
        }
    };
    for (QQuickItem* view : tabViews) {
        const int current = view->property("currentIndex").toInt();
        const int count = view->property("titles").toStringList().size();
        for (int i = 0; i < count; ++i) {
            view->setProperty("currentIndex", i);
            // Layouts inside a page settle once it shows.
            QTest::qWait(30);
            add(cutTexts(top));
        }
        view->setProperty("currentIndex", current);
        QTest::qWait(30);
    }
    return cut;
}

} // namespace gambasse
