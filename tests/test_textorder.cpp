#include <QCollator>
#include <QCoreApplication>
#include <QLocale>
#include <QScopeGuard>
#include <QtTest>
#include <algorithm>
#include <clocale>
#include <string>

#include <logic/InterfaceLocale.h>
#include <logic/TextOrder.h>

namespace gambasse {
namespace {

QStringList words(const char* text) {
    return QString::fromUtf8(text).split(QLatin1Char('|'));
}

// Stable, like the grid's proxy: texts that compare equal keep their order.
QStringList sortedIn(QStringList texts, const QLocale& locale) {
    std::stable_sort(texts.begin(), texts.end(), [&locale](const QString& a, const QString& b) {
        return compareText(a, b, locale) < 0;
    });
    return texts;
}

// Names with what the order has to get right: accents, case, the Ñ, spaces,
// apostrophes and digits, in no particular order.
const char* const kSample = "ZACARIAS|ÁLVARO|ALVES|NUNO|NÚÑEZ|NUNEZ|NUÑO|NUZ|ÑANDÚ|OLIVEIRA|NZINGA|"
                            "CÉSAR|CESAR|ÇA|CABRAL|CUNHA|DA COSTA|DACOSTA|D'ALMEIDA|DIALLO|ÉDER|"
                            "EDUARDO|JOÃO|JOAO|JOANA|maria|MARIA|MÁRIA|MARÍA JOSÉ|MARIA JOSE|"
                            "MARIANA|ANA 2|ANA 10|ANA|ANABELA|SÁ|SA|SABINO|Ú|U|V";

const char* const kSampleSpanish =
    "ÁLVARO|ALVES|ANA|ANA 10|ANA 2|ANABELA|ÇA|CABRAL|CESAR|CÉSAR|CUNHA|D'ALMEIDA|DA COSTA|"
    "DACOSTA|DIALLO|ÉDER|EDUARDO|JOANA|JOAO|JOÃO|maria|MARIA|MÁRIA|MARIA JOSE|MARÍA JOSÉ|"
    "MARIANA|NUNEZ|NUNO|NÚÑEZ|NUÑO|NUZ|NZINGA|ÑANDÚ|OLIVEIRA|SA|SÁ|SABINO|U|Ú|V|ZACARIAS";

const char* const kSampleOthers =
    "ÁLVARO|ALVES|ANA|ANA 10|ANA 2|ANABELA|ÇA|CABRAL|CESAR|CÉSAR|CUNHA|D'ALMEIDA|DA COSTA|"
    "DACOSTA|DIALLO|ÉDER|EDUARDO|JOANA|JOAO|JOÃO|maria|MARIA|MÁRIA|MARIA JOSE|MARÍA JOSÉ|"
    "MARIANA|ÑANDÚ|NUNEZ|NÚÑEZ|NUNO|NUÑO|NUZ|NZINGA|OLIVEIRA|SA|SÁ|SABINO|U|Ú|V|ZACARIAS";

} // namespace

// The alphabetical order of the patient list: by the interface language, and
// the same whatever the system's language or the Qt build.
class TestTextOrder : public QObject {
    Q_OBJECT

private slots:
    void interfaceLocales();
    void enyeIsALetterInSpanish();
    void enyeIsAnNInPortugueseAndEnglish();
    void accentsAndCase();
    void spacesPunctuationAndDigits();
    void sampleInEveryLanguage_data();
    void sampleInEveryLanguage();
    void sameOrderAsIcu_data();
    void sameOrderAsIcu();
};

void TestTextOrder::interfaceLocales() {
    QCOMPARE(interfaceLocale(QStringLiteral("pt")).name(), QStringLiteral("pt_GW"));
    QCOMPARE(interfaceLocale(QStringLiteral("es")).name(), QStringLiteral("es_ES"));
    QCOMPARE(interfaceLocale(QStringLiteral("en")).name(), QStringLiteral("en_GB"));
    QCOMPARE(interfaceLocale(QStringLiteral("fr")).name(), QStringLiteral("en_GB"));
}

void TestTextOrder::enyeIsALetterInSpanish() {
    QCOMPARE(sortedIn(words("OLIVEIRA|ÑANDÚ|NZINGA|NÚÑEZ|NUNO"), interfaceLocale(QStringLiteral("es"))),
             words("NUNO|NÚÑEZ|NZINGA|ÑANDÚ|OLIVEIRA"));
}

void TestTextOrder::enyeIsAnNInPortugueseAndEnglish() {
    for (const char* language : {"pt", "en"})
        QCOMPARE(sortedIn(words("OLIVEIRA|ÑANDÚ|NZINGA|NÚÑEZ|NUNO"),
                          interfaceLocale(QString::fromLatin1(language))),
                 words("ÑANDÚ|NÚÑEZ|NUNO|NZINGA|OLIVEIRA"));
}

void TestTextOrder::accentsAndCase() {
    for (const char* language : {"pt", "es", "en"}) {
        const QLocale locale = interfaceLocale(QString::fromLatin1(language));
        QCOMPARE(sortedIn(words("CUNHA|maria|CÉSAR|ALVES|CESAR|ÁLVARO"), locale),
                 words("ÁLVARO|ALVES|CESAR|CÉSAR|CUNHA|maria"));
        // Case alone never decides; an accent decides only between equal letters.
        QCOMPARE(compareText(u"maria", u"MARIA", locale), 0);
        QVERIFY(compareText(u"CESAR", u"césar", locale) < 0);
        QVERIFY(compareText(u"CÉSAR", u"cunha", locale) < 0);
        QVERIFY(compareText(u"", u"A", locale) < 0);
        QCOMPARE(compareText(u"", u"", locale), 0);
    }
}

void TestTextOrder::spacesPunctuationAndDigits() {
    for (const char* language : {"pt", "es", "en"})
        QCOMPARE(sortedIn(words("DACOSTA|DA COSTA|D'ALMEIDA|ANABELA|ANA 2|ANA 10|ANA"),
                          interfaceLocale(QString::fromLatin1(language))),
                 words("ANA|ANA 10|ANA 2|ANABELA|D'ALMEIDA|DA COSTA|DACOSTA"));
}

// The same order whatever the process locale: the system's language, or none.
void TestTextOrder::sampleInEveryLanguage_data() {
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("system");
    QTest::addColumn<QStringList>("expected");
    for (const char* system : {"", "C"}) {
        const QString name = QString::fromLatin1(*system ? system : "default");
        QTest::addRow("es on %s", qPrintable(name))
            << QStringLiteral("es") << QString::fromLatin1(system) << words(kSampleSpanish);
        QTest::addRow("pt on %s", qPrintable(name))
            << QStringLiteral("pt") << QString::fromLatin1(system) << words(kSampleOthers);
        QTest::addRow("en on %s", qPrintable(name))
            << QStringLiteral("en") << QString::fromLatin1(system) << words(kSampleOthers);
    }
}

void TestTextOrder::sampleInEveryLanguage() {
    QFETCH(QString, language);
    QFETCH(QString, system);
    QFETCH(QStringList, expected);
    const std::string before = std::setlocale(LC_ALL, nullptr);
    const QLocale defaultBefore;
    const auto restore = qScopeGuard([&before, defaultBefore] {
        std::setlocale(LC_ALL, before.c_str());
        QLocale::setDefault(defaultBefore);
    });
    if (!system.isEmpty()) {
        QVERIFY(std::setlocale(LC_ALL, qPrintable(system)) != nullptr);
        QLocale::setDefault(QLocale(system));
    }
    QCOMPARE(sortedIn(words(kSample), interfaceLocale(language)), expected);
}

// Where Qt has ICU (the desktop kit, as in the CI), its collation without case
// is the reference.
void TestTextOrder::sameOrderAsIcu_data() {
    QTest::addColumn<QString>("language");
    for (const char* language : {"pt", "es", "en"})
        QTest::addRow("%s", language) << QString::fromLatin1(language);
}

void TestTextOrder::sameOrderAsIcu() {
#ifdef GAMBASSE_QT_HAS_ICU
    QFETCH(QString, language);
    const QLocale locale = interfaceLocale(language);
    QCollator collator(locale);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    QStringList byIcu = words(kSample);
    std::stable_sort(byIcu.begin(), byIcu.end(), [&collator](const QString& a, const QString& b) {
        return collator.compare(a, b) < 0;
    });
    QCOMPARE(sortedIn(words(kSample), locale), byIcu);
#else
    QSKIP("This Qt is built without ICU");
#endif
}

} // namespace gambasse

QTEST_GUILESS_MAIN(gambasse::TestTextOrder)

#include "test_textorder.moc"
