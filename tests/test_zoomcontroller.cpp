#include <QGuiApplication>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QWindow>
#include <QtTest>

#include <Paths.h>
#include <ui/InterfaceSettings.h>
#include <ui/controller/ZoomController.h>

namespace gambasse {

// The zoom of one history or consultation window: the letter size it shows for
// the chosen one and its screen, the zoom actions, and where the window is
// placed. The available area is fixed: the offscreen screen is smaller than
// the windows.
class TestZoomController : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void shownLetterSize_data();
    void shownLetterSize();
    void zoomInOutAndReset();
    void zoomInUpToTheMaximum();
    void zoomOutFromASizeHeldBackByTheScreen();
    void windowsShareTheChosenSize();
    void smallerAreaShrinksTheWindow();
    void baseSizeChangeIsAnnounced();
    void roomToZoomInIsAnnounced();
    void firstShowIsCentred();
    void showsAgainWhereItWasLeft();
    void zoomCentresAVisibleWindow();
    void zoomWhileHiddenCentresOnShow();
    void zoomWithoutEffectDoesNotMove();
    void showMovesAWindowThatNoLongerFits();
    void windowLargerThanTheAreaAlignsTopLeft();

private:
    QTemporaryDir m_dir;
    InterfaceSettings* m_settings = nullptr;
};

namespace {
constexpr QSize tallWindow(1020, 608);
constexpr QSize lowWindow(1020, 430);
const QRect wideArea(0, 0, 1920, 1040);
const QRect laptopArea(0, 0, 1366, 728);

// A frameless window over a main window, as the history and consultation
// windows are, with its controller.
struct Harness {
    QWindow main;
    QWindow child;
    ZoomController zoom;

    Harness(InterfaceSettings* settings, QSize base, QRect area) {
        main.setFlags(Qt::FramelessWindowHint);
        main.setGeometry(300, 200, 1008, 561);
        main.show();
        child.setFlags(Qt::FramelessWindowHint);
        child.setTransientParent(&main);
        zoom.setSettings(settings);
        zoom.setWindow(&child);
        zoom.setBaseSize(base);
        zoom.setAvailableArea(area);
    }

    // What FormWindow does: place, then show.
    void present() {
        zoom.placeForShow();
        child.show();
    }

    bool centredOverMain() const {
        const QPoint delta = child.geometry().center() - main.geometry().center();
        return qAbs(delta.x()) <= 1 && qAbs(delta.y()) <= 1;
    }
};
} // namespace

void TestZoomController::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    m_settings = new InterfaceSettings(nullptr, this);
}

void TestZoomController::init() {
    m_settings->setLetterSize(13);
}

void TestZoomController::shownLetterSize_data() {
    QTest::addColumn<QSize>("base");
    QTest::addColumn<QRect>("area");
    QTest::addColumn<int>("chosen");
    QTest::addColumn<int>("shown");
    QTest::addColumn<QSize>("size");

    QTest::newRow("tall, wide area") << tallWindow << wideArea << 26 << 22 << QSize(1727, 1029);
    QTest::newRow("low, wide area") << lowWindow << wideArea << 26 << 24 << QSize(1884, 794);
    QTest::newRow("tall, laptop") << tallWindow << laptopArea << 17 << 15 << QSize(1177, 702);
    QTest::newRow("low, laptop") << lowWindow << laptopArea << 17 << 17 << QSize(1334, 563);
    QTest::newRow("tall, small screen")
        << tallWindow << QRect(0, 0, 1024, 560) << 20 << 13 << tallWindow;
    QTest::newRow("base size") << tallWindow << wideArea << 13 << 13 << tallWindow;
}

void TestZoomController::shownLetterSize() {
    QFETCH(QSize, base);
    QFETCH(QRect, area);
    QFETCH(int, chosen);
    QFETCH(int, shown);
    QFETCH(QSize, size);
    m_settings->setLetterSize(chosen);
    Harness h(m_settings, base, area);
    QCOMPARE(h.zoom.shownLetterSize(), shown);
    QCOMPARE(h.zoom.windowSize(), size);
    QCOMPARE(h.zoom.factor(), shown / 13.0);
    QCOMPARE(m_settings->letterSize(), chosen); // the chosen size is kept
}

