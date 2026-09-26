#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSqlQuery>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest>

#include <Paths.h>
#include <UxWidgets/UxCheck.h>
#include <UxWidgets/UxComboInput.h>
#include <UxWidgets/UxTextInput.h>
#include <data/common/Database.h>
#include <data/model/Patient.h>
#include <ui/window/PregnancyHistoryWindow.h>
#include <ui/window/TitleBar.h>

namespace gambasse {
namespace {

QGroupBox* findGroup(PregnancyHistoryWindow& window, const QString& title) {
    const QList<QGroupBox*> groups = window.findChildren<QGroupBox*>();
    for (QGroupBox* group : groups) {
        if (group->title() == title)
            return group;
    }
    return nullptr;
}

} // namespace

// Verifies the pregnancy history window: specified geometry, close-only title
// bar, treatment tabs, new/existing states and the save path through the UI.
class TestPregnancyHistoryWindow : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    qlonglong m_patientId = 4247;

private slots:
    void initTestCase();
    void cleanupTestCase();

    void geometryMatchesSpecification();
    void titleBarIsCloseOnly();
    void treatmentTabsPresent();
    void newHistoryDisablesDelete();
    void saveThroughUiThenReloadShowsExisting();
};

void TestPregnancyHistoryWindow::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));

    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "INSERT INTO b01_patient (id, name, birth_date, sex, approximate_age) "
        "VALUES (?,?,?,?,?)"));
    q.addBindValue(m_patientId);
    q.addBindValue(QStringLiteral("PHISTORY_WINDOW_PREGNANT"));
    q.addBindValue(QStringLiteral("2001-07-19"));
    q.addBindValue(1);
    q.addBindValue(25);
    QVERIFY(q.exec());
}

void TestPregnancyHistoryWindow::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestPregnancyHistoryWindow::geometryMatchesSpecification() {
    PregnancyHistoryWindow window(m_patientId, QStringLiteral("PHISTORY_WINDOW_PREGNANT"));
    QVERIFY(window.isValid());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Content 1008x561 plus the 32 px title bar and the 6 px layout margins.
    QCOMPARE(window.width(), 1020);
    QCOMPARE(window.height(), 599);

    // The 6 main groups plus the 3 nested examination subgroups.
    QList<QRect> expected = {
        QRect(2, 2, 482, 109), QRect(2, 113, 482, 181), QRect(2, 295, 482, 263),
        QRect(492, 2, 514, 87), QRect(492, 95, 514, 126), QRect(492, 223, 514, 295),
        QRect(208, 11, 122, 66), QRect(337, 11, 97, 66), QRect(440, 11, 68, 83),
    };
    QList<QRect> actual;
    for (QGroupBox* group : window.findChildren<QGroupBox*>())
        actual.append(group->geometry());
    QCOMPARE(actual.size(), expected.size());
    for (const QRect& rect : expected)
        QVERIFY2(actual.contains(rect), qPrintable(QString::number(rect.x())));

    // The 3 bottom buttons keep their specified positions and sizes.
    QList<QRect> expectedButtons = {QRect(565, 524, 137, 32), QRect(716, 524, 137, 32),
                                    QRect(867, 524, 137, 32)};
    QList<QPushButton*> buttons = window.findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 3);
    QList<QRect> actualButtons;
    for (QPushButton* button : buttons)
        actualButtons.append(button->geometry());
    QCOMPARE(actualButtons, expectedButtons);
}

void TestPregnancyHistoryWindow::titleBarIsCloseOnly() {
    PregnancyHistoryWindow window(m_patientId, QStringLiteral("PHISTORY_WINDOW_PREGNANT"));
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
    QCOMPARE(labels[1]->text(), QStringLiteral("Pregnancy History"));
}

void TestPregnancyHistoryWindow::treatmentTabsPresent() {
    PregnancyHistoryWindow window(m_patientId, QStringLiteral("PHISTORY_WINDOW_PREGNANT"));
    QVERIFY(window.isValid());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QTabWidget* tabs = window.findChild<QTabWidget*>();
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->tabText(0), QStringLiteral("Previous treatment"));
    QCOMPARE(tabs->tabText(1), QStringLiteral("Current treatments"));
    // Previous treatment page: 5 date/medication/dose rows (15 fields).
    QCOMPARE(tabs->widget(0)->findChildren<UxTextInput*>().size(), 10);
    // Current treatments page: 7 rows of 3 fields (21 fields).
    QCOMPARE(tabs->widget(1)->findChildren<UxTextInput*>().size(), 21);
}

void TestPregnancyHistoryWindow::newHistoryDisablesDelete() {
    QVERIFY(!Database::instance().hasPregnancyHistory(m_patientId));
    PregnancyHistoryWindow window(m_patientId, QStringLiteral("PHISTORY_WINDOW_PREGNANT"));
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

void TestPregnancyHistoryWindow::saveThroughUiThenReloadShowsExisting() {
    {
        PregnancyHistoryWindow window(m_patientId, QStringLiteral("PHISTORY_WINDOW_PREGNANT"));
        QVERIFY(window.isValid());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        QGroupBox* personal = findGroup(window, QStringLiteral("Personal data"));
        QVERIFY(personal != nullptr);
        const QList<UxTextInput*> texts = personal->findChildren<UxTextInput*>();
        QCOMPARE(texts.size(), 2);
        texts[1]->setText(QStringLiteral("Latex")); // allergies

        QGroupBox* pregnancy = findGroup(window, QStringLiteral("Pregnancy history"));
        QVERIFY(pregnancy != nullptr);
        const QList<UxCheck*> checks = pregnancy->findChildren<UxCheck*>();
        QCOMPARE(checks.size(), 2);
        checks[1]->setChecked(true); // HIV (man)

        QGroupBox* housing = findGroup(window, QStringLiteral("Housing"));
        QVERIFY(housing != nullptr);
        UxComboInput* animals = housing->findChild<UxComboInput*>();
        QVERIFY(animals != nullptr);
        animals->comboBox()->setCurrentIndex(animals->comboBox()->findData(2));

        QVERIFY(QMetaObject::invokeMethod(&window, "onSave", Qt::DirectConnection));
        QCOMPARE(window.result(), static_cast<int>(QDialog::Accepted));
    }

    QVERIFY(Database::instance().hasPregnancyHistory(m_patientId));
    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "SELECT allergies, obstetric_hiv_man, housing_domestic_animals "
        "FROM b04_pregnancy_history WHERE patient_id=?"));
    q.addBindValue(m_patientId);
    QVERIFY(q.exec());
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("Latex"));
    QCOMPARE(q.value(1).toInt(), 1);
    QCOMPARE(q.value(2).toInt(), 2);

    // Reopening shows the existing history with Delete enabled.
    PregnancyHistoryWindow reopened(m_patientId, QStringLiteral("PHISTORY_WINDOW_PREGNANT"));
    QVERIFY(reopened.isValid());
    reopened.show();
    QVERIFY(QTest::qWaitForWindowExposed(&reopened));
    QGroupBox* personal = findGroup(reopened, QStringLiteral("Personal data"));
    QVERIFY(personal != nullptr);
    const QList<UxTextInput*> texts = personal->findChildren<UxTextInput*>();
    QCOMPARE(texts.size(), 2);
    QCOMPARE(texts[1]->text(), QStringLiteral("Latex"));
    const QList<QPushButton*> buttons = reopened.findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 3);
    QVERIFY(buttons[0]->isEnabled()); // Delete enabled for an existing history
}

} // namespace gambasse

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    gambasse::TestPregnancyHistoryWindow test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_pregnancyhistorywindow.moc"
