#include <QApplication>
#include <QGuiApplication>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QScreen>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QStyleFactory>
#include <cstdio>

#include <Paths.h>
#include <data/common/Database.h>
#include <ui/InterfaceSettings.h>
#include <ui/PatientController.h>
#include <ui/window/MainWindow.h>
#include <ui/window/SplashWindow.h>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

// Opens the QML main shell once the splash finishes. QML-defined signals can
// only connect via QMetaMethod to real slots (no lambdas), hence this helper.
class SplashGate : public QObject {
    Q_OBJECT
public:
    SplashGate(QQmlApplicationEngine* engine, gambasse::InterfaceSettings* settings,
               QObject* parent = nullptr)
        : QObject(parent), m_engine(engine), m_settings(settings) {
        m_patients = new gambasse::PatientController(this);
        m_patients->load();
    }
    gambasse::PatientController* patients() const { return m_patients; }
public slots:
    void openMain() {
        m_engine->loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("Root"));
        if (m_engine->rootObjects().isEmpty())
            return;
        QObject* root = m_engine->rootObjects().last();
        // Inject the bridges (Theme.mode and the patient screen follow them).
        root->setProperty("uiSettings",
                          QVariant::fromValue(qobject_cast<QObject*>(m_settings)));
        root->setProperty("patientController",
                          QVariant::fromValue(qobject_cast<QObject*>(m_patients)));
        // Centered like the splash it replaces, so the shell does not open in
        // a corner while the splash was in the middle of the screen.
        if (auto* mainWindow = qobject_cast<QQuickWindow*>(root)) {
            QScreen* screen = mainWindow->screen();
            if (screen == nullptr)
                screen = QGuiApplication::primaryScreen();
            if (screen != nullptr) {
                const QRect g = screen->availableGeometry();
                mainWindow->setPosition(g.center() - mainWindow->geometry().center());
            }
        }
    }
private:
    QQmlApplicationEngine* m_engine;
    gambasse::InterfaceSettings* m_settings;
    gambasse::PatientController* m_patients = nullptr;
};

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

    // QML mode: same diagnostics above apply; only the UI shell changes.
    // The Widgets windows stay the default until the final cutover.
    if (app.arguments().contains(QStringLiteral("--qml"))) {
        // Blank canvas for the custom Theme (see qml/Theme.qml in task 2.2).
        QQuickStyle::setStyle(QStringLiteral("Basic"));
        QQmlApplicationEngine engine;
        gambasse::InterfaceSettings uiSettings;
        auto* gate = new SplashGate(&engine, &uiSettings, &app);
        // Language catalog and hot QML retranslation (same lookup as Widgets),
        // plus translated headers and labels for the patient screen.
        uiSettings.applyLanguage(&engine);
        QObject::connect(&uiSettings, &gambasse::InterfaceSettings::languageChanged, &engine,
                         [&engine, &uiSettings, gate]() {
                             uiSettings.applyLanguage(&engine);
                             gate->patients()->refreshLanguage();
                         });
        engine.loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("SplashWindow"));
        if (engine.rootObjects().isEmpty()) {
            std::fprintf(stderr, "qml: could not load the QML splash\n");
            return 1;
        }
        QObject* splash = engine.rootObjects().first();
        if (auto* splashWindow = qobject_cast<QQuickWindow*>(splash)) {
            // Center on the primary screen's available area, like the splash.
            if (QScreen* screen = QGuiApplication::primaryScreen()) {
                const QRect g = screen->availableGeometry();
                splashWindow->setPosition(g.center() - splashWindow->geometry().center());
            }
        }
        QObject::connect(splash, SIGNAL(finished()), gate, SLOT(openMain()));
        return app.exec();
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

#include "main.moc"
