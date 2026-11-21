#include <logic/MainWindowGeometry.h>

#include <algorithm>

#include <logic/ZoomLevels.h>

namespace gambasse::mainwindow {

QSize clampNormalSize(QSize size) {
    return {std::max(size.width(), baseSize().width()),
            std::max(size.height(), baseSize().height())};
}

QSize minimumWindowSize(int shownLetterSize) {
    return zoom::windowSize(baseSize(), shownLetterSize);
}

QSize effectiveNormalSize(QSize preferred, int shownLetterSize, QSize available) {
    const QSize minimum = minimumWindowSize(shownLetterSize);
    const QSize clamped = clampNormalSize(preferred);
    int width = std::max(clamped.width(), minimum.width());
    int height = std::max(clamped.height(), minimum.height());
    if (available.isValid()) {
        if (width > available.width())
            width = std::max(minimum.width(), available.width());
        if (height > available.height())
            height = std::max(minimum.height(), available.height());
    }
    return {width, height};
}

QRect centredRect(QSize size, QRect available) {
    QRect rect(QPoint(0, 0), size);
    if (available.isValid()) {
        rect.moveCenter(available.center());
        rect = clampToArea(rect, available);
    }
    return rect;
}

QRect clampToArea(QRect rect, QRect available) {
    if (!available.isValid())
        return rect;
    const auto fit = [](int start, int length, int areaStart, int areaLength) {
        if (length >= areaLength)
            return areaStart;
        return std::clamp(start, areaStart, areaStart + areaLength - length);
    };
    rect.moveTo(fit(rect.x(), rect.width(), available.x(), available.width()),
                fit(rect.y(), rect.height(), available.y(), available.height()));
    return rect;
}

} // namespace gambasse::mainwindow
