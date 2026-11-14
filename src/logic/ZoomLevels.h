#pragma once

#include <QSize>

namespace gambasse {

// Zoom of the history and consultation windows, expressed as the interface
// letter size in pixels. 13 px is the base size, the one their fixed
// coordinates were laid out with, and also the smallest; 26 px, twice as large,
// is the largest. A window at letter size L is its base layout scaled by
// L / 13, so the controls keep their relative positions.
namespace zoom {

inline constexpr int baseLetterSize = 13;
inline constexpr int maximumLetterSize = 26;

// The letter size brought into [baseLetterSize, maximumLetterSize].
int clampLetterSize(int letterSize);

// How much a window at that letter size is scaled from its base layout.
double factor(int letterSize);

// Size of a window whose base size is `base`, at that letter size: the base
// scaled by factor(), rounded up to whole pixels so the scaled layout always
// fits inside.
QSize windowSize(QSize base, int letterSize);

// The largest letter size, up to `wanted`, at which a window of base size
// `base` fits whole inside `available`. Never below baseLetterSize: a window
// that does not fit even at its base size keeps it.
int fittingLetterSize(QSize base, int wanted, QSize available);

} // namespace zoom

} // namespace gambasse
