#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
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
    // QQuickItem::window() answers a QWindow*: go through QObject.
    auto* window = qobject_cast<QQuickWindow*>(static_cast<QObject*>(item->window()));
    QVERIFY2(window != nullptr,
             qPrintable(QStringLiteral("no window for %1 (%2)").arg(item->objectName(),
                                                                    item->metaObject()->className())));
    const QPoint pos = centerOf(item);
    QTest::mouseMove(window, pos);
    // The hover must be delivered before the press, or the click lands on
    // whatever was under the cursor (a modal overlay, typically).
    QTest::qWait(100);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
}

// Text of the control inside a GxField (or of a bare control by objectName).
QString controlText(QObject* parent, const QString& objectName) {
    auto* field = child(parent, objectName);
    if (field == nullptr)
        return QString();
    const QVariant value = field->property("text");
    if (value.isValid())
        return value.toString();
    auto* byName = field->findChild<QQuickItem*>(QStringLiteral("field"));
    return byName != nullptr ? byName->property("text").toString() : QString();
}

} // namespace

// Verifies the QML adult history screen end to end against the real database:
// a new history cannot be deleted, saving inserts it, reopening shows the
// stored values, editing updates them and deleting removes the row.
class TestQmlAdultHistory : public QObject {
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
    // The history screen is a Window, so it is looked up as a plain object.
    QObject* historyWindow() const;
    QQuickItem* inHistory(const QString& name) const;
    HistoryController* history() const;
    void openScreen();
    void cleanup();
    bool createPatient();
    int storedHistories();
    // Opens the screen and waits for its window to exist.
    HistoryController* openHistory();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void newHistoryCannotBeDeleted();
    void titleBadgesLeaveControlsClear();
    void formRendersEveryControllerField();
    void savingInsertsAndReopeningShowsTheValues();
    void editingUpdatesTheStoredRow();
    void deletingRemovesTheHistory();
    void boundsAndDatesAreCoercedOnSave();
};

void TestQmlAdultHistory::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));
}

void TestQmlAdultHistory::init() {
    QSqlQuery erase(Database::instance().connection());
    QVERIFY2(erase.exec(QStringLiteral("DELETE FROM b03_adult_history")),
             qPrintable(erase.lastError().text()));
    QVERIFY2(erase.exec(QStringLiteral("DELETE FROM b01_patient")),
             qPrintable(erase.lastError().text()));
}

