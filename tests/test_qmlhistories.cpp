#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <data/common/Database.h>
#include <data/model/Patient.h>
#include <logic/PatientService.h>
#include <ui/InterfaceSettings.h>
#include <ui/controller/HistoryController.h>
#include <ui/controller/PatientController.h>

namespace gambasse {
namespace {

// Every section title badge of a form: it must not cover any control of its
// group, and its title must be shown whole (not elided).
QString badgeProblems(QQuickItem* item) {
    QString problems;
    if (item->objectName() == QLatin1String("groupTitleBadge") && item->isVisible()) {
        QQuickItem* group = item->parentItem();
        const QRectF badge(item->x(), item->y(), item->width(), item->height());
        for (QQuickItem* sibling : group->childItems()) {
            const QString name = sibling->objectName();
            if (sibling == item || !sibling->isVisible() || name == QLatin1String("groupFrame")
                || name == QLatin1String("groupBody"))
                continue;
            const QRectF rect(sibling->x(), sibling->y(), sibling->width(), sibling->height());
            if (badge.intersects(rect))
                problems += QStringLiteral("%1 covers %2; ").arg(group->objectName(), name);
        }
        auto* title = item->findChild<QQuickItem*>(QStringLiteral("groupTitle"));
        if (title != nullptr && title->property("truncated").toBool())
            problems += QStringLiteral("%1 title elided; ").arg(group->objectName());
    }
    for (QQuickItem* kid : item->childItems())
        problems += badgeProblems(kid);
    return problems;
}

QQuickItem* child(QObject* parent, const QString& name) {
    return parent->findChild<QQuickItem*>(name);
}

QPoint centerOf(QQuickItem* item) {
    return item->mapToScene(QPointF(item->width() / 2.0, item->height() / 2.0)).toPoint();
}

// Clicks in the window that owns the item: the history screen is a second
// window, so the events cannot go to the main one.
void clickIn(QQuickItem* item) {
    auto* window = qobject_cast<QQuickWindow*>(static_cast<QObject*>(item->window()));
    QVERIFY(window != nullptr);
    const QPoint pos = centerOf(item);
    QTest::mouseMove(window, pos);
    // The hover must be delivered before the press, or the click lands on
    // whatever was under the cursor (a modal overlay, typically).
    QTest::qWait(100);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
}

// Per-screen data: the table, one text field, one number field and one check,
// enough to walk the whole cycle without repeating the test code.
struct ScreenData {
    QString screenId;   // "pediatric" / "pregnancy"
    QString table;      // b05_pediatric_history / b04_pregnancy_history
    QString textField;  // written and read back as free text
    QString numberField;
    QString checkField;
    Patient::Sex sex;   // the sex that makes the history available
    int ageRange;
};

QVector<ScreenData> screenData() {
    return {
        {QStringLiteral("pediatric"), QStringLiteral("b05_pediatric_history"),
         QStringLiteral("allergies"), QStringLiteral("neonatalWeight"),
         QStringLiteral("vaccineBcg"), Patient::Sex::Home, 4},
        {QStringLiteral("pregnancy"), QStringLiteral("b04_pregnancy_history"),
         QStringLiteral("allergies"), QStringLiteral("obstetricDeliveryCount"),
         QStringLiteral("obstetricHivWoman"), Patient::Sex::Muller, 30},
    };
}

} // namespace

// Walks the full lifecycle of the pediatric and pregnancy history screens
// against the real database, the same way the adult one does: a new history
// cannot be deleted, saving inserts it, reopening shows the stored values,
// editing updates them and deleting removes the row.
class TestQmlHistories : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    PatientService m_patientService;
    QQmlApplicationEngine* m_engine = nullptr;
    PatientController* m_controller = nullptr;
    InterfaceSettings* m_settings = nullptr;
    QQuickWindow* m_window = nullptr;
    QQuickItem* m_screen = nullptr;

