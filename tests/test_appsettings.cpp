#include <QCoreApplication>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <logic/AppSettings.h>

namespace gambasse {

// Verifies the language/theme persistence against a temporary config.ini.
class TestAppSettings : public QObject {
    Q_OBJECT

private slots:
    void defaults();
    void roundTrip();
};

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

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestAppSettings test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_appsettings.moc"
