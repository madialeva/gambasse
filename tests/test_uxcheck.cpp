#include <QTest>
#include <QWidget>

#include <UxWidgets/UxCheck.h>

// Verifies the custom check box: readable/writable checked state, theme-safe
// style updates, and read-only mode blocking user toggles while keeping the
// normal look (enabled, no disabled graying).
class TestUxCheck : public QObject {
    Q_OBJECT

private slots:
    void checkedStateRoundTrip();
    void readOnlyBlocksMouseToggle();
    void readOnlyBlocksSpaceToggle();
};

void TestUxCheck::checkedStateRoundTrip() {
    UxCheck check(QStringLiteral("BCG"));
    QVERIFY(!check.isChecked());
    check.setChecked(true);
    QVERIFY(check.isChecked());
    check.setChecked(false);
    QVERIFY(!check.isChecked());
}

void TestUxCheck::readOnlyBlocksMouseToggle() {
    UxCheck check(QStringLiteral("BCG"));
    check.resize(120, 24);
    check.show();
    QVERIFY(QTest::qWaitForWindowExposed(&check));

    check.setReadOnly(true);
    QVERIFY(check.isEnabled());
    QTest::mouseClick(&check, Qt::LeftButton, Qt::NoModifier, QPoint(10, 12));
    QVERIFY(!check.isChecked());

    check.setReadOnly(false);
    QTest::mouseClick(&check, Qt::LeftButton, Qt::NoModifier, QPoint(10, 12));
    QVERIFY(check.isChecked());
}

void TestUxCheck::readOnlyBlocksSpaceToggle() {
    UxCheck check(QStringLiteral("BCG"));
    check.resize(120, 24);
    check.show();
    QVERIFY(QTest::qWaitForWindowExposed(&check));

    check.setReadOnly(true);
    check.setFocus();
    QTest::keyClick(&check, Qt::Key_Space);
    QVERIFY(!check.isChecked());
}

QTEST_MAIN(TestUxCheck)

#include "test_uxcheck.moc"
