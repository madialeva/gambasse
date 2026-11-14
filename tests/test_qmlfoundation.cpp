#include <QDesktopServices>
#include <QDirIterator>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRawFont>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTranslator>
#include <QtTest>
#include <tuple>

#include <Paths.h>
#include <ui/AppIcon.h>
#include <ui/AppInfo.h>
#include <data/common/Database.h>
#include <ui/InterfaceSettings.h>

namespace gambasse {
namespace {

// Shell background pixels for each theme (see qml/Theme.qml).
QColor expectedShell(const QString& mode) {
    return mode == QStringLiteral("oscuro") ? QColor(0x35, 0x35, 0x35)
                                            : QColor(0xF3, 0xF2, 0xEE);
}

QColor grabCenter(QQuickWindow* window) {
    const QImage shot = window->grabWindow();
    if (shot.isNull())
        return QColor(); // QCOMPARE/QTRY_COMPARE below report it cleanly
    return shot.pixelColor(shot.width() / 2, shot.height() / 2);
}

// Center of a QML item in window coordinates (for QTest mouse clicks).
QPoint itemCenter(QQuickItem* item) {
    return item->mapToScene(QPointF(item->width() / 2.0, item->height() / 2.0)).toPoint();
}

// Persists the interface settings the next engine's singleton starts from.
void persistInterface(const QString& theme, const QString& language) {
    InterfaceSettings settings(nullptr);
    settings.setTheme(theme);
    settings.setLanguage(language);
}

// The InterfaceSettings singleton of an engine (created on first use).
InterfaceSettings* settingsOf(QQmlEngine& engine) {
    return engine.singletonInstance<InterfaceSettings*>(QStringLiteral("Gambasse"),
                                                        QStringLiteral("InterfaceSettings"));
}

// Receives what the application opens externally, instead of a browser.
class UrlCatcher : public QObject {
    Q_OBJECT
public:
    QList<QUrl> opened;
public slots:
    void open(const QUrl& url) { opened.append(url); }
};

} // namespace

// Verifies the QML foundation wiring: theme/language persistence through
// InterfaceSettings, change signals, and hot QML updates end to end
// (bridge flip -> Theme bindings -> rendered pixels).
class TestQmlFoundation : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QQmlApplicationEngine* m_engine = nullptr;

    // Fresh shell loaded like the application does, with the given theme
    // persisted beforehand (read by the engine's settings singleton) and the
    // English source texts. Null when the module fails to load.
    QQuickWindow* openShell(const QString& theme);
    void cleanup();
    // The information dialog of the shell, once it is on screen; null if it
    // does not open.
    QObject* openAboutDialog(QQuickWindow* window);

private slots:
    void initTestCase();
    void themePersistsAndNotifies();
    void languagePersistsAndNotifies();
    void localeFollowsTheLanguage();
    void letterSizePersistsAndNotifies();
    void qmlShellFollowsThemeInHot();
    void qmlLanguageSwitchesInHot();
    void qtStringsFollowTheLanguage();
    void splashSequence();
    void appOpensShellAfterSplash();
    void applicationIconIsTheLogo();
    void appInfoReportsTheBuild();
    void aboutDialogContent();
    void aboutDialogIsCentredAndFits();
    void aboutDialogFollowsTheTheme();
    void aboutDialogCloses();
    void aboutDialogIsModal();
    void aboutDialogOpensTheLink();
    void aboutDialogShowsTheFontLicense();
    void interfaceFontComesFromTheExecutable();
    void everyItemUsesTheInterfaceFont();
    void everyTextHasItsGlyphs();
    void helpMenuOpensTheDialog();
    void titleBarMenusDropBelowTheBar();
    void helpAndDialogFollowTheLanguage();
    void titleBarChrome();
    void titleBarStandsOut();
    void titleBarWindowButtons();
    void titleBarZoomButtons();
    void titleBarDrag();
    void windowEdgeResize();
};

QQuickWindow* TestQmlFoundation::openShell(const QString& theme) {
    cleanup();
    persistInterface(theme, QStringLiteral("en"));
    m_engine = new QQmlApplicationEngine();
    // Up front, like the application: it installs the interface font.
    settingsOf(*m_engine);
    m_engine->loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("Root"));
    if (m_engine->rootObjects().isEmpty())
        return nullptr;
    return qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
}

void TestQmlFoundation::cleanup() {
    delete m_engine;
    m_engine = nullptr;
}

QObject* TestQmlFoundation::openAboutDialog(QQuickWindow* window) {
    auto* dialog = window->findChild<QObject*>(QStringLiteral("aboutDialog"));
    if (dialog == nullptr || !QMetaObject::invokeMethod(dialog, "present"))
        return nullptr;
    const bool opened = QTest::qWaitFor([dialog] { return dialog->property("opened").toBool(); });
    return opened ? dialog : nullptr;
}

void TestQmlFoundation::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    // The shell loads the patients on completion, as in the application.
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));
}

void TestQmlFoundation::themePersistsAndNotifies() {
    InterfaceSettings settings(nullptr);
    QCOMPARE(settings.theme(), QStringLiteral("claro")); // config.ini default

    QSignalSpy spy(&settings, &InterfaceSettings::themeChanged);
    settings.setTheme(QStringLiteral("oscuro"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.theme(), QStringLiteral("oscuro"));

    settings.setTheme(QStringLiteral("oscuro")); // same value: no signal
    QCOMPARE(spy.count(), 1);

    InterfaceSettings reloaded(nullptr); // persistence round trip
    QCOMPARE(reloaded.theme(), QStringLiteral("oscuro"));

    settings.setTheme(QStringLiteral("whatever")); // normalized to claro
    QCOMPARE(settings.theme(), QStringLiteral("claro"));
}

void TestQmlFoundation::languagePersistsAndNotifies() {
    InterfaceSettings settings(nullptr);
    QCOMPARE(settings.language(), QStringLiteral("pt")); // config.ini default

    QSignalSpy spy(&settings, &InterfaceSettings::languageChanged);
    settings.setLanguage(QStringLiteral("es"));
    QCOMPARE(spy.count(), 1);

    settings.setLanguage(QStringLiteral("es")); // same value: no signal
    QCOMPARE(spy.count(), 1);

    InterfaceSettings reloaded(nullptr); // persistence round trip
    QCOMPARE(reloaded.language(), QStringLiteral("es"));
}

// The zoom letter size of the history and consultation windows: one for the
// whole application, persisted and kept inside 13-26 px.
void TestQmlFoundation::letterSizePersistsAndNotifies() {
    InterfaceSettings settings(nullptr);
    QCOMPARE(settings.letterSize(), 13); // config.ini default
    const auto restore = qScopeGuard([&settings] { settings.setLetterSize(13); });

    QSignalSpy spy(&settings, &InterfaceSettings::letterSizeChanged);
    settings.setLetterSize(15);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.letterSize(), 15);

    settings.setLetterSize(15); // same value: no signal
    QCOMPARE(spy.count(), 1);

    InterfaceSettings reloaded(nullptr); // persistence round trip
    QCOMPARE(reloaded.letterSize(), 15);

    settings.setLetterSize(40); // brought into the range
    QCOMPARE(settings.letterSize(), 26);
    QCOMPARE(spy.count(), 2);
    settings.setLetterSize(5);
    QCOMPARE(settings.letterSize(), 13);
    QCOMPARE(spy.count(), 3);
    settings.setLetterSize(-1); // already at the base: no signal
    QCOMPARE(spy.count(), 3);
}

