#include <logic/ZoomLevels.h>

#include <algorithm>

namespace gambasse::zoom {

int clampLetterSize(int letterSize) {
    return std::clamp(letterSize, baseLetterSize, maximumLetterSize);
}

double factor(int letterSize) {
    return static_cast<double>(clampLetterSize(letterSize)) / baseLetterSize;
}

QSize windowSize(QSize base, int letterSize) {
    const int size = clampLetterSize(letterSize);
    // Integer arithmetic: a floating point product such as 1020 * 15 / 13
    // would risk rounding a whole result one pixel up.
    const auto scaled = [size](int length) {
        return (length * size + baseLetterSize - 1) / baseLetterSize;
    };
    return {scaled(base.width()), scaled(base.height())};
}

int fittingLetterSize(QSize base, int wanted, QSize available) {
    for (int size = clampLetterSize(wanted); size > baseLetterSize; --size) {
        const QSize window = windowSize(base, size);
        if (window.width() <= available.width() && window.height() <= available.height())
            return size;
    }
    return baseLetterSize;
}

} // namespace gambasse::zoom
