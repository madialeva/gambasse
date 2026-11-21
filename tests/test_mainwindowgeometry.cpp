#include <QCoreApplication>
#include <QtTest>

#include <logic/MainWindowGeometry.h>

namespace gambasse {

// Pure geometry rules of the main patient window: base and scaled minimum
// sizes, elastic normal size fitting the available area, centring and clamping.
class TestMainWindowGeometry : public QObject {
    Q_OBJECT

private slots:
    void baseAndMinimumSizes();
    void clampNormalSize();
    void effectiveNormalSize_data();
    void effectiveNormalSize();
    void centredRect();
    void clampToArea();
};

void TestMainWindowGeometry::baseAndMinimumSizes() {
    QCOMPARE(mainwindow::baseSize(), QSize(1008, 561));
    QCOMPARE(mainwindow::minimumWindowSize(13), QSize(1008, 561));
    QCOMPARE(mainwindow::minimumWindowSize(15), QSize(1164, 648));
    QCOMPARE(mainwindow::minimumWindowSize(16), QSize(1241, 691));
    QCOMPARE(mainwindow::minimumWindowSize(26), QSize(2016, 1122));
    QCOMPARE(mainwindow::minimumWindowSize(5), QSize(1008, 561));
    QCOMPARE(mainwindow::minimumWindowSize(40), QSize(2016, 1122));
}

void TestMainWindowGeometry::clampNormalSize() {
    QCOMPARE(mainwindow::clampNormalSize(QSize(800, 400)), QSize(1008, 561));
    QCOMPARE(mainwindow::clampNormalSize(QSize(1200, 400)), QSize(1200, 561));
    QCOMPARE(mainwindow::clampNormalSize(QSize(900, 700)), QSize(1008, 700));
    QCOMPARE(mainwindow::clampNormalSize(QSize(1280, 720)), QSize(1280, 720));
}

void TestMainWindowGeometry::effectiveNormalSize_data() {
    QTest::addColumn<QSize>("preferred");
    QTest::addColumn<int>("letterSize");
    QTest::addColumn<QSize>("available");
    QTest::addColumn<QSize>("expected");

    const QSize wide(1920, 1040);
    const QSize laptop(1366, 728);
    const QSize small(1024, 560);

    QTest::newRow("preferred larger than minimum")
        << QSize(1400, 800) << 15 << wide << QSize(1400, 800);
    QTest::newRow("preferred smaller than scaled minimum")
        << QSize(1008, 561) << 16 << wide << QSize(1241, 691);
    QTest::newRow("width grows while height stays preferred")
        << QSize(1200, 720) << 16 << wide << QSize(1241, 720);
    QTest::newRow("preferred larger than laptop screen")
        << QSize(1600, 900) << 13 << laptop << QSize(1366, 728);
    QTest::newRow("screen smaller than base height keeps base height")
        << QSize(1200, 700) << 13 << small << QSize(1024, 561);
    QTest::newRow("invalid area keeps clamped preferred or minimum")
        << QSize(1008, 561) << 15 << QSize() << QSize(1164, 648);
}

void TestMainWindowGeometry::effectiveNormalSize() {
    QFETCH(QSize, preferred);
    QFETCH(int, letterSize);
    QFETCH(QSize, available);
    QFETCH(QSize, expected);
    QCOMPARE(mainwindow::effectiveNormalSize(preferred, letterSize, available), expected);
}

void TestMainWindowGeometry::centredRect() {
    const QRect area(80, 40, 1840, 1000);
    const QRect rect = mainwindow::centredRect(QSize(1280, 720), area);
    QCOMPARE(rect.size(), QSize(1280, 720));
    const QPoint delta = rect.center() - area.center();
    QVERIFY(qAbs(delta.x()) <= 1 && qAbs(delta.y()) <= 1);

    // Larger than the available area: aligned to its top-left corner.
    const QRect smallArea(50, 30, 1000, 500);
    QCOMPARE(mainwindow::centredRect(QSize(1008, 561), smallArea), QRect(50, 30, 1008, 561));
}

void TestMainWindowGeometry::clampToArea() {
    const QRect area(100, 40, 1820, 1000);
    // Already inside: unchanged.
    QCOMPARE(mainwindow::clampToArea(QRect(200, 120, 1280, 720), area),
             QRect(200, 120, 1280, 720));
    // Overflows right and bottom: pushed back just enough.
    QCOMPARE(mainwindow::clampToArea(QRect(800, 500, 1280, 720), area),
             QRect(100 + 1820 - 1280, 40 + 1000 - 720, 1280, 720));
    // Overflows left and top: aligned to area origin.
    QCOMPARE(mainwindow::clampToArea(QRect(10, 10, 1008, 561), area), QRect(100, 40, 1008, 561));
    // Larger than area: aligned to area origin.
    QCOMPARE(mainwindow::clampToArea(QRect(200, 200, 1008, 561), QRect(50, 30, 800, 500)),
             QRect(50, 30, 1008, 561));
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestMainWindowGeometry test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mainwindowgeometry.moc"