// Each language stands for a fixed locale, whatever the system's: the calendar
// takes its month and day names from it, and every week starts on Monday.
void TestQmlFoundation::localeFollowsTheLanguage() {
    InterfaceSettings settings(nullptr);
    const QString previous = settings.language();
    const auto restore = qScopeGuard([&settings, previous] { settings.setLanguage(previous); });
    settings.setLanguage(QStringLiteral("en"));

    QSignalSpy spy(&settings, &InterfaceSettings::languageChanged);
    const QList<std::pair<QString, QString>> expected = {
        {QStringLiteral("pt"), QStringLiteral("pt_GW")},
        {QStringLiteral("es"), QStringLiteral("es_ES")},
        {QStringLiteral("en"), QStringLiteral("en_GB")},
        // Shown in English by the interface, so English for the calendar too.
        {QStringLiteral("fr"), QStringLiteral("en_GB")},
    };
    int notifications = 0;
    for (const auto& [language, locale] : expected) {
        settings.setLanguage(language);
        QCOMPARE(spy.count(), ++notifications); // one per change
        QCOMPARE(settings.locale().name(), locale);
        QCOMPARE(settings.locale().firstDayOfWeek(), Qt::Monday);
        // The property reads the same value, for the views.
        QCOMPARE(settings.property("locale").toLocale().name(), locale);
    }
}

void TestQmlFoundation::qmlShellFollowsThemeInHot() {
    persistInterface(QStringLiteral("claro"), QStringLiteral("en"));
    QQmlApplicationEngine engine;
    // Same loading as the application: the module (and its resources) comes
    // from the shared GambasseQml library in both binaries.
    engine.loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("Root"));
    QVERIFY(!engine.rootObjects().isEmpty());

    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QCOMPARE(grabCenter(window), expectedShell(QStringLiteral("claro")));

    // Theme.mode follows the singleton: hot change, no reload.
    settingsOf(engine)->setTheme(QStringLiteral("oscuro"));
    QTRY_COMPARE(grabCenter(window), expectedShell(QStringLiteral("oscuro")));
}

void TestQmlFoundation::qmlLanguageSwitchesInHot() {
    // Deterministic baseline whatever previous tests persisted: the singleton
    // installs the persisted catalog when the engine creates it.
    persistInterface(QStringLiteral("claro"), QStringLiteral("pt"));
    QQmlApplicationEngine engine;
    InterfaceSettings* settings = settingsOf(engine);
    QVERIFY(settings != nullptr);

    QQmlComponent probe(
        &engine, QUrl::fromLocalFile(QStringLiteral(QML_FIXTURE_DIR "/LangProbe.qml")));
    QObject* item = probe.create();
    QVERIFY(item != nullptr);
    QCOMPARE(item->property("probed").toString(), QStringLiteral("No estábulo"));

    // Every change reapplies the catalog and retranslates the engine itself.
    settings->setLanguage(QStringLiteral("es"));
    QCOMPARE(item->property("probed").toString(), QStringLiteral("En establo"));
    settings->setLanguage(QStringLiteral("en")); // back to the source language
    QCOMPARE(item->property("probed").toString(), QStringLiteral("In stable"));

    InterfaceSettings reloaded(nullptr); // persistence rode along
    QCOMPARE(reloaded.language(), QStringLiteral("en"));
    delete item;
}

// Qt's own strings the photo chooser shows follow the interface language, not
// the system's: English answers with the sources even when a Qt catalog of
// another language is installed (QQmlApplicationEngine loads the system's).
void TestQmlFoundation::qtStringsFollowTheLanguage() {
    persistInterface(QStringLiteral("claro"), QStringLiteral("pt"));
    QQmlApplicationEngine engine;
    InterfaceSettings* settings = settingsOf(engine);
    QVERIFY(settings != nullptr);
    const auto qt = [](const char* context, const char* source) {
        return QCoreApplication::translate(context, source);
    };
    QCOMPARE(qt("FileDialog", "Filter"), QStringLiteral("Filtro"));
    QCOMPARE(qt("QStandardPaths", "Downloads"), QStringLiteral("Transferências"));
    QCOMPARE(qt("QPlatformTheme", "Open"), QStringLiteral("Abrir"));

    settings->setLanguage(QStringLiteral("es"));
    QCOMPARE(qt("QStandardPaths", "Downloads"), QStringLiteral("Descargas"));
    QCOMPARE(qt("QStandardPaths", "Pictures"), QStringLiteral("Imágenes"));

    // A Spanish Qt catalog installed first, like the system's on a Spanish
    // desktop, must not answer while the interface is in English.
    QTranslator system;
    QVERIFY(system.load(QStringLiteral("qtstrings_es"), QStringLiteral(":/i18n")));
    QCoreApplication::installTranslator(&system);
    settings->setLanguage(QStringLiteral("en"));
    QCOMPARE(qt("QStandardPaths", "Home"), QStringLiteral("Home"));
    QCOMPARE(qt("FileDialog", "Filter"), QStringLiteral("Filter"));
    QCoreApplication::removeTranslator(&system);

    // The engine installs the system's Qt catalog when it loads its first
    // view, after the singleton: loading a view puts the Qt strings back in
    // front of it.
    settings->setLanguage(QStringLiteral("pt"));
    QVERIFY(system.load(QStringLiteral("qtstrings_es"), QStringLiteral(":/i18n")));
    QCoreApplication::installTranslator(&system);
    QCOMPARE(qt("QStandardPaths", "Home"), QStringLiteral("Carpeta personal"));
    engine.loadData("import QtQuick\nItem {}");
    QCOMPARE(qt("QStandardPaths", "Home"), QStringLiteral("Pasta pessoal"));
    QCoreApplication::removeTranslator(&system);
}

