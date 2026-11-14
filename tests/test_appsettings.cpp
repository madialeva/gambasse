#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <logic/AppSettings.h>

namespace gambasse {

// Verifies the language, theme and letter size persistence against a
// temporary config.ini.
class TestAppSettings : public QObject {
    Q_OBJECT

private slots:
    void defaults();
    void roundTrip();
    void letterSizeDefault();
    void letterSizeRoundTrip();
    void letterSizeInvalid_data();
    void letterSizeInvalid();
    void letterSizeKeepsTheOtherKeys();
};

namespace {
// A config.ini with exactly `content` in a new temporary base path.
bool writeConfig(const QTemporaryDir& dir, const QByteArray& content) {
    setBasePath(dir.path());
    QFile file(QDir(dir.path()).filePath(QStringLiteral("config.ini")));
    return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
}
} // namespace

void TestAppSettings::defaults() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    setBasePath(dir.path());

    const AppSettings settings;
    QCOMPARE(settings.language(), QStringLiteral("pt"));
    QCOMPARE(settings.theme(), QStringLiteral("claro"));
}

void TestAppSettings::roundTrip() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    setBasePath(dir.path());

    AppSettings().setLanguage(QStringLiteral("es"));
    AppSettings().setTheme(QStringLiteral("oscuro"));

    const AppSettings settings;
    QCOMPARE(settings.language(), QStringLiteral("es"));
    QCOMPARE(settings.theme(), QStringLiteral("oscuro"));
}

void TestAppSettings::letterSizeDefault() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    setBasePath(dir.path());
    QCOMPARE(AppSettings().letterSize(), 13);
}

void TestAppSettings::letterSizeRoundTrip() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    setBasePath(dir.path());

    AppSettings().setLetterSize(15);
    QCOMPARE(AppSettings().letterSize(), 15);
    const QSettings raw(QDir(dir.path()).filePath(QStringLiteral("config.ini")),
                        QSettings::IniFormat);
    QCOMPARE(raw.value(QStringLiteral("letra/pixeles")).toString(), QStringLiteral("15"));
}

void TestAppSettings::letterSizeInvalid_data() {
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<int>("read");
    QTest::newRow("not a number") << QByteArray("grande") << 13;
    QTest::newRow("empty") << QByteArray("") << 13;
    QTest::newRow("decimal") << QByteArray("15.5") << 13;
    QTest::newRow("above the maximum") << QByteArray("40") << 26;
    QTest::newRow("below the base") << QByteArray("5") << 13;
    QTest::newRow("in range") << QByteArray("20") << 20;
}

void TestAppSettings::letterSizeInvalid() {
    QFETCH(QByteArray, value);
    QFETCH(int, read);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(writeConfig(dir, "[letra]\npixeles=" + value + "\n"));
    QCOMPARE(AppSettings().letterSize(), read);
}

void TestAppSettings::letterSizeKeepsTheOtherKeys() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(writeConfig(dir, "[database]\npath=database.db\nuser=gambasse\npassword=gambasse\n\n"
                             "[idioma]\ncodigo=es\n\n[tema]\nmodo=oscuro\n"));

    AppSettings().setLetterSize(17);

    const AppSettings settings;
    QCOMPARE(settings.letterSize(), 17);
    QCOMPARE(settings.language(), QStringLiteral("es"));
    QCOMPARE(settings.theme(), QStringLiteral("oscuro"));
    const QSettings raw(QDir(dir.path()).filePath(QStringLiteral("config.ini")),
                        QSettings::IniFormat);
    QCOMPARE(raw.value(QStringLiteral("database/path")).toString(), QStringLiteral("database.db"));
    QCOMPARE(raw.value(QStringLiteral("database/user")).toString(), QStringLiteral("gambasse"));
    QCOMPARE(raw.value(QStringLiteral("database/password")).toString(),
             QStringLiteral("gambasse"));
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestAppSettings test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_appsettings.moc"