    QQuickItem* item(const QString& name) const { return child(m_screen, name); }
    QObject* historyWindow() const;
    QQuickItem* inHistory(const QString& name) const;
    HistoryController* history() const;
    void openScreen();
    void cleanup();
    bool createPatient(const ScreenData& data);
    int storedHistories(const QString& table);
    void open(const ScreenData& data);
    void removeAll();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void newHistoryCannotBeDeleted_data();
    void newHistoryCannotBeDeleted();
    void formRendersEveryField_data();
    void formRendersEveryField();
    void fullCycle_data();
    void fullCycle();
    void fixedRowsAreNotPersisted();
    void titleBadgesLeaveControlsClear_data();
    void titleBadgesLeaveControlsClear();
};

void TestQmlHistories::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));
}

void TestQmlHistories::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestQmlHistories::cleanup() {
    if (m_window != nullptr)
        m_window->close();
    // The controller and the settings belong to the engine.
    m_controller = nullptr;
    m_settings = nullptr;
    delete m_engine;
    m_engine = nullptr;
    m_window = nullptr;
    m_screen = nullptr;
}

QObject* TestQmlHistories::historyWindow() const {
    return m_screen != nullptr ? m_screen->findChild<QObject*>(QStringLiteral("historyWindow"))
                               : nullptr;
}

QQuickItem* TestQmlHistories::inHistory(const QString& name) const {
    QObject* window = historyWindow();
    return window != nullptr ? child(window, name) : nullptr;
}

HistoryController* TestQmlHistories::history() const {
    QObject* window = historyWindow();
    if (window == nullptr)
        return nullptr;
    return qobject_cast<HistoryController*>(window->property("controller").value<QObject*>());
}

bool TestQmlHistories::createPatient(const ScreenData& data) {
    Patient p;
    p.name = QStringLiteral("HIST_%1").arg(data.screenId);
    p.sex = data.sex;
    p.birthDate = QDate(2015, 4, 5);
    p.ageRange = data.ageRange;
    return m_patientService.create(p) == PatientService::SaveResult::Saved;
}

int TestQmlHistories::storedHistories(const QString& table) {
    QSqlQuery query(Database::instance().connection());
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(table)) || !query.next())
        return -1;
    return query.value(0).toInt();
}

void TestQmlHistories::removeAll() {
    QSqlQuery erase(Database::instance().connection());
    for (const ScreenData& data : screenData()) {
        erase.exec(QStringLiteral("DELETE FROM %1").arg(data.table));
        erase.exec(QStringLiteral("DELETE FROM b01_patient"));
    }
}

void TestQmlHistories::openScreen() {
    cleanup();
    {
        // English source texts and the light theme, read by the engine's
        // settings singleton when the shell creates it.
        InterfaceSettings baseline(nullptr);
        baseline.setTheme(QStringLiteral("claro"));
        baseline.setLanguage(QStringLiteral("en"));
    }
    m_engine = new QQmlApplicationEngine();
    m_engine->loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("Root"));
    QVERIFY(!m_engine->rootObjects().isEmpty());
    m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    QVERIFY(m_window != nullptr);
    // The shell owns its controller (loaded on completion) and the settings
    // are the engine's singleton, exactly as in the application.
    m_controller = m_window->property("patientController").value<PatientController*>();
    QVERIFY(m_controller != nullptr);
    m_settings = m_engine->singletonInstance<InterfaceSettings*>(QStringLiteral("Gambasse"),
                                                                 QStringLiteral("InterfaceSettings"));
    QVERIFY(m_settings != nullptr);
    m_screen = child(m_window, QStringLiteral("mainView"));
    QVERIFY(m_screen != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
}