void TestQmlFoundation::splashSequence() {
    QQmlApplicationEngine engine;
    engine.loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("SplashWindow"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto* splash = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(splash != nullptr);
    QCOMPARE(QSize(splash->width(), splash->height()), QSize(560, 470));
    QVERIFY(splash->flags() & Qt::FramelessWindowHint);
    QVERIFY(QTest::qWaitForWindowExposed(splash));

    QSignalSpy finishedSpy(splash, SIGNAL(finished()));
    QTRY_VERIFY_WITH_TIMEOUT(!splash->isVisible(), 5000);
    QCOMPARE(finishedSpy.count(), 1);
}

void TestQmlFoundation::appOpensShellAfterSplash() {
    // The application entry point: the splash first, then the main shell.
    persistInterface(QStringLiteral("claro"), QStringLiteral("en"));
    QQmlApplicationEngine engine;
    engine.loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("App"));
    QVERIFY(!engine.rootObjects().isEmpty());
    QObject* app = engine.rootObjects().first();
    auto* splash = qvariant_cast<QQuickWindow*>(app->property("splash"));
    QVERIFY(splash != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(splash));
    QVERIFY(app->property("shell").value<QObject*>() == nullptr);

    QTRY_VERIFY_WITH_TIMEOUT(app->property("shell").value<QObject*>() != nullptr, 5000);
    auto* shell = qvariant_cast<QQuickWindow*>(app->property("shell"));
    QVERIFY(shell != nullptr);
    QVERIFY(!splash->isVisible());
    QVERIFY(QTest::qWaitForWindowExposed(shell));
}

void TestQmlFoundation::titleBarStandsOut() {
    // Title bar background per theme (see qml/Theme.qml), distinct from the
    // window grey of expectedShell().
    const auto expectedBar = [](const QString& mode) {
        return mode == QStringLiteral("oscuro") ? QColor(0x24, 0x3B, 0x3A)
                                                : QColor(0xDC, 0xEE, 0xEA);
    };
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    auto* bar = window->findChild<QQuickItem*>(QStringLiteral("titleBar"));
    QVERIFY(bar != nullptr);
    // An empty spot of the bar: between the title and the window buttons.
    const QPoint spot = bar->mapToScene(QPointF(bar->width() / 2.0, 4)).toPoint();

    for (const QString& mode : {QStringLiteral("claro"), QStringLiteral("oscuro")}) {
        settingsOf(*m_engine)->setTheme(mode); // hot change
        QTRY_COMPARE(window->grabWindow().pixelColor(spot), expectedBar(mode));
        QVERIFY(expectedBar(mode) != expectedShell(mode));
    }
}

void TestQmlFoundation::applicationIconIsTheLogo() {
    // The title bar logo at every usual desktop size, not an empty icon.
    const QIcon icon = applicationIcon();
    QVERIFY(!icon.isNull());
    for (int size : {16, 24, 32, 48, 64, 128, 256}) {
        QVERIFY2(icon.availableSizes().contains(QSize(size, size)), qPrintable(QString::number(size)));
        const QImage image = icon.pixmap(QSize(size, size)).toImage();
        QCOMPARE(image.size(), QSize(size, size));
        // The logo is a filled disc: its centre is opaque.
        QCOMPARE(image.pixelColor(size / 2, size / 2).alpha(), 255);
    }
}

