#include <QApplication>
#include <QGuiApplication>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QScreen>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QStyleFactory>
#include <cstdio>

#include <Paths.h>
#include <data/common/Database.h>
#include <ui/window/MainWindow.h>
#include <ui/window/SplashWindow.h>

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
} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Gambasse"));
    QApplication::setOrganizationName(QStringLiteral("Gambasse"));
    QApplication::setApplicationVersion(QStringLiteral(GAMBASSE_VERSION));
    // Fusion is necessary for the custom dark palette to apply consistently;
    // the native Windows style ignores the palette.
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Deployment base path (the root directory above lib/). The launcher passes
    // it through --base; otherwise the executable directory is used.
    {
        const QStringList args = app.arguments();
        const int bi = args.indexOf(QStringLiteral("--base"));
        if (bi >= 0 && bi + 1 < args.size())
            gambasse::setBasePath(args.at(bi + 1));
    }

    // Development utility: rasterizes an SVG to a PNG scaled to a given height,
    // using Qt's qsvg image plugin. Usage: --svg2png <in.svg> <out.png> <height>
    if (app.arguments().contains(QStringLiteral("--svg2png"))) {
        attachConsoleIfNeeded();
        const QStringList a = app.arguments();
        const int i = a.indexOf(QStringLiteral("--svg2png"));
        const QString in = a.value(i + 1);
        const QString out = a.value(i + 2);
        int height = a.value(i + 3).toInt();
        if (height <= 0)
            height = 96;
        QPixmap pm(in);
        if (pm.isNull()) {
            std::fprintf(stderr, "svg2png: could not load %s\n", in.toUtf8().constData());
            return 2;
        }
        const QPixmap scaled = pm.scaledToHeight(height, Qt::SmoothTransformation);
        const bool ok = scaled.save(out, "PNG");
        std::fprintf(stdout, "svg2png: %s -> %s (%dx%d) %s\n", in.toUtf8().constData(),
                     out.toUtf8().constData(), scaled.width(), scaled.height(),
                     ok ? "OK" : "FAILED");
        std::fflush(stdout);
        return ok ? 0 : 3;
    }

    // Open the SQLite database (path from config.ini, database.db by default).
    QString error;
    if (!gambasse::Database::instance().open(&error)) {
        QMessageBox::critical(nullptr, QObject::tr("Database error"), error);
        return 1;
    }

    // Diagnostic mode: count patients and exit (database/deployment verification).
    if (app.arguments().contains(QStringLiteral("--check-db"))) {
        attachConsoleIfNeeded();
        QSqlQuery q(QStringLiteral("SELECT COUNT(*) FROM b01_patient"),
                    gambasse::Database::instance().connection());
        long long n = -1;
        if (q.next())
            n = q.value(0).toLongLong();
        std::fprintf(stdout, "PATIENTS=%lld\n", n);
        std::fflush(stdout);
        return 0;
    }

    // CRUD diagnostic: exercises insert/duplicate/update/delete inside a
    // transaction followed by ROLLBACK, so it does not alter real data.
    if (app.arguments().contains(QStringLiteral("--crud-selftest"))) {
        attachConsoleIfNeeded();
        auto& db = gambasse::Database::instance();
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

    // Create the main window now and show it when the splash screen finishes.
    auto* window = new gambasse::MainWindow();

    auto* splash = new gambasse::SplashWindow();
    QObject::connect(splash, &gambasse::SplashWindow::finished, window, [window, splash]() {
        // Center on the primary screen's available area, like the splash.
        if (QScreen* screen = QGuiApplication::primaryScreen()) {
            const QRect g = screen->availableGeometry();
            window->move(g.center() - window->rect().center());
        }
        window->show();
        splash->deleteLater();
    });
    splash->show();

    return app.exec();
}
