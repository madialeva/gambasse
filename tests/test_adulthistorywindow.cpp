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
#include <ui/window/AdultHistoryWindow.h>
#include <ui/window/TitleBar.h>

namespace gambasse {
namespace {

QGroupBox* findGroup(AdultHistoryWindow& window, const QString& title) {
    const QList<QGroupBox*> groups = window.findChildren<QGroupBox*>();
    for (QGroupBox* group : groups) {
        if (group->title() == title)
            return group;
    }
    return nullptr;
}

} // namespace

// Verifies the adult history window: specified geometry, close-only title
// bar, new/existing states and the save path through the UI controls.
class TestAdultHistoryWindow : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    qlonglong m_patientId = 4246;

private slots:
    void initTestCase();
    void cleanupTestCase();

    void geometryMatchesSpecification();
    void titleBarIsCloseOnly();
    void newHistoryDisablesDelete();
    void saveThroughUiThenReloadShowsExisting();
};

void TestAdultHistoryWindow::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));

    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "INSERT INTO b01_patient (id, name, birth_date, sex, approximate_age) "
        "VALUES (?,?,?,?,?)"));
    q.addBindValue(m_patientId);
    q.addBindValue(QStringLiteral("AHISTORY_WINDOW_ADULT"));
    q.addBindValue(QStringLiteral("1985-04-12"));
    q.addBindValue(0);
    q.addBindValue(41);
    QVERIFY(q.exec());
}

void TestAdultHistoryWindow::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestAdultHistoryWindow::geometryMatchesSpecification() {
    AdultHistoryWindow window(m_patientId, QStringLiteral("AHISTORY_WINDOW_ADULT"));
    QVERIFY(window.isValid());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Content 1008x383 plus the 32 px title bar and the 6 px layout margins.
    QCOMPARE(window.width(), 1020);
    QCOMPARE(window.height(), 421);

    // The 4 groups keep the specified geometry (left column with the
    // approved 7 px inner padding: taller personal group, adult group
    // pushed 3 px down and 3 px shorter so its bottom stays aligned with
    // the buttons).
    QList<QRect> expected = {
        QRect(2, 2, 482, 112), QRect(2, 120, 482, 260),
        QRect(492, 2, 514, 109), QRect(492, 117, 514, 134),
    };
    QList<QRect> actual;
    for (QGroupBox* group : window.findChildren<QGroupBox*>())
        actual.append(group->geometry());
    QCOMPARE(actual.size(), expected.size());
    for (const QRect& rect : expected)
        QVERIFY2(actual.contains(rect), qPrintable(QString::number(rect.x())));

    // The 3 bottom buttons keep their specified positions and sizes.
    QList<QRect> expectedButtons = {QRect(560, 348, 137, 32), QRect(714, 348, 137, 32),
                                    QRect(867, 348, 137, 32)};
    QList<QPushButton*> buttons = window.findChildren<QPushButton*>();
    // Window buttons are tool buttons; only the 3 bottom buttons are push.
    QCOMPARE(buttons.size(), 3);
    QList<QRect> actualButtons;
    for (QPushButton* button : buttons)
        actualButtons.append(button->geometry());
    QCOMPARE(actualButtons, expectedButtons);
}

void TestAdultHistoryWindow::titleBarIsCloseOnly() {
    AdultHistoryWindow window(m_patientId, QStringLiteral("AHISTORY_WINDOW_ADULT"));
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
    QCOMPARE(labels[1]->text(), QStringLiteral("Adult History"));
}

void TestAdultHistoryWindow::newHistoryDisablesDelete() {
    QVERIFY(!Database::instance().hasAdultHistory(m_patientId));
    AdultHistoryWindow window(m_patientId, QStringLiteral("AHISTORY_WINDOW_ADULT"));
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

void TestAdultHistoryWindow::saveThroughUiThenReloadShowsExisting() {
    {
        AdultHistoryWindow window(m_patientId, QStringLiteral("AHISTORY_WINDOW_ADULT"));
        QVERIFY(window.isValid());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        QGroupBox* personal = findGroup(window, QStringLiteral("Personal data"));
        QVERIFY(personal != nullptr);
        const QList<UxTextInput*> texts = personal->findChildren<UxTextInput*>();
        QCOMPARE(texts.size(), 2);
        texts[1]->setText(QStringLiteral("Aspirin")); // allergies

        QGroupBox* adult = findGroup(window, QStringLiteral("Adult history"));
        QVERIFY(adult != nullptr);
        const QList<UxCheck*> checks = adult->findChildren<UxCheck*>();
        QCOMPARE(checks.size(), 3);
        checks[0]->setChecked(true); // diabetes

        QGroupBox* housing = findGroup(window, QStringLiteral("Housing"));
        QVERIFY(housing != nullptr);
        UxComboInput* animals = housing->findChild<UxComboInput*>();
        QVERIFY(animals != nullptr);
        animals->comboBox()->setCurrentIndex(animals->comboBox()->findData(1));

        QVERIFY(QMetaObject::invokeMethod(&window, "onSave", Qt::DirectConnection));
        QCOMPARE(window.result(), static_cast<int>(QDialog::Accepted));
    }

    QVERIFY(Database::instance().hasAdultHistory(m_patientId));
    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "SELECT allergies, history_diabetes, housing_domestic_animals "
        "FROM b03_adult_history WHERE patient_id=?"));
    q.addBindValue(m_patientId);
    QVERIFY(q.exec());
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("Aspirin"));
    QCOMPARE(q.value(1).toInt(), 1);
    QCOMPARE(q.value(2).toInt(), 1);

    // Reopening shows the existing history with Delete enabled.
    AdultHistoryWindow reopened(m_patientId, QStringLiteral("AHISTORY_WINDOW_ADULT"));
    QVERIFY(reopened.isValid());
    reopened.show();
    QVERIFY(QTest::qWaitForWindowExposed(&reopened));
    QGroupBox* personal = findGroup(reopened, QStringLiteral("Personal data"));
    QVERIFY(personal != nullptr);
    const QList<UxTextInput*> texts = personal->findChildren<UxTextInput*>();
    QCOMPARE(texts.size(), 2);
    QCOMPARE(texts[1]->text(), QStringLiteral("Aspirin"));
    const QList<QPushButton*> buttons = reopened.findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 3);
    QVERIFY(buttons[0]->isEnabled()); // Delete enabled for an existing history
}

} // namespace gambasse

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    gambasse::TestAdultHistoryWindow test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_adulthistorywindow.moc"