void TestQmlFoundation::appInfoReportsTheBuild() {
    QCoreApplication::setApplicationVersion(QStringLiteral("9.8.7"));
    const AppInfo info;
    QCOMPARE(info.version(), QStringLiteral("9.8.7"));
    QCOMPARE(info.languageStandard(), QStringLiteral("C++26"));
    // These suites and the module are built by the same compiler.
    QCOMPARE(info.compilerVersion(),
             QStringLiteral("%1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__));
    QCOMPARE(info.qtVersion(), QString::fromLatin1(qVersion()));
    QCOMPARE(info.repositoryUrl(), QStringLiteral("https://github.com/madialeva/gambasse"));
}

void TestQmlFoundation::aboutDialogContent() {
    QCoreApplication::setApplicationVersion(QStringLiteral("9.8.7"));
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QObject* dialog = openAboutDialog(window);
    QVERIFY(dialog != nullptr);

    const auto textOf = [dialog](const char* name) {
        auto* item = dialog->findChild<QQuickItem*>(QLatin1String(name));
        return item != nullptr ? item->property("text").toString() : QString();
    };
    const AppInfo info;
    QCOMPARE(textOf("aboutName"), QStringLiteral("Gambasse"));
    QVERIFY(textOf("aboutVersion").contains(QStringLiteral("9.8.7")));
    QVERIFY(textOf("aboutLicense").contains(QStringLiteral("MIT")));
    QVERIFY(textOf("aboutLanguage").contains(QStringLiteral("C++26")));
    QVERIFY(textOf("aboutLanguage").contains(QStringLiteral("ISO")));
    QVERIFY(textOf("aboutCompiler").contains(QStringLiteral("GCC ") + info.compilerVersion()));
    QVERIFY(textOf("aboutCompiler").contains(QStringLiteral("GPL v3")));
    QVERIFY(textOf("aboutCompiler").contains(QStringLiteral("Runtime Library Exception")));
    QVERIFY(textOf("aboutQt").contains(QStringLiteral("Qt ") + info.qtVersion()));
    QVERIFY(textOf("aboutQt").contains(QStringLiteral("LGPL v3")));
    QVERIFY(textOf("aboutFont").contains(QStringLiteral("Noto Sans")));
    QVERIFY(textOf("aboutFont").contains(QStringLiteral("SIL OFL 1.1")));
    QVERIFY(textOf("aboutLink").contains(QStringLiteral("https://github.com/madialeva/gambasse")));

    auto* logo = dialog->findChild<QQuickItem*>(QStringLiteral("aboutLogo"));
    QVERIFY(logo != nullptr);
    QCOMPARE(logo->property("source").toUrl(), QUrl(QStringLiteral("qrc:/img/logo.svg")));
    QCOMPARE(logo->width(), qreal(96));
    QCOMPARE(logo->height(), qreal(96));
    QTRY_COMPARE(logo->property("status").toInt(), 1); // Image.Ready
}

void TestQmlFoundation::aboutDialogIsCentredAndFits() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QCOMPARE(window->size(), QSize(1008, 561));
    QObject* dialog = openAboutDialog(window);
    QVERIFY(dialog != nullptr);

    auto* content = dialog->property("contentItem").value<QQuickItem*>();
    QVERIFY(content != nullptr);
    const QRectF area = content->mapRectToScene(QRectF(0, 0, content->width(), content->height()));
    QCOMPARE(qRound(area.center().x()), window->width() / 2);
    QVERIFY(qAbs(area.center().y() - window->height() / 2.0) <= 1);
    QVERIFY(area.top() >= 0 && area.bottom() <= window->height());

    auto* logo = dialog->findChild<QQuickItem*>(QStringLiteral("aboutLogo"));
    QVERIFY(logo != nullptr);
    QCOMPARE(qRound(itemCenter(logo).x() - area.center().x()), 0);
}

void TestQmlFoundation::aboutDialogFollowsTheTheme() {
    const QList<std::pair<QString, QColor>> themes = {
        {QStringLiteral("claro"), QColor(0x1F, 0x6F, 0xB8)},
        {QStringLiteral("oscuro"), QColor(0x58, 0xA6, 0xFF)}};
    for (const auto& [mode, linkColor] : themes) {
        QQuickWindow* window = openShell(mode);
        QVERIFY(window != nullptr);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QObject* dialog = openAboutDialog(window);
        QVERIFY(dialog != nullptr);

        auto* link = dialog->findChild<QQuickItem*>(QStringLiteral("aboutLink"));
        QVERIFY(link != nullptr);
        QCOMPARE(link->property("linkColor").value<QColor>(), linkColor);
        auto* name = dialog->findChild<QQuickItem*>(QStringLiteral("aboutName"));
        QVERIFY(name != nullptr);
        QCOMPARE(name->property("color").value<QColor>(),
                 mode == QStringLiteral("oscuro") ? QColor(Qt::white) : QColor(0x1A, 0x1A, 0x1A));
    }
}

void TestQmlFoundation::aboutDialogCloses() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QObject* dialog = openAboutDialog(window);
    QVERIFY(dialog != nullptr);
    auto* ok = dialog->findChild<QQuickItem*>(QStringLiteral("aboutOkButton"));
    QVERIFY(ok != nullptr);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(ok));
    QTRY_VERIFY(!dialog->property("visible").toBool());

    QCOMPARE(openAboutDialog(window), dialog);
    QTest::keyClick(window, Qt::Key_Escape);
    QTRY_VERIFY(!dialog->property("visible").toBool());
}

void TestQmlFoundation::aboutDialogIsModal() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QObject* dialog = openAboutDialog(window);
    QVERIFY(dialog != nullptr);

    // A button with a visible effect, well outside the dialog.
    auto* bar = window->findChild<QObject*>(QStringLiteral("titleBar"));
    QVERIFY(bar != nullptr);
    auto* maximizeButton = bar->findChild<QQuickItem*>(QStringLiteral("maximizeButton"));
    QVERIFY(maximizeButton != nullptr);
    const QPoint outside = itemCenter(maximizeButton);
    const QWindow::Visibility before = window->visibility();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, outside);
    QTest::qWait(100);
    QCOMPARE(window->visibility(), before);
    QVERIFY(dialog->property("visible").toBool());

    // The same click does act once the dialog is closed.
    QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
    QTRY_VERIFY(!dialog->property("visible").toBool());
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, outside);
    QTRY_COMPARE(window->visibility(), QWindow::Maximized);
}

void TestQmlFoundation::aboutDialogOpensTheLink() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QObject* dialog = openAboutDialog(window);
    QVERIFY(dialog != nullptr);
    auto* link = dialog->findChild<QQuickItem*>(QStringLiteral("aboutLink"));
    QVERIFY(link != nullptr);

    UrlCatcher catcher;
    QDesktopServices::setUrlHandler(QStringLiteral("https"), &catcher, "open");
    // The address is the only text of the item and it is centred.
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(link));
    QDesktopServices::unsetUrlHandler(QStringLiteral("https"));

    QCOMPARE(catcher.opened, QList<QUrl>{QUrl(QStringLiteral("https://github.com/madialeva/gambasse"))});
    QVERIFY(dialog->property("visible").toBool());
}

// The licence has to go with the font, and the executable may be a single file:
// it is read from inside the application, and closing it goes back to the dialog.
void TestQmlFoundation::aboutDialogShowsTheFontLicense() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QObject* dialog = openAboutDialog(window);
    QVERIFY(dialog != nullptr);

    auto* link = dialog->findChild<QQuickItem*>(QStringLiteral("aboutFont"));
    auto* licenseDialog = dialog->findChild<QObject*>(QStringLiteral("fontLicenseDialog"));
    QVERIFY(link != nullptr && licenseDialog != nullptr);
    QVERIFY(QMetaObject::invokeMethod(link, "linkActivated", Q_ARG(QString, QStringLiteral("license"))));
    QTRY_VERIFY(licenseDialog->property("opened").toBool());

    auto* text = licenseDialog->findChild<QQuickItem*>(QStringLiteral("fontLicenseText"));
    QVERIFY(text != nullptr);
    const QString license = text->property("text").toString();
    for (const char* expected : {"Noto Sans, version 2.003", "Copyright 2015 Google Inc.",
                                 "The Noto Project Authors", "SIL OPEN FONT LICENSE Version 1.1",
                                 "OTHER DEALINGS IN THE FONT SOFTWARE."})
        QVERIFY2(license.contains(QLatin1String(expected)), expected);

    // Whole inside the base window, and all the text can be reached by scrolling.
    auto* content = licenseDialog->property("contentItem").value<QQuickItem*>();
    QVERIFY(content != nullptr);
    const QRectF area = content->mapRectToScene(QRectF(0, 0, content->width(), content->height()));
    QVERIFY(area.top() >= 0 && area.bottom() <= window->height());
    auto* view = licenseDialog->findChild<QQuickItem*>(QStringLiteral("fontLicenseView"));
    QVERIFY(view != nullptr);
    QVERIFY(view->property("contentHeight").toReal() > view->height());
    QCOMPARE(view->property("contentHeight").toReal(), text->height());
    QVERIFY(text->property("contentHeight").toReal() <= text->height());

    auto* ok = licenseDialog->findChild<QQuickItem*>(QStringLiteral("fontLicenseOkButton"));
    QVERIFY(ok != nullptr);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(ok));
    QTRY_VERIFY(!licenseDialog->property("visible").toBool());
    QVERIFY(dialog->property("opened").toBool());
}