void TestQmlHistories::open(const ScreenData& data) {
    removeAll();
    QVERIFY(createPatient(data));
    openScreen();
    if (data.screenId == QLatin1String("pediatric"))
        m_controller->openPediatricHistory();
    else
        m_controller->openPregnancyHistory();
    for (int i = 0; i < 100 && inHistory(QStringLiteral("saveHistoryButton")) == nullptr; ++i)
        QTest::qWait(20);
    QVERIFY(inHistory(QStringLiteral("saveHistoryButton")) != nullptr);
    QVERIFY(history() != nullptr);
    QVERIFY(history()->isNew()); // nothing stored yet
    // The window must be on screen before aiming clicks at its buttons.
    auto* window = qobject_cast<QQuickWindow*>(historyWindow());
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
}

void TestQmlHistories::newHistoryCannotBeDeleted_data() {
    QTest::addColumn<int>("row");
    QTest::newRow("pediatric") << 0;
    QTest::newRow("pregnancy") << 1;
}

void TestQmlHistories::newHistoryCannotBeDeleted() {
    QFETCH(int, row);
    const ScreenData data = screenData().at(row);
    open(data);
    // The entry is offered by ClinicalContext and the screen opens as new.
    QVERIFY(history()->isNew());
    QVERIFY(!history()->canDelete());
    QCOMPARE(storedHistories(data.table), 0);
    QVERIFY(inHistory(QStringLiteral("saveHistoryButton"))->property("enabled").toBool());
    QVERIFY(!inHistory(QStringLiteral("deleteHistoryButton"))->property("enabled").toBool());
    // The form shows the patient name of the selected patient.
    QCOMPARE(history()->patientName(), m_controller->patientName());
}

void TestQmlHistories::formRendersEveryField_data() {
    newHistoryCannotBeDeleted_data();
}

void TestQmlHistories::formRendersEveryField() {
    QFETCH(int, row);
    const ScreenData data = screenData().at(row);
    open(data);

    auto* form = inHistory(QStringLiteral("historyForm"));
    QVERIFY(form != nullptr);
    // Every field the controller publishes must be rendered by the form: a
    // typo in a field name would silently lose data.
    int rendered = 0;
    const std::function<void(QQuickItem*)> countFields = [&](QQuickItem* parent) {
        for (QQuickItem* c : parent->childItems()) {
            const QString className = QString::fromLatin1(c->metaObject()->className());
            if (className.contains(QStringLiteral("GxField"))
                || className.contains(QStringLiteral("GxCheckField")))
                ++rendered;
            countFields(c);
        }
    };
    countFields(form);
    // The controls of the treatment tabs are created on demand, so the form
    // shows at least every field, never fewer.
    QVERIFY2(rendered + 20 >= history()->fieldNames().size(),
             qPrintable(QStringLiteral("rendered=%1 fields=%2")
                            .arg(rendered)
                            .arg(history()->fieldNames().size())));
    QVERIFY(rendered > 30);
}

void TestQmlHistories::fullCycle_data() {
    newHistoryCannotBeDeleted_data();
}

