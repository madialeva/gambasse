#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest>

#include <Paths.h>
#include <UxWidgets/UxCheck.h>
#include <UxWidgets/UxComboInput.h>
#include <UxWidgets/UxTextInput.h>
#include <data/common/Database.h>
#include <data/model/Patient.h>
#include <ui/window/PediatricHistoryWindow.h>
#include <ui/window/TitleBar.h>

namespace gambasse {
namespace {

QGroupBox* findGroup(PediatricHistoryWindow& window, const QString& title) {
    const QList<QGroupBox*> groups = window.findChildren<QGroupBox*>();
    for (QGroupBox* group : groups) {
        if (group->title() == title)
            return group;
    }
    return nullptr;
}

} // namespace

// Verifies the pediatric history window: specified geometry, close-only title
// bar, new/existing states and the save path through the UI controls.
class TestPediatricHistoryWindow : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    qlonglong m_patientId = 4243;

private slots:
    void initTestCase();
    void cleanupTestCase();

    void geometryMatchesSpecification();
    void titleBarIsCloseOnly();
    void newHistoryDisablesDelete();
    void saveThroughUiThenReloadShowsExisting();
};

void TestPediatricHistoryWindow::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));

    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "INSERT INTO b01_patient (id, name, birth_date, sex, approximate_age) "
        "VALUES (?,?,?,?,?)"));
    q.addBindValue(m_patientId);
    q.addBindValue(QStringLiteral("PHISTORY_WINDOW_KID"));
    q.addBindValue(QStringLiteral("2019-02-03"));
    q.addBindValue(0);
    q.addBindValue(7);
    QVERIFY(q.exec());
}

void TestPediatricHistoryWindow::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestPediatricHistoryWindow::geometryMatchesSpecification() {
    PediatricHistoryWindow window(m_patientId, QStringLiteral("PHISTORY_WINDOW_KID"));
    QVERIFY(window.isValid());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Content 1008x561 plus the 32 px title bar and the 6 px layout margins.
    QCOMPARE(window.width(), 1020);
    QCOMPARE(window.height(), 599);

    // The 7 groups: left column pushed 7 px down with Personal data enlarged
    // to 116 px; right column keeps the specified geometry.
    QList<QRect> expected = {
        QRect(2, 2, 482, 116), QRect(2, 120, 482, 108), QRect(2, 233, 482, 129),
        QRect(2, 368, 482, 174), QRect(492, 2, 514, 117), QRect(492, 121, 514, 173),
        QRect(492, 297, 371, 169),
    };
    QList<QRect> actual;
    for (QGroupBox* group : window.findChildren<QGroupBox*>())
        actual.append(group->geometry());
    QCOMPARE(actual.size(), expected.size());
    for (const QRect& rect : expected)
        QVERIFY2(actual.contains(rect), qPrintable(QString::number(rect.x())));

    // The 3 bottom buttons keep their specified positions and sizes.
    QList<QRect> expectedButtons = {QRect(570, 524, 134, 32), QRect(720, 524, 134, 32),
                                    QRect(871, 524, 134, 32)};
    QList<QRect> actualButtons;
    for (QPushButton* button : window.findChildren<QPushButton*>())
        actualButtons.append(button->geometry());
    QCOMPARE(actualButtons, expectedButtons);
}

void TestPediatricHistoryWindow::titleBarIsCloseOnly() {
    PediatricHistoryWindow window(m_patientId, QStringLiteral("PHISTORY_WINDOW_KID"));
    QVERIFY(window.isValid());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    TitleBar* bar = window.findChild<TitleBar*>();
    QVERIFY(bar != nullptr);
    QVERIFY(bar->isCloseOnly());
    const QList<QToolButton*> buttons = bar->windowButtons();
    QCOMPARE(buttons.size(), 3);
    QVERIFY(!buttons[0]->isVisible());
    QVERIFY(!buttons[1]->isVisible());
    QVERIFY(buttons[2]->isVisible());
    // The custom bar shows the window title, not the application name.
    const QList<QLabel*> labels = bar->findChildren<QLabel*>();
    QCOMPARE(labels.size(), 2);
    QCOMPARE(labels[1]->text(), QStringLiteral("Pediatric History"));
}

void TestPediatricHistoryWindow::newHistoryDisablesDelete() {
    QVERIFY(!Database::instance().hasPediatricHistory(m_patientId));
    PediatricHistoryWindow window(m_patientId, QStringLiteral("PHISTORY_WINDOW_KID"));
    QVERIFY(window.isValid());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Delete/Save/Exit are the only buttons, from left to right.
    const QList<QPushButton*> buttons = window.findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 3);
    QVERIFY(!buttons[0]->isEnabled()); // Delete disabled for a new history
    QVERIFY(buttons[1]->isEnabled());
    QVERIFY(buttons[2]->isEnabled());
}

void TestPediatricHistoryWindow::saveThroughUiThenReloadShowsExisting() {
    {
        PediatricHistoryWindow window(m_patientId, QStringLiteral("PHISTORY_WINDOW_KID"));
        QVERIFY(window.isValid());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        QGroupBox* personal = findGroup(window, QStringLiteral("Personal data"));
        QVERIFY(personal != nullptr);
        const QList<UxTextInput*> texts = personal->findChildren<UxTextInput*>();
        QCOMPARE(texts.size(), 2);
        texts[1]->setText(QStringLiteral("Pollen")); // allergies

        QGroupBox* housing = findGroup(window, QStringLiteral("Housing"));
        QVERIFY(housing != nullptr);
        const QList<UxCheck*> checks = housing->findChildren<UxCheck*>();
        QCOMPARE(checks.size(), 4);
        checks[0]->setChecked(true); // latrine
        UxComboInput* animals = housing->findChild<UxComboInput*>();
        QVERIFY(animals != nullptr);
        animals->comboBox()->setCurrentIndex(animals->comboBox()->findData(2));

        QVERIFY(QMetaObject::invokeMethod(&window, "onSave", Qt::DirectConnection));
        QCOMPARE(window.result(), static_cast<int>(QDialog::Accepted));
    }

    QVERIFY(Database::instance().hasPediatricHistory(m_patientId));
    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "SELECT allergies, latrine, domestic_animals FROM b05_pediatric_history "
        "WHERE patient_id=?"));
    q.addBindValue(m_patientId);
    QVERIFY(q.exec());
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("Pollen"));
    QCOMPARE(q.value(1).toInt(), 1);
    QCOMPARE(q.value(2).toInt(), 2);

    // Reopening shows the existing history with Delete enabled.
    PediatricHistoryWindow reopened(m_patientId, QStringLiteral("PHISTORY_WINDOW_KID"));
    QVERIFY(reopened.isValid());
    reopened.show();
    QVERIFY(QTest::qWaitForWindowExposed(&reopened));
    QGroupBox* personal = findGroup(reopened, QStringLiteral("Personal data"));
    QVERIFY(personal != nullptr);
    const QList<UxTextInput*> texts = personal->findChildren<UxTextInput*>();
    QCOMPARE(texts.size(), 2);
    QCOMPARE(texts[1]->text(), QStringLiteral("Pollen"));
    const QList<QPushButton*> buttons = reopened.findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 3);
    QVERIFY(buttons[0]->isEnabled()); // Delete enabled for an existing history
}

} // namespace gambasse

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    gambasse::TestPediatricHistoryWindow test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_pediatricehistorywindow.moc"
