#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QTranslator>
#include <QXmlStreamReader>
#include <QtTest>

namespace gambasse {
namespace {

struct CatalogEntry {
    QString context;
    QString source;
    QString translation;
    QString state;
};

// The entries of translations/gambasse_<language>.ts, without the ones lupdate
// keeps for texts that are no longer in the sources.
QList<CatalogEntry> readCatalog(const QString& language) {
    QList<CatalogEntry> entries;
    QFile file(QStringLiteral(TRANSLATIONS_DIR "/gambasse_%1.ts").arg(language));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return entries;
    QXmlStreamReader xml(&file);
    CatalogEntry entry;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement())
            continue;
        if (xml.name() == u"name") {
            entry.context = xml.readElementText();
        } else if (xml.name() == u"source") {
            entry.source = xml.readElementText();
        } else if (xml.name() == u"translation") {
            entry.state = xml.attributes().value(u"type").toString();
            entry.translation = xml.readElementText();
            if (entry.state != u"vanished" && entry.state != u"obsolete")
                entries.append(entry);
        }
    }
    return xml.hasError() ? QList<CatalogEntry>{} : entries;
}

QString describe(const CatalogEntry& entry) {
    return entry.context + QStringLiteral(" | ") + entry.source;
}

// The %1, %2... of a text.
QSet<QString> placeholders(const QString& text) {
    static const QRegularExpression pattern(QStringLiteral("%\\d+"));
    QSet<QString> found;
    for (const QRegularExpressionMatch& match : pattern.globalMatch(text))
        found.insert(match.captured());
    return found;
}

} // namespace

// Keeps the Spanish and Portuguese catalogs complete: a text left untranslated,
// a lost placeholder or a .qm not regenerated after editing its .ts fail here,
// naming the language and the text, instead of showing up in the interface.
class TestTranslationCatalogs : public QObject {
    Q_OBJECT

private slots:
    void everyEntryIsFinished_data();
    void everyEntryIsFinished();
    void placeholdersAreKept_data();
    void placeholdersAreKept();
    void compiledCatalogMatchesItsSource_data();
    void compiledCatalogMatchesItsSource();

private:
    void addLanguages();
};

void TestTranslationCatalogs::addLanguages() {
    QTest::addColumn<QString>("language");
    QTest::newRow("es") << QStringLiteral("es");
    QTest::newRow("pt") << QStringLiteral("pt");
}

void TestTranslationCatalogs::everyEntryIsFinished_data() {
    addLanguages();
}

void TestTranslationCatalogs::everyEntryIsFinished() {
    QFETCH(QString, language);
    const QList<CatalogEntry> entries = readCatalog(language);
    QVERIFY(!entries.isEmpty());
    QStringList unfinished;
    for (const CatalogEntry& entry : entries) {
        if (entry.state == u"unfinished" || entry.translation.trimmed().isEmpty())
            unfinished.append(describe(entry));
    }
    QVERIFY2(unfinished.isEmpty(), qPrintable(unfinished.join(u'\n')));
}

void TestTranslationCatalogs::placeholdersAreKept_data() {
    addLanguages();
}

void TestTranslationCatalogs::placeholdersAreKept() {
    QFETCH(QString, language);
    const QList<CatalogEntry> entries = readCatalog(language);
    QVERIFY(!entries.isEmpty());
    QStringList different;
    for (const CatalogEntry& entry : entries) {
        if (placeholders(entry.source) != placeholders(entry.translation))
            different.append(describe(entry));
    }
    QVERIFY2(different.isEmpty(), qPrintable(different.join(u'\n')));
}

void TestTranslationCatalogs::compiledCatalogMatchesItsSource_data() {
    addLanguages();
}

void TestTranslationCatalogs::compiledCatalogMatchesItsSource() {
    QFETCH(QString, language);
    QTranslator compiled;
    QVERIFY(compiled.load(QStringLiteral("gambasse_") + language, QStringLiteral(":/i18n")));
    const QList<CatalogEntry> entries = readCatalog(language);
    QVERIFY(!entries.isEmpty());
    QStringList different;
    for (const CatalogEntry& entry : entries) {
        const QString shown = compiled.translate(entry.context.toUtf8().constData(),
                                                 entry.source.toUtf8().constData());
        if (shown != entry.translation)
            different.append(describe(entry));
    }
    QVERIFY2(different.isEmpty(), qPrintable(different.join(u'\n')));
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestTranslationCatalogs test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_translationcatalogs.moc"