// ctest runs the suites with no system font: Noto Sans can only come from the
// executable. The sample width holds without hinting; with it the result depends
// on the system's font configuration (174 px here).
void TestQmlFoundation::interfaceFontComesFromTheExecutable() {
    QVERIFY(openShell(QStringLiteral("claro")) != nullptr);
    const QFont font = QGuiApplication::font();
    QCOMPARE(font.family(), QStringLiteral("Noto Sans"));
    QCOMPARE(font.pixelSize(), 13);
    QCOMPARE(font.hintingPreference(), QFont::PreferNoHinting);
    const QFontInfo regular(font);
    QCOMPARE(regular.family(), QStringLiteral("Noto Sans"));
    QCOMPARE(regular.styleName(), QStringLiteral("Regular"));
    QFont boldFont = font;
    boldFont.setBold(true);
    QCOMPARE(QFontInfo(boldFont).styleName(), QStringLiteral("Bold"));
    const qreal width =
        QFontMetricsF(font).horizontalAdvance(QStringLiteral("Temperatura (ºC): 28/08/2026"));
    QVERIFY2(qAbs(width - 178.67) < 0.5, qPrintable(QString::number(width)));
}

void TestQmlFoundation::everyItemUsesTheInterfaceFont() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    int texts = 0;
    QList<QQuickItem*> pending = {window->contentItem()};
    while (!pending.isEmpty()) {
        QQuickItem* item = pending.takeLast();
        pending += item->childItems();
        const QVariant value = item->property("font");
        if (value.metaType() != QMetaType::fromType<QFont>())
            continue;
        ++texts;
        const QFont font = value.value<QFont>();
        const QString where = QString::fromLatin1(item->metaObject()->className());
        QVERIFY2(font.family() == QStringLiteral("Noto Sans"), qPrintable(where));
        QVERIFY2(font.hintingPreference() == QFont::PreferNoHinting, qPrintable(where));
    }
    QVERIFY(texts > 20);
}

// A character the font lacks would come from a system font, or from none. The
// catalogs hold every translated text; QML, the fixed ones and the symbols.
void TestQmlFoundation::everyTextHasItsGlyphs() {
    QStringList missing;
    const auto check = [&missing](const QString& file, char32_t character) {
        if (character < 0x7F)
            return;
        for (const char* name : {":/fonts/NotoSans-Regular.ttf", ":/fonts/NotoSans-Bold.ttf"}) {
            const QRawFont font(QLatin1String(name), 13);
            if (!font.isValid() || !font.supportsCharacter(character))
                missing << QStringLiteral("%1: U+%2 in %3")
                               .arg(file)
                               .arg(quint32(character), 4, 16, QLatin1Char('0'))
                               .arg(QLatin1String(name));
        }
    };

    QDirIterator catalogs(QStringLiteral(TRANSLATIONS_DIR), {QStringLiteral("*.ts")}, QDir::Files);
    int files = 0;
    while (catalogs.hasNext()) {
        const QString path = catalogs.next();
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        ++files;
        for (const char32_t character : QString::fromUtf8(file.readAll()).toUcs4())
            check(path, character);
    }
    QVERIFY(files >= 2);

    static const QRegularExpression escape(QStringLiteral("\\\\u([0-9A-Fa-f]{4})"));
    QDirIterator views(QStringLiteral(QML_SOURCE_DIR), {QStringLiteral("*.qml")}, QDir::Files,
                       QDirIterator::Subdirectories);
    files = 0;
    while (views.hasNext()) {
        const QString path = views.next();
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        ++files;
        for (const QString& line : QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'))) {
            const QString code = line.trimmed().startsWith(QStringLiteral("//")) ? QString() : line;
            for (const char32_t character : code.toUcs4())
                check(path, character);
            for (auto match = escape.globalMatch(code); match.hasNext();)
                check(path, char32_t(match.next().captured(1).toUInt(nullptr, 16)));
        }
    }
    QVERIFY(files > 20);
    QVERIFY2(missing.isEmpty(), qPrintable(missing.join(QStringLiteral(" | "))));
}

void TestQmlFoundation::helpMenuOpensTheDialog() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    // From the bar: the hidden child windows hold bars of their own.
    auto* bar = window->findChild<QObject*>(QStringLiteral("titleBar"));
    QVERIFY(bar != nullptr);
    auto* helpButton = bar->findChild<QQuickItem*>(QStringLiteral("helpButton"));
    auto* languageButton = bar->findChild<QQuickItem*>(QStringLiteral("languageButton"));
    auto* menu = bar->findChild<QObject*>(QStringLiteral("helpMenu"));
    auto* dialog = window->findChild<QObject*>(QStringLiteral("aboutDialog"));
    QVERIFY(helpButton != nullptr);
    QVERIFY(languageButton != nullptr);
    QVERIFY(menu != nullptr);
    QVERIFY(dialog != nullptr);

    // Right beside the language button, on its left (the row's 6 px apart).
    QVERIFY(helpButton->isVisible());
    QCOMPARE(helpButton->property("text").toString(), QStringLiteral("Help"));
    QCOMPARE(helpButton->x() + helpButton->width() + 6, languageButton->x());

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(helpButton));
    QTRY_VERIFY(menu->property("opened").toBool());
    QCOMPARE(menu->property("count").toInt(), 1);

    QQuickItem* entry = nullptr;
    QMetaObject::invokeMethod(menu, "itemAt", Q_RETURN_ARG(QQuickItem*, entry), Q_ARG(int, 0));
    QVERIFY(entry != nullptr);
    QCOMPARE(entry->property("text").toString(), QStringLiteral("About"));

    // The real click: the dialog must survive the focus change of the event
    // that closes the menu.
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(entry));
    QTRY_VERIFY(dialog->property("opened").toBool());
    QVERIFY(!menu->property("visible").toBool());
    QTest::qWait(100);
    QVERIFY(dialog->property("visible").toBool());
}

