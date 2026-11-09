#include <logic/TextOrder.h>

#include <QChar>
#include <QList>
#include <QString>

namespace gambasse {

namespace {

// A text ready to compare: decomposed (NFD), so that an accent is a mark after
// its letter, and case folded.
QList<uint> prepared(QStringView text) {
    return text.toString().normalized(QString::NormalizationForm_D).toCaseFolded().toUcs4();
}

enum Kind : quint64 { Other = 0, Digit = 1, Letter = 2 };

// The weight of the next base character of `text` from `at`, accents skipped,
// or false at its end. The kind decides first (spaces and punctuation, then
// digits, then letters), then the code point; in Spanish, ñ comes right after
// n.
bool nextWeight(const QList<uint>& text, qsizetype& at, bool spanish, quint64& weight) {
    while (at < text.size()) {
        const char32_t character = text.at(at++);
        if (QChar::isMark(character))
            continue;
        const quint64 kind = QChar::isLetter(character) ? Letter
                             : QChar::isDigit(character) ? Digit
                                                         : Other;
        weight = (kind << 32) | (quint64(character) << 1);
        if (spanish && character == U'n' && at < text.size() && text.at(at) == 0x0303) {
            weight |= 1;
            ++at;
        }
        return true;
    }
    return false;
}

} // namespace

int compareText(QStringView left, QStringView right, const QLocale& locale) {
    const bool spanish = locale.language() == QLocale::Spanish;
    const QList<uint> a = prepared(left);
    const QList<uint> b = prepared(right);

    // First level: the letters, without case or accents.
    qsizetype i = 0;
    qsizetype j = 0;
    for (;;) {
        quint64 x = 0;
        quint64 y = 0;
        const bool moreLeft = nextWeight(a, i, spanish, x);
        const bool moreRight = nextWeight(b, j, spanish, y);
        if (!moreLeft || !moreRight) {
            if (moreLeft != moreRight)
                return moreLeft ? 1 : -1;
            break;
        }
        if (x != y)
            return x < y ? -1 : 1;
    }

    // Second level: the accents. A mark sorts after any letter, so the text
    // without it goes first.
    for (qsizetype k = 0; k < qMin(a.size(), b.size()); ++k) {
        if (a.at(k) != b.at(k))
            return a.at(k) < b.at(k) ? -1 : 1;
    }
    if (a.size() == b.size())
        return 0;
    return a.size() < b.size() ? -1 : 1;
}

} // namespace gambasse
