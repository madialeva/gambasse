#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QSqlQuery>
#include <QStringList>
#include <QVariantMap>
#include <cstdio>

#include <Paths.h>
#include <data/common/Database.h>
#include <ui/AppIcon.h>
#include <ui/InterfaceSettings.h>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

namespace {
// A GUI application in the WIN32 subsystem has no attached console, so stdout
// does not reach the terminal. Attach the parent console for diagnostic modes
// unless output is already redirected to a file or pipe.
void attachConsoleIfNeeded() {
#ifdef Q_OS_WIN
    const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    const DWORD outputType = GetFileType(hOut);
    const bool redirected = (outputType == FILE_TYPE_DISK || outputType == FILE_TYPE_PIPE);
    if (!redirected && AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* f = nullptr;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);
    }
#endif
}

// Engine ready for the views: the Basic style as a blank canvas for Theme, and
// the InterfaceSettings singleton created up front so the font and the language
// catalog are installed before the first view is built.
void prepareEngine(QQmlApplicationEngine& engine) {
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    engine.singletonInstance<gambasse::InterfaceSettings*>(QStringLiteral("Gambasse"),
                                                           QStringLiteral("InterfaceSettings"));
}

// Diagnostic modes: console only, in English, with no interface loaded.
// --check-db counts the patients (database/deployment verification);
// --crud-selftest exercises insert/duplicate/update/delete inside a transaction
// followed by ROLLBACK, so it does not alter real data.
int runDiagnostic(const QStringList& args) {
    attachConsoleIfNeeded();
    auto& db = gambasse::Database::instance();
    QString error;
    if (!db.open(&error)) {
        std::fprintf(stderr, "database: %s\n", error.toUtf8().constData());
        return 1;
    }

    if (args.contains(QStringLiteral("--check-db"))) {
        QSqlQuery q(QStringLiteral("SELECT COUNT(*) FROM b01_patient"), db.connection());
        long long n = -1;
        if (q.next())
            n = q.value(0).toLongLong();
        std::fprintf(stdout, "PATIENTS=%lld\n", n);
        std::fflush(stdout);
        return 0;
    }

    db.connection().transaction();
    gambasse::Patient p;
    p.name = QStringLiteral("__SELFTEST__");
    p.birthDate = QDate(2000, 1, 2);
    p.sex = gambasse::Patient::Sex::Muller;
    p.ageRange = 24;
    p.address = QStringLiteral("rua x");
    const bool ins = db.insert(p);                 // assigns p.id
    const bool dupDetect = db.hasDuplicate(p, -1); // existing patient -> true
    const bool dupSelf = db.hasDuplicate(p, p.id); // excluding itself -> false
    p.address = QStringLiteral("rua y");
    const bool upd = db.update(p);
    const bool del = db.remove(p.id);
    db.connection().rollback();
    std::fprintf(stdout, "INSERT=%d id=%lld DUPDETECT=%d DUPSELF=%d UPDATE=%d DELETE=%d\n",
                 ins, static_cast<long long>(p.id), dupDetect, dupSelf, upd, del);
    std::fflush(stdout);
    return 0;
}
} // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Gambasse"));
    QGuiApplication::setOrganizationName(QStringLiteral("Gambasse"));
    QGuiApplication::setApplicationVersion(QStringLiteral(GAMBASSE_VERSION));
    // The title bar logo for the task bar and every window, instead of the
    // generic default icon.
    QGuiApplication::setWindowIcon(gambasse::applicationIcon());

    const QStringList args = app.arguments();

    // Base path of the data: the executable's directory, unless --base gives
    // another one.
    const int bi = args.indexOf(QStringLiteral("--base"));
    if (bi >= 0 && bi + 1 < args.size())
        gambasse::setBasePath(args.at(bi + 1));

    if (args.contains(QStringLiteral("--check-db")) || args.contains(QStringLiteral("--crud-selftest")))
        return runDiagnostic(args);

    // The engine comes before the database: its settings singleton installs the
    // language catalog, and the database builds its error messages with tr().
    QQmlApplicationEngine engine;
    prepareEngine(engine);

    // Open the SQLite database (path from config.ini, database.db by default).
    // The interface shows the error in a themed, translated window and exits
    // once it is closed.
    QString error;
    if (!gambasse::Database::instance().open(&error)) {
        std::fprintf(stderr, "database: %s\n", error.toUtf8().constData());
        engine.setInitialProperties(
            {{QStringLiteral("titleText"), QObject::tr("Database error")},
             {QStringLiteral("messageText"), error}});
        engine.loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("StartupError"));
        if (!engine.rootObjects().isEmpty())
            app.exec();
        return 1;
    }

    // The views take over from here: App.qml shows the splash and then the
    // main shell, which reads the database opened above.
    engine.loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("App"));
    if (engine.rootObjects().isEmpty()) {
        std::fprintf(stderr, "qml: could not load the interface\n");
        return 1;
    }
    return app.exec();
}