void TestZoomController::zoomInOutAndReset() {
    Harness h(m_settings, tallWindow, laptopArea);
    QSignalSpy spy(&h.zoom, &ZoomController::zoomChanged);
    QVERIFY(h.zoom.canZoomIn());
    QVERIFY(!h.zoom.canZoomOut());

    h.zoom.zoomOut(); // already at the base size
    QCOMPARE(m_settings->letterSize(), 13);
    QCOMPARE(spy.count(), 0);

    h.zoom.zoomIn();
    QCOMPARE(h.zoom.shownLetterSize(), 14);
    h.zoom.zoomIn();
    QCOMPARE(h.zoom.shownLetterSize(), 15);
    QCOMPARE(spy.count(), 2);
    QVERIFY(h.zoom.canZoomOut());
    // 16 px would make the window 749 px tall: the screen holds it back.
    QVERIFY(!h.zoom.canZoomIn());
    h.zoom.zoomIn();
    QCOMPARE(h.zoom.shownLetterSize(), 15);
    QCOMPARE(m_settings->letterSize(), 15);

    h.zoom.zoomOut();
    QCOMPARE(h.zoom.shownLetterSize(), 14);
    h.zoom.zoomIn();
    h.zoom.reset();
    QCOMPARE(h.zoom.shownLetterSize(), 13);
    QCOMPARE(m_settings->letterSize(), 13);
    QVERIFY(!h.zoom.canZoomOut());
}

void TestZoomController::zoomInUpToTheMaximum() {
    m_settings->setLetterSize(25);
    Harness h(m_settings, tallWindow, QRect(0, 0, 4000, 3000));
    QVERIFY(h.zoom.canZoomIn());
    h.zoom.zoomIn();
    QCOMPARE(h.zoom.shownLetterSize(), 26);
    QVERIFY(!h.zoom.canZoomIn());
    h.zoom.zoomIn();
    QCOMPARE(m_settings->letterSize(), 26);
}

void TestZoomController::zoomOutFromASizeHeldBackByTheScreen() {
    m_settings->setLetterSize(17);
    Harness h(m_settings, tallWindow, laptopArea);
    QCOMPARE(h.zoom.shownLetterSize(), 15);
    h.zoom.zoomOut();
    QCOMPARE(m_settings->letterSize(), 14);
    QCOMPARE(h.zoom.shownLetterSize(), 14);
}

void TestZoomController::windowsShareTheChosenSize() {
    Harness history(m_settings, tallWindow, wideArea);
    Harness consultation(m_settings, lowWindow, wideArea);
    history.zoom.zoomIn();
    history.zoom.zoomIn();
    QCOMPARE(consultation.zoom.shownLetterSize(), 15);
    consultation.zoom.reset();
    QCOMPARE(history.zoom.shownLetterSize(), 13);
}

void TestZoomController::smallerAreaShrinksTheWindow() {
    m_settings->setLetterSize(17);
    Harness h(m_settings, tallWindow, wideArea);
    QCOMPARE(h.zoom.shownLetterSize(), 17);
    QSignalSpy spy(&h.zoom, &ZoomController::zoomChanged);
    h.zoom.setAvailableArea(laptopArea);
    QCOMPARE(h.zoom.shownLetterSize(), 15);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_settings->letterSize(), 17); // the choice is not lost
    h.zoom.setAvailableArea(wideArea);
    QCOMPARE(h.zoom.shownLetterSize(), 17);
}

// One window shows forms of different heights (the adult history is lower
// than the pediatric one): its size follows the form at the same letter size,
// and whoever binds to it, as the window does with its size limits, is told.
void TestZoomController::baseSizeChangeIsAnnounced() {
    Harness h(m_settings, tallWindow, wideArea);
    QSignalSpy spy(&h.zoom, &ZoomController::zoomChanged);
    h.zoom.setBaseSize(lowWindow);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(h.zoom.windowSize(), lowWindow);
    QCOMPARE(h.zoom.shownLetterSize(), 13);
    h.zoom.setBaseSize(lowWindow); // same size: nothing to tell
    QCOMPARE(spy.count(), 1);

    m_settings->setLetterSize(15);
    QCOMPARE(spy.count(), 2);
    h.zoom.setBaseSize(tallWindow);
    QCOMPARE(spy.count(), 3);
    QCOMPARE(h.zoom.windowSize(), QSize(1177, 702));
}