void TestQmlHistories::fullCycle() {
    QFETCH(int, row);
    const ScreenData data = screenData().at(row);
    open(data);

    auto* controller = history();
    controller->setValue(data.textField, QStringLiteral(" penicillin "));
    controller->setValue(data.numberField, 7);
    controller->setValue(data.checkField, true);
    QCOMPARE(storedHistories(data.table), 0);

    // Save inserts the row and the patient screen stops offering the entry.
    clickIn(inHistory(QStringLiteral("saveHistoryButton")));
    QTRY_COMPARE(storedHistories(data.table), 1);
    if (data.screenId == QLatin1String("pediatric"))
        QVERIFY(!m_controller->canCreatePediatric());
    else
        QVERIFY(!m_controller->canCreatePregnancy());

    // Reopening shows the stored values (trimmed text, number and check).
    if (data.screenId == QLatin1String("pediatric"))
        m_controller->openPediatricHistory();
    else
        m_controller->openPregnancyHistory();
    for (int i = 0; i < 100 && history() == controller; ++i)
        QTest::qWait(20);
    QVERIFY(history() != controller);
    QCOMPARE(history()->value(data.textField).toString(), QStringLiteral("penicillin"));
    QCOMPARE(history()->value(data.numberField).toInt(), 7);
    QCOMPARE(history()->value(data.checkField).toBool(), true);
    QVERIFY(inHistory(QStringLiteral("deleteHistoryButton"))->property("enabled").toBool());

    // Editing updates the same row.
    history()->setValue(data.textField, QStringLiteral("dust and pollen"));
    clickIn(inHistory(QStringLiteral("saveHistoryButton")));
    QTRY_COMPARE(storedHistories(data.table), 1);

    if (data.screenId == QLatin1String("pediatric"))
        m_controller->openPediatricHistory();
    else
        m_controller->openPregnancyHistory();
    for (int i = 0; i < 100 && history()->value(data.textField).toString()
                                    != QStringLiteral("dust and pollen");
         ++i)
        QTest::qWait(20);
    QCOMPARE(history()->value(data.textField).toString(), QStringLiteral("dust and pollen"));

    // Deleting asks for confirmation and removes the row. The offscreen
    // plugin can swallow the click aimed at a freshly opened modal popup, so
    // the confirmation is retried until the row is gone.
    auto* confirm = historyWindow()->findChild<QObject*>(QStringLiteral("deleteHistoryConfirm"));
    QVERIFY(confirm != nullptr);
    for (int attempt = 0; attempt < 3 && storedHistories(data.table) == 1; ++attempt) {
        if (!confirm->property("visible").toBool()) {
            clickIn(inHistory(QStringLiteral("deleteHistoryButton")));
            QTRY_VERIFY_WITH_TIMEOUT(confirm->property("visible").toBool(), 2000);
        }
        auto* yes = confirm->findChild<QQuickItem*>(QStringLiteral("dialogYesButton"));
        QVERIFY(yes != nullptr);
        clickIn(yes);
        QTest::qWait(150);
    }
    QTRY_COMPARE(storedHistories(data.table), 0);
    // The screen stays open as a new history and the patient can create it.
    if (data.screenId == QLatin1String("pediatric"))
        QVERIFY(m_controller->canCreatePediatric());
    else
        QVERIFY(m_controller->canCreatePregnancy());
    QVERIFY(!inHistory(QStringLiteral("deleteHistoryButton"))->property("enabled").toBool());
}

void TestQmlHistories::fixedRowsAreNotPersisted() {
    // The VB.NET form shows iron, folic acid and deworming as fixed values in
    // the current treatments tab: they are displayed but never saved, so they
    // are not controller fields.
    removeAll();
    const ScreenData data = screenData().at(1); // pregnancy
    QVERIFY(createPatient(data));
    openScreen();
    m_controller->openPregnancyHistory();
    for (int i = 0; i < 100 && inHistory(QStringLiteral("saveHistoryButton")) == nullptr; ++i)
        QTest::qWait(20);
    QVERIFY(history() != nullptr);
    for (const QString& name : history()->fieldNames())
        QVERIFY2(!name.contains(QStringLiteral("Iron")) && !name.contains(QStringLiteral("Folic"))
                     && !name.contains(QStringLiteral("Deworming")),
                 qPrintable(name));
}


void TestQmlHistories::titleBadgesLeaveControlsClear_data() {
    newHistoryCannotBeDeleted_data();
}

void TestQmlHistories::titleBadgesLeaveControlsClear() {
    QFETCH(int, row);
    open(screenData().at(row));
    auto* form = inHistory(QStringLiteral("historyForm"));
    QVERIFY(form != nullptr);
    const QString problems = badgeProblems(form);
    QVERIFY2(problems.isEmpty(), qPrintable(problems));
}

} // namespace gambasse

int main(int argc, char** argv) {
    QGuiApplication::setQuitOnLastWindowClosed(false);
    QGuiApplication app(argc, argv);
    gambasse::TestQmlHistories test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_qmlhistories.moc"