void TestQmlFoundation::titleBarMenusDropBelowTheBar() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    auto* bar = window->findChild<QObject*>(QStringLiteral("titleBar"));
    QVERIFY(bar != nullptr);
    const QList<std::pair<const char*, const char*>> menus = {{"helpButton", "helpMenu"},
                                                              {"languageButton", "languageMenu"},
                                                              {"themeButton", "themeMenu"}};
    for (const auto& [buttonName, menuName] : menus) {
        auto* button = bar->findChild<QQuickItem*>(QLatin1String(buttonName));
        auto* menu = bar->findChild<QObject*>(QLatin1String(menuName));
        QVERIFY2(button != nullptr, buttonName);
        QVERIFY2(menu != nullptr, menuName);

        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(button));
        QTRY_VERIFY2(menu->property("opened").toBool(), menuName);

        auto* content = menu->property("contentItem").value<QQuickItem*>();
        QVERIFY2(content != nullptr, menuName);
        const QPointF menuOrigin = content->mapToScene(QPointF(0, 0));
        const QPointF buttonOrigin = button->mapToScene(QPointF(0, 0));
        const qreal padding = menu->property("leftPadding").toReal();
        const qreal topPadding = menu->property("topPadding").toReal();
        QCOMPARE(menuOrigin.x() - padding, buttonOrigin.x());
        QVERIFY2(menuOrigin.y() - topPadding >= buttonOrigin.y() + button->height(), menuName);

        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_VERIFY2(!menu->property("visible").toBool(), menuName);
    }
}

void TestQmlFoundation::helpAndDialogFollowTheLanguage() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QCOMPARE(window->size(), QSize(1008, 561));
    auto* bar = window->findChild<QObject*>(QStringLiteral("titleBar"));
    QVERIFY(bar != nullptr);
    auto* helpButton = bar->findChild<QQuickItem*>(QStringLiteral("helpButton"));
    auto* menu = bar->findChild<QObject*>(QStringLiteral("helpMenu"));
    QVERIFY(helpButton != nullptr);
    QVERIFY(menu != nullptr);
    QQuickItem* entry = nullptr;
    QMetaObject::invokeMethod(menu, "itemAt", Q_RETURN_ARG(QQuickItem*, entry), Q_ARG(int, 0));
    QVERIFY(entry != nullptr);
    InterfaceSettings* settings = settingsOf(*m_engine);
    QVERIFY(settings != nullptr);

    struct Expected {
        QString code, help, about, version, license, language, compiler, qt, font, ok;
    };
    const QList<Expected> languages = {
        {QStringLiteral("es"), QStringLiteral("Ayuda"), QStringLiteral("Información"),
         QStringLiteral("Versión"), QStringLiteral("Licencia MIT"), QStringLiteral("estándar ISO"),
         QStringLiteral("biblioteca de ejecución de GCC"), QStringLiteral("licencia LGPL v3"),
         QStringLiteral("la fuente Noto Sans"), QStringLiteral("Aceptar")},
        {QStringLiteral("pt"), QStringLiteral("Ajuda"), QStringLiteral("Informação"),
         QStringLiteral("Versão"), QStringLiteral("Licença MIT"), QStringLiteral("norma ISO"),
         QStringLiteral("biblioteca de execução do GCC"), QStringLiteral("licença LGPL v3"),
         QStringLiteral("o tipo de letra Noto Sans"), QStringLiteral("OK")},
        {QStringLiteral("en"), QStringLiteral("Help"), QStringLiteral("About"),
         QStringLiteral("Version"), QStringLiteral("MIT License"), QStringLiteral("ISO standard"),
         QStringLiteral("GCC Runtime Library Exception"), QStringLiteral("LGPL v3"),
         QStringLiteral("the Noto Sans font"), QStringLiteral("OK")}};

    for (const Expected& expected : languages) {
        settings->setLanguage(expected.code);
        QCOMPARE(helpButton->property("text").toString(), expected.help);
        QCOMPARE(entry->property("text").toString(), expected.about);

        QObject* dialog = openAboutDialog(window);
        QVERIFY(dialog != nullptr);
        const auto textOf = [dialog](const char* name) {
            auto* item = dialog->findChild<QQuickItem*>(QLatin1String(name));
            return item != nullptr ? item->property("text").toString() : QString();
        };
        QVERIFY2(textOf("aboutVersion").startsWith(expected.version), qPrintable(expected.code));
        QCOMPARE(textOf("aboutLicense"), expected.license);
        QVERIFY2(textOf("aboutLanguage").contains(expected.language), qPrintable(expected.code));
        QVERIFY2(textOf("aboutCompiler").contains(expected.compiler), qPrintable(expected.code));
        QVERIFY2(textOf("aboutQt").contains(expected.qt), qPrintable(expected.code));
        QVERIFY2(textOf("aboutFont").contains(expected.font), qPrintable(expected.code));
        QCOMPARE(textOf("aboutOkButton"), expected.ok);

        // Whole inside the base window, and every text at its full height.
        auto* content = dialog->property("contentItem").value<QQuickItem*>();
        QVERIFY(content != nullptr);
        const QRectF area =
            content->mapRectToScene(QRectF(0, 0, content->width(), content->height()));
        QVERIFY2(area.top() >= 0 && area.bottom() <= window->height(), qPrintable(expected.code));
        for (const char* name : {"aboutName", "aboutVersion", "aboutLicense", "aboutLanguage",
                                 "aboutCompiler", "aboutQt", "aboutFont", "aboutLink"}) {
            auto* text = dialog->findChild<QQuickItem*>(QLatin1String(name));
            QVERIFY2(text != nullptr, name);
            QVERIFY2(text->property("contentHeight").toReal() <= text->height(), name);
            const QRectF box = text->mapRectToScene(QRectF(0, 0, text->width(), text->height()));
            QVERIFY2(box.top() >= area.top() && box.bottom() <= area.bottom(), name);
        }
        auto* ok = dialog->findChild<QQuickItem*>(QStringLiteral("aboutOkButton"));
        QVERIFY(ok != nullptr);
        QVERIFY(ok->mapRectToScene(QRectF(0, 0, ok->width(), ok->height())).bottom() <= area.bottom());

        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
    }
}

