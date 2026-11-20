#include <QGuiApplication>
#include <QScreen>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QWindow>
#include <QtTest>

#include <Paths.h>
#include <logic/AppSettings.h>
#include <ui/InterfaceSettings.h>
#include <ui/controller/ShellWindowController.h>
#include <ui/controller/ZoomController.h>

namespace gambasse {

// Main shell window controller: startup maximized by default, persisted
// state and normal size, elastic zoom limits and response, and shared
// letter-size sync with ZoomController.
class TestShellWindowController : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void defaultStartupIsMaximized();
    void normalStartupIsCentredAndClampedWithoutOverwriting();
    void restoreResizeMaximizeAndMinimizePersistence();
    void zoomLimitsPerScreenArea();
    void elasticZoomInMaximizedAndNormalModes();
    void shrinkingAreaReducesShownLetterSize();
    void sharesLetterSizeWithZoomController();

private:
    QTemporaryDir m_dir;
    InterfaceSettings* m_settings = nullptr;

    void clearConfig();
};

namespace {
const QRect wideArea(0, 0, 1920, 1040);
const QRect laptopArea(0, 0, 1366, 728);
const QRect tinyArea(0, 0, 1024, 560);

struct Harness {
    QWindow window;
    ShellWindowController controller;

    Harness(InterfaceSettings* settings, QRect area) {
        window.setFlags(Qt::FramelessWindowHint);
        controller.setSettings(settings);
        controller.setWindow(&window);
        controller.setAvailableArea(area);
    }

    bool centredInArea() const {
        const QPoint delta = window.geometry().center() - controller.availableArea().center();
        return qAbs(delta.x()) <= 1 && qAbs(delta.y()) <= 1;
    }
};
} // namespace

void TestShellWindowController::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    m_settings = new InterfaceSettings(nullptr, this);
}

void TestShellWindowController::clearConfig() {
    QFile::remove(QDir(m_dir.path()).filePath(QStringLiteral("config.ini")));
}

void TestShellWindowController::init() {
    clearConfig();
    m_settings->setLetterSize(13);
}

void TestShellWindowController::defaultStartupIsMaximized() {
    clearConfig();
    Harness h(m_settings, wideArea);
    h.controller.applyInitialGeometry();
    QTRY_COMPARE(h.window.visibility(), QWindow::Maximized);
    QCOMPARE(h.controller.maximized(), true);
    QCOMPARE(h.controller.shownLetterSize(), 13);
    QCOMPARE(h.controller.minimumSize(), QSize(1008, 561));
}

void TestShellWindowController::normalStartupIsCentredAndClampedWithoutOverwriting() {
    AppSettings().setMainWindowGeometry({false, QSize(1280, 720)});
    {
        Harness h(m_settings, wideArea);
        h.controller.applyInitialGeometry();
        QTRY_COMPARE(h.window.visibility(), QWindow::Windowed);
        QCOMPARE(h.window.size(), QSize(1280, 720));
        QVERIFY(h.centredInArea());
        QCOMPARE(h.controller.maximized(), false);
    }

    // Saved size larger than laptop screen: clamped on screen without
    // overwriting the user's preferred size in config.ini.
    AppSettings().setMainWindowGeometry({false, QSize(1600, 900)});
    {
        Harness h(m_settings, laptopArea);
        h.controller.applyInitialGeometry();
        QTRY_COMPARE(h.window.visibility(), QWindow::Windowed);
        QCOMPARE(h.window.size(), QSize(1366, 728));
        QVERIFY(h.centredInArea());
        QTest::qWait(200);
        h.window.close();
    }
    QCOMPARE(AppSettings().mainWindowGeometry(), (MainWindowGeometry{false, QSize(1600, 900)}));
}

