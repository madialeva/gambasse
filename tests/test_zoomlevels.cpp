#include <QCoreApplication>
#include <QSize>
#include <QtTest>

#include <logic/ZoomLevels.h>

namespace gambasse {

// The zoom rules of the history and consultation windows, with the numbers of
// their specification.
class TestZoomLevels : public QObject {
    Q_OBJECT

private slots:
    void clampsToTheRange();
    void factorOfALetterSize();
    void windowSizeRoundsUp();
    void fittingLetterSize_data();
    void fittingLetterSize();
};

namespace {
// The pediatric history (1008x561 of content) and the adult history (1008x383),
// with the 41 px title bar and the 6 px frame.
constexpr QSize tallWindow(1020, 608);
constexpr QSize lowWindow(1020, 430);
} // namespace

void TestZoomLevels::clampsToTheRange() {
    QCOMPARE(zoom::clampLetterSize(5), 13);
    QCOMPARE(zoom::clampLetterSize(13), 13);
    QCOMPARE(zoom::clampLetterSize(17), 17);
    QCOMPARE(zoom::clampLetterSize(26), 26);
    QCOMPARE(zoom::clampLetterSize(40), 26);
}

void TestZoomLevels::factorOfALetterSize() {
    QCOMPARE(zoom::factor(13), 1.0);
    QCOMPARE(zoom::factor(26), 2.0);
    QCOMPARE(zoom::factor(15), 15.0 / 13.0);
    // Out of range sizes scale like the nearest end.
    QCOMPARE(zoom::factor(40), 2.0);
}

void TestZoomLevels::windowSizeRoundsUp() {
    QCOMPARE(zoom::windowSize(tallWindow, 13), tallWindow);
    QCOMPARE(zoom::windowSize(tallWindow, 15), QSize(1177, 702));
    QCOMPARE(zoom::windowSize(tallWindow, 16), QSize(1256, 749));
    QCOMPARE(zoom::windowSize(lowWindow, 17), QSize(1334, 563));
    QCOMPARE(zoom::windowSize(tallWindow, 26), QSize(2040, 1216));
}

void TestZoomLevels::fittingLetterSize_data() {
    QTest::addColumn<QSize>("base");
    QTest::addColumn<int>("wanted");
    QTest::addColumn<QSize>("available");
    QTest::addColumn<int>("shown");

    QTest::newRow("tall in 1366x728") << tallWindow << 26 << QSize(1366, 728) << 15;
    QTest::newRow("low in 1366x728") << lowWindow << 26 << QSize(1366, 728) << 17;
    QTest::newRow("tall in 1920x1040") << tallWindow << 26 << QSize(1920, 1040) << 22;
    QTest::newRow("smaller than the base") << tallWindow << 20 << QSize(1024, 560) << 13;
    QTest::newRow("wanted fits") << tallWindow << 17 << QSize(1920, 1040) << 17;
    QTest::newRow("wanted below the base") << tallWindow << 5 << QSize(1920, 1040) << 13;
    QTest::newRow("wanted above the maximum") << tallWindow << 40 << QSize(4000, 4000) << 26;
    QTest::newRow("exact fit") << tallWindow << 26 << QSize(1177, 702) << 15;
    QTest::newRow("one pixel short") << tallWindow << 26 << QSize(1177, 701) << 14;
}

void TestZoomLevels::fittingLetterSize() {
    QFETCH(QSize, base);
    QFETCH(int, wanted);
    QFETCH(QSize, available);
    QFETCH(int, shown);
    QCOMPARE(zoom::fittingLetterSize(base, wanted, available), shown);
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestZoomLevels test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_zoomlevels.moc"
