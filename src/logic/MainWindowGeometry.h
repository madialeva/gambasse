#pragma once

#include <QRect>
#include <QSize>

namespace gambasse {

// Persisted state and preferred normal size of the main patient window.
struct MainWindowGeometry {
    bool maximized = true;
    QSize normalSize{1008, 561};

    bool operator==(const MainWindowGeometry&) const = default;
};

// Pure geometry rules of the main patient window: base and scaled minimum
// sizes, elastic normal size fitting the screen available area, and
// positioning within that area.
namespace mainwindow {

inline constexpr QSize baseSize() {
    return {1008, 561};
}

// Ensures a normal window size is never smaller than the 1008x561 base size.
QSize clampNormalSize(QSize size);

// Minimum window size at the shown letter size (base size scaled by L / 13,
// rounded up to whole pixels).
QSize minimumWindowSize(int shownLetterSize);

// Normal window size to show: at least the user's preferred size and the
// scaled minimum, reduced only when exceeding `available` (never below the
// scaled minimum).
QSize effectiveNormalSize(QSize preferred, int shownLetterSize, QSize available);

// Rectangle of `size` centred in `available` (aligned to its top-left corner
// when larger than `available`).
QRect centredRect(QSize size, QRect available);

// Moves `rect` just enough to lie whole inside `available` (aligned to its
// top-left corner when larger).
QRect clampToArea(QRect rect, QRect available);

} // namespace mainwindow

} // namespace gambasse
