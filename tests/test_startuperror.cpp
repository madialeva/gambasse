#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScopeGuard>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

namespace gambasse {
namespace {

// A base directory whose database cannot be opened: its path is a directory.
void prepareBrokenBase(const QTemporaryDir& base, const QString& language) {
    QDir(base.path()).mkdir(QStringLiteral("isdir"));
    QSettings config(base.filePath(QStringLiteral("config.ini")), QSettings::IniFormat);
    config.setValue(QStringLiteral("database/path"), base.filePath(QStringLiteral("isdir")));
    config.setValue(QStringLiteral("idioma/codigo"), language);
}

// Never on the user's desktop, however the suite is started.
void startApplication(QProcess& app, const QStringList& arguments) {
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
    app.setProcessEnvironment(environment);
    app.start(QStringLiteral(GAMBASSE_EXECUTABLE), arguments);
}

} // namespace

// Runs the real executable with a database that cannot be opened, since only
// main.cpp decides when the language catalog is installed: the error comes out
// in the interface language, and the diagnostic modes keep it on the console in
// English.
class TestStartupError : public QObject {
    Q_OBJECT

private slots:
    void interfaceErrorFollowsTheLanguage_data();
    void interfaceErrorFollowsTheLanguage();
    void diagnosticErrorStaysInEnglish();
};

void TestStartupError::interfaceErrorFollowsTheLanguage_data() {
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("expected");
    QTest::newRow("es") << QStringLiteral("es") << QStringLiteral("No se pudo abrir la base de datos");
    QTest::newRow("pt") << QStringLiteral("pt") << QStringLiteral("Não foi possível abrir a base de dados");
    QTest::newRow("en") << QStringLiteral("en") << QStringLiteral("Could not open the database");
}

void TestStartupError::interfaceErrorFollowsTheLanguage() {
    QFETCH(QString, language);
    QFETCH(QString, expected);
    QTemporaryDir base;
    QVERIFY(base.isValid());
    prepareBrokenBase(base, language);

    QProcess app;
    // It would stay waiting in its error window.
    const auto stop = qScopeGuard([&app] {
        app.kill();
        app.waitForFinished();
    });
    startApplication(app, {QStringLiteral("--base"), base.path()});
    QVERIFY2(app.waitForStarted(), qPrintable(app.errorString()));

    QByteArray output;
    const auto reported = [&] {
        output += app.readAllStandardError();
        return QString::fromUtf8(output).contains(expected);
    };
    QTRY_VERIFY2_WITH_TIMEOUT(reported(), output.constData(), 30000);
}

void TestStartupError::diagnosticErrorStaysInEnglish() {
    QTemporaryDir base;
    QVERIFY(base.isValid());
    prepareBrokenBase(base, QStringLiteral("es"));

    QProcess app;
    startApplication(app, {QStringLiteral("--base"), base.path(), QStringLiteral("--check-db")});
    QVERIFY2(app.waitForFinished(30000), qPrintable(app.errorString()));
    QCOMPARE(app.exitStatus(), QProcess::NormalExit);
    QCOMPARE(app.exitCode(), 1);
    const QString output = QString::fromUtf8(app.readAllStandardError());
    QVERIFY2(output.contains(QStringLiteral("Could not open the database")), qPrintable(output));
    QVERIFY2(!output.contains(QStringLiteral("No se pudo")), qPrintable(output));
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestStartupError test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_startuperror.moc"
