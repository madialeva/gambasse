#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <ui/AppIcon.h>
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

private slots:
    void initTestCase();
    void themePersistsAndNotifies();
    void languagePersistsAndNotifies();
    void qmlShellFollowsThemeInHot();
    void qmlLanguageSwitchesInHot();
    void splashSequence();
    void appOpensShellAfterSplash();
    void applicationIconIsTheLogo();
    void titleBarChrome();
    void titleBarStandsOut();
    void titleBarWindowButtons();
    void titleBarDrag();
    void windowEdgeResize();
};

QQuickWindow* TestQmlFoundation::openShell(const QString& theme) {
    cleanup();
    persistInterface(theme, QStringLiteral("en"));
    m_engine = new QQmlApplicationEngine();
    m_engine->loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("Root"));
    if (m_engine->rootObjects().isEmpty())
        return nullptr;
    return qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
}

void TestQmlFoundation::cleanup() {
    delete m_engine;
    m_engine = nullptr;
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
    QCOMPARE(item->property("probed").toString(), QStringLiteral("En Estabulo"));

    // Every change reapplies the catalog and retranslates the engine itself.
    settings->setLanguage(QStringLiteral("es"));
    QCOMPARE(item->property("probed").toString(), QStringLiteral("En establo"));
    settings->setLanguage(QStringLiteral("en")); // back to the source language
    QCOMPARE(item->property("probed").toString(), QStringLiteral("In stable"));

    InterfaceSettings reloaded(nullptr); // persistence rode along
    QCOMPARE(reloaded.language(), QStringLiteral("en"));
    delete item;
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
        for (const char* name : {"languageButton", "themeButton", "minimizeButton",
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
        QCOMPARE(maximizeButton->property("glyph").toString(), QStringLiteral("\u25A1"));
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
    QCOMPARE(maximizeButton->property("glyph").toString(), QStringLiteral("\u2750"));
    QCOMPARE(maximizeButton->property("tip").toString(), QStringLiteral("Restore"));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, itemCenter(maximizeButton));
    QTRY_VERIFY(window->visibility() != QWindow::Maximized);
    QCOMPARE(maximizeButton->property("glyph").toString(), QStringLiteral("\u25A1"));

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