void TestShellWindowController::restoreResizeMaximizeAndMinimizePersistence() {
    clearConfig();
    Harness h(m_settings, wideArea);
    h.controller.applyInitialGeometry();
    QTRY_COMPARE(h.window.visibility(), QWindow::Maximized);

    // Restoring from first-run maximized centres at 1008x561 and persists normal.
    h.window.showNormal();
    QTRY_COMPARE(h.window.visibility(), QWindow::Windowed);
    QTRY_COMPARE(h.window.size(), QSize(1008, 561));
    QVERIFY(h.centredInArea());
    QCOMPARE(AppSettings().mainWindowGeometry(), (MainWindowGeometry{false, QSize(1008, 561)}));

    // User resizes to 1280x720: persisted after debounce / flush.
    h.window.resize(1280, 720);
    QTRY_COMPARE(h.window.size(), QSize(1280, 720));
    h.controller.flushPendingSave();
    QCOMPARE(AppSettings().mainWindowGeometry(), (MainWindowGeometry{false, QSize(1280, 720)}));

    // Minimizing to the taskbar while normal does not overwrite state.
    h.window.showMinimized();
    QTRY_COMPARE(h.window.visibility(), QWindow::Minimized);
    QCOMPARE(AppSettings().mainWindowGeometry(), (MainWindowGeometry{false, QSize(1280, 720)}));
    h.window.showNormal();
    QTRY_COMPARE(h.window.visibility(), QWindow::Windowed);
    QCOMPARE(AppSettings().mainWindowGeometry(), (MainWindowGeometry{false, QSize(1280, 720)}));

    // Maximizing again persists maximized while keeping 1280x720 as normal size.
    h.window.showMaximized();
    QTRY_COMPARE(h.window.visibility(), QWindow::Maximized);
    QCOMPARE(AppSettings().mainWindowGeometry(), (MainWindowGeometry{true, QSize(1280, 720)}));

    // Minimizing while maximized keeps maximized persisted.
    h.window.showMinimized();
    QTRY_COMPARE(h.window.visibility(), QWindow::Minimized);
    QCOMPARE(AppSettings().mainWindowGeometry(), (MainWindowGeometry{true, QSize(1280, 720)}));

    // Restoring from maximized returns to 1280x720.
    h.window.showMaximized();
    QTRY_COMPARE(h.window.visibility(), QWindow::Maximized);
    h.window.showNormal();
    QTRY_COMPARE(h.window.visibility(), QWindow::Windowed);
    QTRY_COMPARE(h.window.size(), QSize(1280, 720));
    QCOMPARE(AppSettings().mainWindowGeometry(), (MainWindowGeometry{false, QSize(1280, 720)}));
}

void TestShellWindowController::zoomLimitsPerScreenArea() {
    // Laptop 1366x728: 16 px gives 1241x691 (fits); 17 px gives 1319x734 (too tall).
    m_settings->setLetterSize(20);
    {
        Harness h(m_settings, laptopArea);
        QCOMPARE(h.controller.shownLetterSize(), 16);
        QCOMPARE(h.controller.minimumSize(), QSize(1241, 691));
        QVERIFY(!h.controller.canZoomIn());
        QVERIFY(h.controller.canZoomOut());
        // Zooming out from a size held back by the screen drops chosen size from 16 to 15.
        h.controller.zoomOut();
        QCOMPARE(m_settings->letterSize(), 15);
        QCOMPARE(h.controller.shownLetterSize(), 15);
        QVERIFY(h.controller.canZoomIn());
    }

    // 1920x1040: 24 px gives 1861x1036 (fits); 25 px gives 1939x1079 (too large).
    m_settings->setLetterSize(26);
    {
        Harness h(m_settings, wideArea);
        QCOMPARE(h.controller.shownLetterSize(), 24);
        QCOMPARE(h.controller.minimumSize(), QSize(1861, 1036));
        QVERIFY(!h.controller.canZoomIn());
    }

    // Large screen reaches 26 px.
    {
        Harness h(m_settings, QRect(0, 0, 4000, 3000));
        QCOMPARE(h.controller.shownLetterSize(), 26);
        QCOMPARE(h.controller.minimumSize(), QSize(2016, 1122));
        QVERIFY(!h.controller.canZoomIn());
    }

    // Tiny screen stays at 13 px.
    {
        Harness h(m_settings, tinyArea);
        QCOMPARE(h.controller.shownLetterSize(), 13);
        QCOMPARE(h.controller.minimumSize(), QSize(1008, 561));
        QVERIFY(!h.controller.canZoomIn());
        QVERIFY(!h.controller.canZoomOut());
    }
}