void TestQmlAdultHistory::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestQmlAdultHistory::cleanup() {
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

bool TestQmlAdultHistory::createPatient() {
    Patient p;
    p.name = QStringLiteral("HIST_ADULT");
    p.sex = Patient::Sex::Muller;
    p.birthDate = QDate(1990, 5, 6);
    p.ageRange = 34;
    return m_patientService.create(p) == PatientService::SaveResult::Saved;
}

int TestQmlAdultHistory::storedHistories() {
    QSqlQuery query(Database::instance().connection());
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM b03_adult_history")) || !query.next())
        return -1;
    return query.value(0).toInt();
}

void TestQmlAdultHistory::openScreen() {
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

QObject* TestQmlAdultHistory::historyWindow() const {
    return m_screen != nullptr ? m_screen->findChild<QObject*>(QStringLiteral("historyWindow"))
                               : nullptr;
}

HistoryController* TestQmlAdultHistory::history() const {
    QObject* window = historyWindow();
    if (window == nullptr)
        return nullptr;
    return qobject_cast<HistoryController*>(window->property("controller").value<QObject*>());
}

QQuickItem* TestQmlAdultHistory::inHistory(const QString& name) const {
    QObject* window = historyWindow();
    return window != nullptr ? child(window, name) : nullptr;
}

HistoryController* TestQmlAdultHistory::openHistory() {
    // Every open builds a new controller, so opening again must show the
    // stored data: wait until the instance changes.
    HistoryController* previous = history();
    m_controller->openAdultHistory();
    for (int i = 0; i < 100; ++i) {
        if (inHistory(QStringLiteral("saveHistoryButton")) != nullptr && history() != nullptr
            && history() != previous)
            break;
        QTest::qWait(20);
    }
    HistoryController* controller = history();
    if (controller == nullptr)
        return nullptr;
    auto* window = qobject_cast<QQuickWindow*>(historyWindow());
    // Best effort: offscreen may never report the window as exposed, and the
    // callers check what they need themselves.
    if (window != nullptr)
        (void)QTest::qWaitForWindowExposed(window);
    return controller;
}

void TestQmlAdultHistory::newHistoryCannotBeDeleted() {
    QVERIFY(createPatient());
    openScreen();
    QVERIFY(m_controller->canCreateAdult());
    auto* controller = openHistory();

    // Nothing stored yet: the history does not exist, so it cannot be deleted
    // and the entry is still offered by the main screen.
    QVERIFY(controller->isNew());
    QVERIFY(!controller->canDelete());
    QCOMPARE(storedHistories(), 0);
    QVERIFY(inHistory(QStringLiteral("saveHistoryButton"))->property("enabled").toBool());
    QVERIFY(!inHistory(QStringLiteral("deleteHistoryButton"))->property("enabled").toBool());
    // The form shows the patient name read-only and today's opening date.
    QCOMPARE(controlText(inHistory(QStringLiteral("historyForm")), QStringLiteral("nameField")),
             m_controller->patientName());
    QCOMPARE(controller->value(QStringLiteral("openingDate")).toString(),
             QDate::currentDate().toString(QStringLiteral("dd/MM/yyyy")));
    // The title bar runs edge to edge like the main window's.
    auto* window = qobject_cast<QQuickWindow*>(historyWindow());
    QVERIFY(window != nullptr);
    auto* bar = inHistory(QStringLiteral("historyTitleBar"));
    QVERIFY(bar != nullptr);
    QCOMPARE(bar->mapToScene(QPointF(0, 0)), QPointF(0, 0));
    QCOMPARE(bar->width(), qreal(window->width()));
}

void TestQmlAdultHistory::formRendersEveryControllerField() {
    QVERIFY(createPatient());
    openScreen();
    auto* controller = openHistory();

    auto* form = inHistory(QStringLiteral("historyForm"));
    QVERIFY(form != nullptr);
    QVERIFY(child(form, QStringLiteral("personalGroup")) != nullptr);
    QVERIFY(child(form, QStringLiteral("adultHistoryGroup")) != nullptr);
    QVERIFY(child(form, QStringLiteral("housingGroup")) != nullptr);
    QVERIFY(child(form, QStringLiteral("vitalsGroup")) != nullptr);

    // Every field the controller publishes must be rendered by the form (plus
    // the read-only name box): a typo in a field name would silently lose data.
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
    QCOMPARE(rendered, controller->fieldNames().size() + 1);
}

void TestQmlAdultHistory::savingInsertsAndReopeningShowsTheValues() {
    QVERIFY(createPatient());
    openScreen();
    auto* controller = openHistory();

    // Type into the form the way the user would.
    controller->setValue(QStringLiteral("allergies"), QStringLiteral(" penicillin "));
    controller->setValue(QStringLiteral("historyOccupation"), QStringLiteral("Teacher"));
    controller->setValue(QStringLiteral("historyChildCount"), 2);
    controller->setValue(QStringLiteral("physicalExamWeight"), QStringLiteral("70,5"));
    controller->setValue(QStringLiteral("housingLatrine"), true);
    controller->setValue(QStringLiteral("housingDomesticAnimals"), 2);
    QCOMPARE(storedHistories(), 0);

    clickIn(inHistory(QStringLiteral("saveHistoryButton")));
    QTRY_COMPARE(storedHistories(), 1);
    // Saved: the patient can no longer create the history, only open it.
    QVERIFY(!m_controller->canCreateAdult());
    QVERIFY(m_controller->showAdult());

    // Reopening shows the stored values, both in the controller and in the
    // controls of the form.
    openHistory();
    auto* reopened = history();
    QCOMPARE(reopened->value(QStringLiteral("allergies")).toString(), QStringLiteral("penicillin"));
    QCOMPARE(reopened->value(QStringLiteral("historyOccupation")).toString(),
             QStringLiteral("Teacher"));
    QCOMPARE(reopened->value(QStringLiteral("historyChildCount")).toInt(), 2);
    QCOMPARE(reopened->value(QStringLiteral("physicalExamWeight")).toString(),
             QStringLiteral("70,5"));
    QCOMPARE(reopened->value(QStringLiteral("housingLatrine")).toBool(), true);
    QCOMPARE(reopened->value(QStringLiteral("housingDomesticAnimals")).toInt(), 2);
    QCOMPARE(controlText(inHistory(QStringLiteral("historyForm")),
                         QStringLiteral("historyOccupation")),
             QStringLiteral("Teacher"));
}

void TestQmlAdultHistory::editingUpdatesTheStoredRow() {
    QVERIFY(createPatient());
    openScreen();
    openHistory()->setValue(QStringLiteral("allergies"), QStringLiteral("dust"));
    clickIn(inHistory(QStringLiteral("saveHistoryButton")));
    QTRY_COMPARE(storedHistories(), 1);

    openHistory();
    auto* reopened = history();
    QVERIFY(!reopened->isNew());
    QVERIFY(inHistory(QStringLiteral("deleteHistoryButton"))->property("enabled").toBool());

    reopened->setValue(QStringLiteral("allergies"), QStringLiteral("dust and pollen"));
    reopened->setValue(QStringLiteral("historyHepatitis"), true);
    clickIn(inHistory(QStringLiteral("saveHistoryButton")));
    QTRY_COMPARE(storedHistories(), 1); // updated, not inserted again

    openHistory();
    QCOMPARE(history()->value(QStringLiteral("allergies")).toString(),
             QStringLiteral("dust and pollen"));
    QCOMPARE(history()->value(QStringLiteral("historyHepatitis")).toBool(), true);
}

void TestQmlAdultHistory::deletingRemovesTheHistory() {
    QVERIFY(createPatient());
    openScreen();
    openHistory()->setValue(QStringLiteral("allergies"), QStringLiteral("dust"));
    clickIn(inHistory(QStringLiteral("saveHistoryButton")));
    QTRY_COMPARE(storedHistories(), 1);

    openHistory();
    auto* confirm = historyWindow()->findChild<QObject*>(QStringLiteral("deleteHistoryConfirm"));
    QVERIFY(confirm != nullptr);
    // The popup opens on the next event loop turn after the click that asks
    // for it, so give it a moment before aiming at its buttons.
    clickIn(inHistory(QStringLiteral("deleteHistoryButton")));
    QTRY_VERIFY_WITH_TIMEOUT(confirm->property("visible").toBool(), 2000);
    auto* yes = confirm->findChild<QQuickItem*>(QStringLiteral("dialogYesButton"));
    QVERIFY(yes != nullptr);
    QSignalSpy acceptedSpy(confirm, SIGNAL(accepted()));
    qInfo() << "yes at" << centerOf(yes) << "size" << yes->width() << yes->height()
            << "window" << yes->window()->size() << "popup x/y/w/h"
            << confirm->property("x") << confirm->property("y") << confirm->property("width")
            << confirm->property("height") << "msg" << confirm->property("messageText");
    auto* yesWindow = qobject_cast<QQuickWindow*>(static_cast<QObject*>(yes->window()));
    QTest::mouseMove(yesWindow, centerOf(yes));
    QTest::qWait(100);
    qInfo() << "hovered" << yes->property("hovered") << "visible before"
            << confirm->property("visible");
    clickIn(yes);
    QTest::qWait(100);
    qInfo() << "accepted emitted" << acceptedSpy.count() << "rows" << storedHistories()
            << "visible after" << confirm->property("visible");
    QTRY_COMPARE(storedHistories(), 0);

    // The screen stays open as a new history and the patient can create it.
    QVERIFY(m_controller->canCreateAdult());
    QVERIFY(!inHistory(QStringLiteral("deleteHistoryButton"))->property("enabled").toBool());
}

void TestQmlAdultHistory::boundsAndDatesAreCoercedOnSave() {
    QVERIFY(createPatient());
    openScreen();
    auto* controller = openHistory();

    controller->setValue(QStringLiteral("openingDate"), QStringLiteral("not a date"));
    controller->setValue(QStringLiteral("historyChildCount"), 500);   // above the bound
    controller->setValue(QStringLiteral("physicalExamHeight"), 5000); // above the bound
    controller->setValue(QStringLiteral("allergies"), QString(300, QLatin1Char('x')));
    clickIn(inHistory(QStringLiteral("saveHistoryButton")));
    QTRY_COMPARE(storedHistories(), 1);

    // Same rules as the Widgets gather step: default date, clamped numbers and
    // the field maximum length.
    openHistory();
    const QVariantMap values = history()->values();
    QCOMPARE(values.value(QStringLiteral("openingDate")).toString(), QStringLiteral("01/01/1900"));
    QCOMPARE(values.value(QStringLiteral("historyChildCount")).toInt(), 99);
    QCOMPARE(values.value(QStringLiteral("physicalExamHeight")).toInt(), 999);
    QCOMPARE(values.value(QStringLiteral("allergies")).toString().size(), 100);
}


void TestQmlAdultHistory::titleBadgesLeaveControlsClear() {
    QVERIFY(createPatient());
    openScreen();
    openHistory();
    auto* form = inHistory(QStringLiteral("historyForm"));
    QVERIFY(form != nullptr);
    const QString problems = badgeProblems(form);
    QVERIFY2(problems.isEmpty(), qPrintable(problems));
}

} // namespace gambasse

int main(int argc, char** argv) {
    QGuiApplication::setQuitOnLastWindowClosed(false);
    QGuiApplication app(argc, argv);
    gambasse::TestQmlAdultHistory test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_qmladulthistory.moc"
