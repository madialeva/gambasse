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

} // namespace

// Verifies the QML foundation wiring: theme/language persistence through
// InterfaceSettings, change signals, and hot QML updates end to end
// (bridge flip -> Theme bindings -> rendered pixels).
class TestQmlFoundation : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QQmlApplicationEngine* m_engine = nullptr;
    InterfaceSettings* m_settings = nullptr;

    // Fresh shell like main.cpp wires it (module load + settings injection).
    // Null when the module fails to load; slots check it.
    QQuickWindow* openShell(const QString& theme);
    void cleanup();

private slots:
    void initTestCase();
    void themePersistsAndNotifies();
    void languagePersistsAndNotifies();
    void qmlShellFollowsThemeInHot();
    void qmlLanguageSwitchesInHot();
    void splashSequence();
    void titleBarChrome();
    void titleBarWindowButtons();
    void titleBarDrag();
    void windowEdgeResize();
};

QQuickWindow* TestQmlFoundation::openShell(const QString& theme) {
    cleanup();
    m_settings = new InterfaceSettings();
    m_settings->setTheme(theme);
    m_engine = new QQmlApplicationEngine();
    m_engine->loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("Root"));
    if (m_engine->rootObjects().isEmpty())
        return nullptr;
    auto* window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    if (window == nullptr)
        return nullptr;
    window->setProperty("uiSettings", QVariant::fromValue(qobject_cast<QObject*>(m_settings)));
    return window;
}

void TestQmlFoundation::cleanup() {
    delete m_engine;
    m_engine = nullptr;
    delete m_settings;
    m_settings = nullptr;
}

void TestQmlFoundation::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
}

void TestQmlFoundation::themePersistsAndNotifies() {
    InterfaceSettings settings;
    QCOMPARE(settings.theme(), QStringLiteral("claro")); // config.ini default

    QSignalSpy spy(&settings, &InterfaceSettings::themeChanged);
    settings.setTheme(QStringLiteral("oscuro"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(settings.theme(), QStringLiteral("oscuro"));

    settings.setTheme(QStringLiteral("oscuro")); // same value: no signal
    QCOMPARE(spy.count(), 1);

    InterfaceSettings reloaded; // persistence round trip
    QCOMPARE(reloaded.theme(), QStringLiteral("oscuro"));

    settings.setTheme(QStringLiteral("whatever")); // normalized to claro
    QCOMPARE(settings.theme(), QStringLiteral("claro"));
}

void TestQmlFoundation::languagePersistsAndNotifies() {
    InterfaceSettings settings;
    QCOMPARE(settings.language(), QStringLiteral("pt")); // config.ini default

    QSignalSpy spy(&settings, &InterfaceSettings::languageChanged);
    settings.setLanguage(QStringLiteral("es"));
    QCOMPARE(spy.count(), 1);

    settings.setLanguage(QStringLiteral("es")); // same value: no signal
    QCOMPARE(spy.count(), 1);

    InterfaceSettings reloaded; // persistence round trip
    QCOMPARE(reloaded.language(), QStringLiteral("es"));
}

void TestQmlFoundation::qmlShellFollowsThemeInHot() {
    InterfaceSettings settings;
    settings.setTheme(QStringLiteral("claro"));

    QQmlApplicationEngine engine;
    // Same loading as main.cpp: the module (and its resources) comes from
    // the shared GambasseQml library in both binaries.
    engine.loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("Root"));
    QVERIFY(!engine.rootObjects().isEmpty());

    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(window != nullptr);
    // Same injection as main.cpp: drives Theme.mode (see Root.qml).
    window->setProperty("uiSettings", QVariant::fromValue(qobject_cast<QObject*>(&settings)));
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QCOMPARE(grabCenter(window), expectedShell(QStringLiteral("claro")));

    settings.setTheme(QStringLiteral("oscuro")); // hot change, no reload
    QTRY_COMPARE(grabCenter(window), expectedShell(QStringLiteral("oscuro")));
}

void TestQmlFoundation::qmlLanguageSwitchesInHot() {
    InterfaceSettings settings;
    QQmlApplicationEngine engine;
    // Same wiring as main.cpp: every language change reapplies the catalog
    // and refreshes all QML-bound strings.
    QObject::connect(&settings, &InterfaceSettings::languageChanged,
                     [&]() { settings.applyLanguage(&engine); });

    // Deterministic baseline whatever previous tests persisted.
    settings.setLanguage(QStringLiteral("pt"));
    settings.applyLanguage(&engine);

    QQmlComponent probe(
        &engine, QUrl::fromLocalFile(QStringLiteral(QML_FIXTURE_DIR "/LangProbe.qml")));
    QObject* item = probe.create();
    QVERIFY(item != nullptr);
    QCOMPARE(item->property("probed").toString(), QStringLiteral("En Estabulo"));

    settings.setLanguage(QStringLiteral("es"));
    QCOMPARE(item->property("probed").toString(), QStringLiteral("En establo"));
    settings.setLanguage(QStringLiteral("en")); // back to the source language
    QCOMPARE(item->property("probed").toString(), QStringLiteral("In stable"));

    InterfaceSettings reloaded; // persistence rode along
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

void TestQmlFoundation::titleBarChrome() {
    // Same assertions in both themes (tooltips stay in English: no
    // translator is installed in tests, like a fresh English session).
    for (const QString& mode : {QStringLiteral("claro"), QStringLiteral("oscuro")}) {
        QQuickWindow* window = openShell(mode);
        QVERIFY(window != nullptr);
        QVERIFY(QTest::qWaitForWindowExposed(window));

        QObject* bar = window->findChild<QObject*>(QStringLiteral("titleBar"));
        QVERIFY(bar != nullptr);
        QCOMPARE(bar->property("height").toInt(), 32);

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

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(300, 16));
    QTest::mouseMove(window, QPoint(360, 56));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(360, 56));
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