void TestQmlFoundation::titleBarChrome() {
    // Same assertions in both themes (tooltips stay in English: no
    // translator is installed in tests, like a fresh English session).
    for (const QString& mode : {QStringLiteral("claro"), QStringLiteral("oscuro")}) {
        QQuickWindow* window = openShell(mode);
        QVERIFY(window != nullptr);
        QVERIFY(QTest::qWaitForWindowExposed(window));

        QObject* bar = window->findChild<QObject*>(QStringLiteral("titleBar"));
        QVERIFY(bar != nullptr);
        // 40 px of buttons plus the 1 px bottom line: every button of the bar
        // fits exactly above that line.
        QCOMPARE(bar->property("height").toInt(), 41);
        for (const char* name : {"helpButton", "languageButton", "themeButton", "minimizeButton",
                                 "maximizeButton", "closeButton"}) {
            auto* button = bar->findChild<QQuickItem*>(QLatin1String(name));
            QVERIFY2(button != nullptr, name);
            const QPointF top = button->mapToItem(qobject_cast<QQuickItem*>(bar), QPointF(0, 0));
            QVERIFY2(top.y() == 0 && button->height() == 40, name);
        }

        auto* minimizeButton = bar->findChild<QQuickItem*>(QStringLiteral("minimizeButton"));
        auto* maximizeButton = bar->findChild<QQuickItem*>(QStringLiteral("maximizeButton"));
        auto* closeButton = bar->findChild<QQuickItem*>(QStringLiteral("closeButton"));
        QVERIFY(minimizeButton != nullptr);
        QVERIFY(maximizeButton != nullptr);
        QVERIFY(closeButton != nullptr);
        QVERIFY(minimizeButton->isVisible());
        QVERIFY(maximizeButton->isVisible());
        QVERIFY(closeButton->isVisible());
        QCOMPARE(minimizeButton->property("tip").toString(), QStringLiteral("Minimize"));
        QCOMPARE(maximizeButton->property("tip").toString(), QStringLiteral("Maximize"));
        QCOMPARE(closeButton->property("tip").toString(), QStringLiteral("Close"));
        QCOMPARE(maximizeButton->property("maximized").toBool(), false);
    }
}

void TestQmlFoundation::titleBarWindowButtons() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QObject* bar = window->findChild<QObject*>(QStringLiteral("titleBar"));
    QVERIFY(bar != nullptr);
    auto* minimizeButton = bar->findChild<QQuickItem*>(QStringLiteral("minimizeButton"));
    auto* maximizeButton = bar->findChild<QQuickItem*>(QStringLiteral("maximizeButton"));
    auto* closeButton = bar->findChild<QQuickItem*>(QStringLiteral("closeButton"));
    QVERIFY(minimizeButton != nullptr && maximizeButton != nullptr && closeButton != nullptr);

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(maximizeButton));
    QTRY_VERIFY(window->visibility() == QWindow::Maximized);
    QCOMPARE(maximizeButton->property("maximized").toBool(), true);
    QCOMPARE(maximizeButton->property("tip").toString(), QStringLiteral("Restore"));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(maximizeButton));
    QTRY_VERIFY(window->visibility() != QWindow::Maximized);
    QCOMPARE(maximizeButton->property("maximized").toBool(), false);

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(minimizeButton));
    QTRY_VERIFY(window->visibility() == QWindow::Minimized);
    window->showNormal();
    QTRY_VERIFY(window->visibility() != QWindow::Minimized);

    bar->setProperty("closeOnly", true); // child mode: only Close stays
    QVERIFY(!minimizeButton->isVisible());
    QVERIFY(!maximizeButton->isVisible());
    QVERIFY(closeButton->isVisible());
    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(300, 16));
    QVERIFY(window->visibility() != QWindow::Maximized); // no gesture in close-only

    bar->setProperty("closeOnly", false);
    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(300, 16));
    QTRY_VERIFY(window->visibility() == QWindow::Maximized);

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(closeButton));
    QTRY_VERIFY(!window->isVisible());
}

void TestQmlFoundation::titleBarDrag() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->setPosition(100, 100);

    // The move is delegated to the window manager with startSystemMove(), so
    // the window follows the pointer natively. The offscreen platform used by
    // the tests does not implement it: what is verified here is that the drag
    // handler exists and that QML no longer moves the window by itself (the
    // incremental x/y math that used to make the drag jump).
    QObject* bar = window->findChild<QObject*>(QStringLiteral("titleBar"));
    QVERIFY(bar != nullptr);
    QVERIFY(bar->findChild<QObject*>(QStringLiteral("windowDrag")) != nullptr);

    // The double-click MouseArea must not swallow the drag: the handler still
    // becomes active when the pointer moves with the button down.
    QObject* handler = bar->findChild<QObject*>(QStringLiteral("windowDrag"));
    QVERIFY(handler != nullptr);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(300, 16));
    QTest::mouseMove(window, QPoint(360, 56));
    QTRY_VERIFY(handler->property("active").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(360, 56));
    QTRY_VERIFY(!handler->property("active").toBool());
    // QML no longer moves the window by itself.
    QCOMPARE(window->position(), QPoint(100, 100));

    // A double click on the bar still toggles maximize.
    QCOMPARE(window->visibility(), QQuickWindow::Windowed);
    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(300, 16));
    QTRY_COMPARE(window->visibility(), QQuickWindow::Maximized);
    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(300, 16));
    QTRY_COMPARE(window->visibility(), QQuickWindow::Windowed);
}