void TestShellWindowController::elasticZoomInMaximizedAndNormalModes() {
    // Maximized stays maximized when zooming.
    {
        clearConfig();
        Harness h(m_settings, wideArea);
        h.controller.applyInitialGeometry();
        QTRY_COMPARE(h.window.visibility(), QWindow::Maximized);
        m_settings->setLetterSize(18);
        QCOMPARE(h.controller.shownLetterSize(), 18);
        QCOMPARE(h.window.visibility(), QWindow::Maximized);
        m_settings->setLetterSize(13);
    }

    // Normal 1400x800: zooming to 15 px (min 1164x648) keeps 1400x800.
    AppSettings().setMainWindowGeometry({false, QSize(1400, 800)});
    {
        Harness h(m_settings, wideArea);
        h.controller.applyInitialGeometry();
        QTRY_COMPARE(h.window.size(), QSize(1400, 800));
        h.controller.zoomIn();
        h.controller.zoomIn();
        QCOMPARE(h.controller.shownLetterSize(), 15);
        QCOMPARE(h.window.size(), QSize(1400, 800));
        h.controller.reset();
        QCOMPARE(h.window.size(), QSize(1400, 800));
    }

    // Normal 1008x561: zooming to 16 px grows to 1241x691 without overwriting
    // config.ini, and reset() returns to 1008x561.
    AppSettings().setMainWindowGeometry({false, QSize(1008, 561)});
    {
        Harness h(m_settings, wideArea);
        h.controller.applyInitialGeometry();
        QTRY_COMPARE(h.window.size(), QSize(1008, 561));
        // Near bottom-right corner so growth must clamp back inside wideArea.
        h.window.setPosition(900, 460);
        QTRY_COMPARE(h.window.position(), QPoint(900, 460));

        m_settings->setLetterSize(16);
        QTRY_COMPARE(h.window.size(), QSize(1241, 691));
        QCOMPARE(h.window.minimumSize(), QSize(1241, 691));
        QVERIFY(wideArea.contains(h.window.geometry()));
        QTest::qWait(200);
        // Preferred size in config.ini was NOT overwritten by zoom growth.
        QCOMPARE(AppSettings().mainWindowGeometry(), (MainWindowGeometry{false, QSize(1008, 561)}));

        h.controller.reset();
        QTRY_COMPARE(h.window.size(), QSize(1008, 561));
        QCOMPARE(h.window.minimumSize(), QSize(1008, 561));
    }
}

void TestShellWindowController::shrinkingAreaReducesShownLetterSize() {
    m_settings->setLetterSize(18);
    AppSettings().setMainWindowGeometry({false, QSize(1400, 800)});
    Harness h(m_settings, wideArea);
    h.controller.applyInitialGeometry();
    QCOMPARE(h.controller.shownLetterSize(), 18);
    QSignalSpy spy(&h.controller, &ShellWindowController::zoomChanged);

    h.controller.setAvailableArea(laptopArea);
    QCOMPARE(h.controller.shownLetterSize(), 16);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_settings->letterSize(), 18); // user choice preserved
    QVERIFY(laptopArea.contains(h.window.geometry()));

    h.controller.setAvailableArea(wideArea);
    QCOMPARE(h.controller.shownLetterSize(), 18);
}

void TestShellWindowController::sharesLetterSizeWithZoomController() {
    Harness shell(m_settings, wideArea);
    QWindow child;
    child.setFlags(Qt::FramelessWindowHint);
    ZoomController childZoom;
    childZoom.setSettings(m_settings);
    childZoom.setWindow(&child);
    childZoom.setBaseSize(QSize(1020, 608));
    childZoom.setAvailableArea(wideArea);

    shell.controller.zoomIn();
    shell.controller.zoomIn();
    QCOMPARE(shell.controller.shownLetterSize(), 15);
    QCOMPARE(childZoom.shownLetterSize(), 15);

    childZoom.zoomIn();
    QCOMPARE(shell.controller.shownLetterSize(), 16);
    QCOMPARE(childZoom.shownLetterSize(), 16);

    shell.controller.reset();
    QCOMPARE(childZoom.shownLetterSize(), 13);
}

} // namespace gambasse

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    gambasse::TestShellWindowController test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_shellwindowcontroller.moc"