// The zoom in button follows the room the screen leaves, also when a change
// of that room leaves the size shown as it was.
void TestZoomController::roomToZoomInIsAnnounced() {
    Harness h(m_settings, tallWindow, QRect(0, 0, 1024, 700));
    QVERIFY(!h.zoom.canZoomIn()); // 14 px would be 1099 px wide
    QSignalSpy spy(&h.zoom, &ZoomController::zoomChanged);
    h.zoom.setAvailableArea(wideArea);
    QCOMPARE(h.zoom.shownLetterSize(), 13);
    QCOMPARE(spy.count(), 1);
    QVERIFY(h.zoom.canZoomIn());
    h.zoom.setAvailableArea(QRect(0, 0, 1024, 700));
    QCOMPARE(spy.count(), 2);
    QVERIFY(!h.zoom.canZoomIn());
    // A lower form in the same window has room again.
    h.zoom.setAvailableArea(QRect(0, 0, 1100, 620));
    QVERIFY(!h.zoom.canZoomIn());
    h.zoom.setBaseSize(lowWindow);
    QVERIFY(h.zoom.canZoomIn());
}

void TestZoomController::firstShowIsCentred() {
    m_settings->setLetterSize(15);
    Harness h(m_settings, tallWindow, wideArea);
    h.present();
    QTRY_COMPARE(h.child.size(), QSize(1177, 702));
    QTRY_VERIFY(h.centredOverMain());
}

void TestZoomController::showsAgainWhereItWasLeft() {
    Harness h(m_settings, tallWindow, wideArea);
    h.present();
    QTRY_VERIFY(h.centredOverMain());
    h.child.setPosition(40, 30); // dragged by the user
    QTRY_COMPARE(h.child.position(), QPoint(40, 30));
    h.child.hide();
    h.present();
    QTRY_VERIFY(h.child.isVisible());
    QCOMPARE(h.child.position(), QPoint(40, 30));
}

void TestZoomController::zoomCentresAVisibleWindow() {
    Harness h(m_settings, tallWindow, wideArea);
    h.present();
    h.child.setPosition(40, 30);
    QTRY_COMPARE(h.child.position(), QPoint(40, 30));
    h.zoom.zoomIn();
    QTRY_COMPARE(h.child.geometry().size(), QSize(1099, 655));
    QTRY_VERIFY(h.centredOverMain());
    h.child.setPosition(60, 50); // dragged again afterwards
    QTRY_COMPARE(h.child.position(), QPoint(60, 50));
    h.zoom.zoomOut();
    QTRY_COMPARE(h.child.geometry().size(), tallWindow);
    QTRY_VERIFY(h.centredOverMain());
}

void TestZoomController::zoomWhileHiddenCentresOnShow() {
    Harness consultation(m_settings, lowWindow, wideArea);
    consultation.present();
    consultation.child.setPosition(10, 10);
    QTRY_COMPARE(consultation.child.position(), QPoint(10, 10));
    consultation.child.hide();

    m_settings->setLetterSize(16); // zoom made in a history window
    QCOMPARE(consultation.child.position(), QPoint(10, 10)); // hidden: not yet
    consultation.present();
    QTRY_VERIFY(consultation.centredOverMain());
    QCOMPARE(consultation.child.size(), consultation.zoom.windowSize());
}

void TestZoomController::zoomWithoutEffectDoesNotMove() {
    m_settings->setLetterSize(15);
    Harness h(m_settings, tallWindow, laptopArea);
    h.present();
    h.child.setPosition(100, 20);
    QTRY_COMPARE(h.child.position(), QPoint(100, 20));
    QVERIFY(!h.zoom.canZoomIn());
    h.zoom.zoomIn();
    QTest::qWait(50);
    QCOMPARE(h.child.position(), QPoint(100, 20));
    QCOMPARE(h.child.size(), QSize(1177, 702));
}

void TestZoomController::showMovesAWindowThatNoLongerFits() {
    Harness h(m_settings, lowWindow, wideArea);
    h.present();
    h.child.setPosition(200, 600); // the low window near the bottom edge
    QTRY_COMPARE(h.child.position(), QPoint(200, 600));
    h.child.hide();
    // The same window now holds the taller history, same letter size.
    h.zoom.setBaseSize(tallWindow);
    h.present();
    QTRY_COMPARE(h.child.position(), QPoint(200, 1040 - 608));
}

void TestZoomController::windowLargerThanTheAreaAlignsTopLeft() {
    Harness h(m_settings, tallWindow, QRect(50, 30, 1000, 500));
    h.present();
    QTRY_COMPARE(h.child.geometry(), QRect(QPoint(50, 30), tallWindow));
}

} // namespace gambasse

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    gambasse::TestZoomController test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_zoomcontroller.moc"