void TestQmlFoundation::windowEdgeResize() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->setPosition(100, 100);
    QCOMPARE(QSize(window->width(), window->height()), QSize(1008, 561));

    // Bottom-right corner shrinks both ways (staying above the 960x540 minimum).
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(1005, 558));
    QTest::mouseMove(window, QPoint(995, 548));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(995, 548));
    QCOMPARE(window->width(), 998);
    QCOMPARE(window->height(), 551);

    // Right edge shrinks; left edge stays.
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(995, 300));
    QTest::mouseMove(window, QPoint(975, 300));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(975, 300));
    QCOMPARE(window->width(), 978);
    QCOMPARE(window->x(), 100);

    // Bottom edge shrinks.
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(500, 548));
    QTest::mouseMove(window, QPoint(500, 538));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(500, 538));
    QCOMPARE(window->height(), 541);

    // Left edge cannot shrink below the 960 minimum (x follows the clamp).
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(3, 300));
    QTest::mouseMove(window, QPoint(203, 300));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(203, 300));
    QCOMPARE(window->width(), 960);
    QCOMPARE(window->x(), 100 + (978 - 960));
}


// The zoom buttons of the history and consultation windows: shown only when
// the bar is asked to, left of Close and as tall as the bar, with their
// tooltips in the interface language, a drawn magnifier in the theme colour
// (muted when the window cannot zoom that way) and one request per click.
void TestQmlFoundation::titleBarZoomButtons() {
    QQuickWindow* window = openShell(QStringLiteral("claro"));
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QObject* bar = window->findChild<QObject*>(QStringLiteral("titleBar"));
    QVERIFY(bar != nullptr);
    auto* zoomIn = bar->findChild<QQuickItem*>(QStringLiteral("zoomInButton"));
    auto* zoomOut = bar->findChild<QQuickItem*>(QStringLiteral("zoomOutButton"));
    auto* closeButton = bar->findChild<QQuickItem*>(QStringLiteral("closeButton"));
    QVERIFY(zoomIn != nullptr && zoomOut != nullptr && closeButton != nullptr);
    // The main window's bar has none.
    QVERIFY(!zoomIn->isVisible());
    QVERIFY(!zoomOut->isVisible());

    bar->setProperty("closeOnly", true);
    bar->setProperty("showZoom", true);
    QVERIFY(zoomIn->isVisible() && zoomOut->isVisible());
    const auto left = [](QQuickItem* item) { return item->mapToScene(QPointF()).x(); };
    // The row lays the buttons out on its next polish.
    QTRY_VERIFY(left(zoomOut) < left(zoomIn));
    QVERIFY(left(zoomIn) < left(closeButton));
    for (QQuickItem* button : {zoomIn, zoomOut}) {
        QCOMPARE(button->mapToScene(QPointF()).y(), 0.0);
        QCOMPARE(button->height(), 40.0);
    }
    QVERIFY(zoomIn->findChild<QQuickItem*>(QStringLiteral("magnifierPlus"))->isVisible());
    QVERIFY(!zoomOut->findChild<QQuickItem*>(QStringLiteral("magnifierPlus"))->isVisible());

    // Tooltips in the three languages.
    InterfaceSettings* settings = settingsOf(*m_engine);
    const auto restore = qScopeGuard([settings] { settings->setLanguage(QStringLiteral("en")); });
    const QList<std::tuple<QString, QString, QString>> tips = {
        {QStringLiteral("en"), QStringLiteral("Zoom in"), QStringLiteral("Zoom out")},
        {QStringLiteral("es"), QStringLiteral("Ampliar"), QStringLiteral("Reducir")},
        {QStringLiteral("pt"), QStringLiteral("Ampliar"), QStringLiteral("Reduzir")},
    };
    for (const auto& [language, in, out] : tips) {
        settings->setLanguage(language);
        QCOMPARE(zoomIn->property("tip").toString(), in);
        QCOMPARE(zoomOut->property("tip").toString(), out);
    }
    settings->setLanguage(QStringLiteral("en"));

    // One request per click.
    QSignalSpy inSpy(bar, SIGNAL(zoomInRequested()));
    QSignalSpy outSpy(bar, SIGNAL(zoomOutRequested()));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(zoomIn));
    QTRY_COMPARE(inSpy.count(), 1);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(zoomOut));
    QTRY_COMPARE(outSpy.count(), 1);
    QCOMPARE(inSpy.count(), 1);

    // The theme colour, muted when the window cannot zoom that way.
    const auto lensColour = [](QQuickItem* button) {
        QObject* border = button->findChild<QQuickItem*>(QStringLiteral("magnifierLens"))
                              ->property("border")
                              .value<QObject*>();
        return border->property("color").value<QColor>();
    };
    for (const auto& [mode, text, muted] :
         {std::tuple{"claro", "#1A1A1A", "#A6A6A6"}, std::tuple{"oscuro", "#FFFFFF", "#969696"}}) {
        settings->setTheme(QLatin1String(mode));
        bar->setProperty("canZoomIn", true);
        bar->setProperty("canZoomOut", true);
        QCOMPARE(lensColour(zoomIn), QColor(QLatin1String(text)));
        QCOMPARE(lensColour(zoomOut), QColor(QLatin1String(text)));
        bar->setProperty("canZoomIn", false);
        bar->setProperty("canZoomOut", false);
        QVERIFY(!zoomIn->isEnabled());
        QVERIFY(!zoomOut->isEnabled());
        QCOMPARE(lensColour(zoomIn), QColor(QLatin1String(muted)));
        QCOMPARE(lensColour(zoomOut), QColor(QLatin1String(muted)));
    }
    settings->setTheme(QStringLiteral("claro"));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(zoomIn));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(zoomOut));
    QTest::qWait(50);
    QCOMPARE(inSpy.count(), 1);
    QCOMPARE(outSpy.count(), 1);
}

} // namespace gambasse

int main(int argc, char** argv) {
    // QML needs a GUI application even offscreen. Closing the last window
    // must not quit: several slots close their window on purpose.
    QGuiApplication::setQuitOnLastWindowClosed(false);
    QGuiApplication app(argc, argv);
    gambasse::TestQmlFoundation test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_qmlfoundation.moc"
