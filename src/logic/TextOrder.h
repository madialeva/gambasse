#pragma once

#include <QLocale>
#include <QStringView>

namespace gambasse {

// Alphabetical order of two texts in a language, the same on every system and
// in every build: QCollator follows the system's collation and, in the static
// Qt kits, built without ICU, ignores the locale it is given. Only Qt's own
// Unicode tables are used.
//
// Case never decides. An accented letter sorts with its base letter, and the
// text without the accent goes first (CESAR CÉSAR CUNHA). Spaces and
// punctuation sort before digits, and digits before letters. In Spanish the Ñ
// is a letter of its own, after N; in other languages it sorts as an N.
// Returns a negative number, zero or a positive number, like QString::compare().
int compareText(QStringView left, QStringView right, const QLocale& locale);

} // namespace gambasse
