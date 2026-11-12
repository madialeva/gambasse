#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>
#include <functional>

#include <Paths.h>
#include <VisibleTextChecks.h>
#include <ZoomTestSupport.h>
#include <data/common/Database.h>
#include <data/model/Patient.h>
#include <logic/PatientService.h>
#include <ui/InterfaceSettings.h>
#include <ui/controller/ConsultationController.h>
#include <ui/controller/PatientController.h>
#include <ui/controller/PediatricConsultationController.h>

namespace gambasse {
namespace {

QQuickItem* child(QObject* parent, const QString& name) {
    return parent->findChild<QQuickItem*>(name);
}

// Visual-tree search: delegates (list rows, repeated form rows) are not in
// the QObject tree of the window.
QQuickItem* visualChild(QQuickItem* item, const QString& name) {
    if (item->objectName() == name)
        return item;
    for (QQuickItem* kid : item->childItems()) {
        if (QQuickItem* found = visualChild(kid, name))
            return found;
    }
    return nullptr;
}

QPoint centerOf(QQuickItem* item) {
    return item->mapToScene(QPointF(item->width() / 2.0, item->height() / 2.0)).toPoint();
}

// The letters of the patient's side over the body diagram, from left to right.
QList<QQuickItem*> sideLetters(QQuickItem* diagram) {
    QList<QQuickItem*> letters;
    for (QQuickItem* kid : diagram->childItems()) {
        if (kid->objectName() == QLatin1String("sideLetter"))
            letters.append(kid);
    }
    std::sort(letters.begin(), letters.end(),
              [](const QQuickItem* a, const QQuickItem* b) { return a->x() < b->x(); });
    return letters;
}

// Clicks in the window that owns the item: the consultation screen is a
// second window, so the events cannot go to the main one.
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

// A form control bound to a controller field (GxField, GxCheckField and the
// inline components the forms derive from them).
bool isFieldControl(QQuickItem* item) {
    const QMetaObject* meta = item->metaObject();
    return meta->indexOfProperty("field") >= 0 && meta->indexOfProperty("controller") >= 0;
}

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

// Per-screen data: the tables, the patient that makes the consultation
// available, and one text, number and check field to drive the cycle.
struct ScreenData {
    QString screenId;
    QString historyTable;      // the foreign key target of the consultations
    QString consultationTable;
    Patient::Sex sex;
    int ageRange;
    QString numberField;
    QString numberValue;       // as typed in the form
    QString checkField;
};

QList<ScreenData> screenData() {
    return {
        {QStringLiteral("adult"), QStringLiteral("b03_adult_history"),
         QStringLiteral("b06_adult_consultation"), Patient::Sex::Home, 40,
         QStringLiteral("lesionRegion2"), QStringLiteral("37"), QStringLiteral("wheezing")},
        {QStringLiteral("pregnancy"), QStringLiteral("b04_pregnancy_history"),
         QStringLiteral("b07_pregnancy_consultation"), Patient::Sex::Muller, 30,
         QStringLiteral("weight"), QStringLiteral("72,5"), QStringLiteral("proteinuria")},
        {QStringLiteral("pediatric"), QStringLiteral("b05_pediatric_history"),
         QStringLiteral("b08_pediatric_consultation"), Patient::Sex::Home, 5,
         QStringLiteral("weight"), QStringLiteral("12,34"), QStringLiteral("cough")},
    };
}

} // namespace

// Verifies the three consultation screens end to end against the real
// database: the close-only window, a form control for every published field,
// the list and the new/save/select/delete cycle without closing the window.
class TestQmlConsultations : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QQmlApplicationEngine* m_engine = nullptr;
    QQuickWindow* m_window = nullptr;
    PatientController* m_controller = nullptr;
    QQuickItem* m_screen = nullptr;
    PatientService m_patientService;
    qlonglong m_patientId = 0;

    void openScreen();
    // With an `area`, the child windows take it as their available area before
    // the consultation window is first placed.
    void open(const ScreenData& data, int storedBefore = 0, const QRect& area = QRect());
    InterfaceSettings* settings() const {
        return m_engine->singletonInstance<InterfaceSettings*>(QStringLiteral("Gambasse"),
                                                               QStringLiteral("InterfaceSettings"));
    }
    void openConsultation(const ScreenData& data);
    QQuickWindow* consultationWindow() const;
    QQuickItem* inConsultation(const QString& name) const;
    ConsultationController* consultation() const;
    int stored(const ScreenData& data) const;
    void confirmDelete(bool yes);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void opensWithANewConsultation_data();
    void opensWithANewConsultation();
    void everyTextShowsWhole_data();
    void everyTextShowsWhole();
    void formRendersEveryField_data();
    void formRendersEveryField();
    void fullCycle_data();
    void fullCycle();
    void cancelledDeleteKeepsTheConsultation_data();
    void cancelledDeleteKeepsTheConsultation();
    void exitClosesWithoutSaving_data();
    void exitClosesWithoutSaving();
    void titleBadgesLeaveControlsClear_data();
    void titleBadgesLeaveControlsClear();
    void pregnancyFixedRowsAreReadOnly();
    void pediatricCoughAndCodesPersist();
    void reusedWindowKeepsTheFormControllerTyped();
    void pediatricTabsShowWholeCaptions_data();
    void pediatricTabsShowWholeCaptions();
    void bodyDiagramLettersFollowTheLanguage_data();
    void bodyDiagramLettersFollowTheLanguage();
    void bodyDiagramFollowsItsSize();
    void zoomScalesTheWindow_data();
    void zoomScalesTheWindow();
    void zoomElsewhereCentresTheWindowOnOpen();
    void everyTextShowsWholeZoomed_data();
    void everyTextShowsWholeZoomed();
    void popupsFollowTheZoom();
    void bodyDiagramIsRenderedAtItsZoomedSize();
};

void TestQmlConsultations::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));
}

void TestQmlConsultations::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestQmlConsultations::cleanup() {
    if (m_window != nullptr)
        m_window->close();
    m_controller = nullptr;
    delete m_engine;
    m_engine = nullptr;
    m_window = nullptr;
    m_screen = nullptr;
}

QQuickWindow* TestQmlConsultations::consultationWindow() const {
    return m_screen != nullptr
               ? m_screen->findChild<QQuickWindow*>(QStringLiteral("consultationWindow"))
               : nullptr;
}

QQuickItem* TestQmlConsultations::inConsultation(const QString& name) const {
    QQuickWindow* window = consultationWindow();
    if (window == nullptr)
        return nullptr;
    if (QQuickItem* item = child(window, name))
        return item;
    return visualChild(window->contentItem(), name);
}

ConsultationController* TestQmlConsultations::consultation() const {
    QQuickWindow* window = consultationWindow();
    if (window == nullptr)
        return nullptr;
    return qobject_cast<ConsultationController*>(
        window->property("controller").value<QObject*>());
}

int TestQmlConsultations::stored(const ScreenData& data) const {
    QSqlQuery query(Database::instance().connection());
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(data.consultationTable))
        || !query.next())
        return -1;
    return query.value(0).toInt();
}

void TestQmlConsultations::openScreen() {
    cleanup();
    {
        // English source texts and the light theme, read by the engine's
        // settings singleton when the shell creates it.
        InterfaceSettings baseline(nullptr);
        baseline.setTheme(QStringLiteral("claro"));
        baseline.setLanguage(QStringLiteral("en"));
        baseline.setLetterSize(13);
    }
    m_engine = new QQmlApplicationEngine();
    // Up front, like the application: it installs the interface font.
    m_engine->singletonInstance<InterfaceSettings*>(QStringLiteral("Gambasse"),
                                                    QStringLiteral("InterfaceSettings"));
    m_engine->loadFromModule(QStringLiteral("Gambasse"), QStringLiteral("Root"));
    QVERIFY(!m_engine->rootObjects().isEmpty());
    m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    QVERIFY(m_window != nullptr);
    m_controller = m_window->property("patientController").value<PatientController*>();
    QVERIFY(m_controller != nullptr);
    m_screen = child(m_window, QStringLiteral("mainView"));
    QVERIFY(m_screen != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
}

// A patient with the history the consultations need, `storedBefore`
// consultations already saved, the main screen and the consultation screen.
void TestQmlConsultations::open(const ScreenData& data, int storedBefore, const QRect& area) {
    QSqlQuery erase(Database::instance().connection());
    QVERIFY(erase.exec(QStringLiteral("DELETE FROM b01_patient")));
    Patient p;
    p.name = QStringLiteral("CONSULT_%1").arg(data.screenId);
    p.sex = data.sex;
    p.birthDate = QDate(2015, 4, 5);
    p.ageRange = data.ageRange;
    QCOMPARE(m_patientService.create(p), PatientService::SaveResult::Saved);
    m_patientId = p.id;
    QSqlQuery history(Database::instance().connection());
    history.prepare(QStringLiteral("INSERT INTO %1 (patient_id) VALUES (?)").arg(data.historyTable));
    history.addBindValue(p.id);
    QVERIFY2(history.exec(), qPrintable(history.lastError().text()));
    const QString dateColumn = data.screenId == QLatin1String("pediatric")
                                   ? QStringLiteral("consultation_date")
                                   : QStringLiteral("date");
    for (int i = 0; i < storedBefore; ++i) {
        QSqlQuery c(Database::instance().connection());
        c.prepare(QStringLiteral("INSERT INTO %1 (patient_id, %2, reason) VALUES (?, ?, ?)")
                      .arg(data.consultationTable, dateColumn));
        c.addBindValue(p.id);
        c.addBindValue(QStringLiteral("2025-0%1-10 10:00:00").arg(i + 1));
        c.addBindValue(QStringLiteral("Earlier %1").arg(i + 1));
        QVERIFY2(c.exec(), qPrintable(c.lastError().text()));
    }
    openScreen();
    if (area.isValid())
        zoomtest::setArea({m_screen->findChild<QObject*>(QStringLiteral("historyWindow")),
                           consultationWindow()},
                          area);
    openConsultation(data);
}

void TestQmlConsultations::openConsultation(const ScreenData& data) {
    // The consultation entry is shown because the history exists.
    if (data.screenId == QLatin1String("adult")) {
        QVERIFY(m_controller->showAdult());
        clickIn(child(m_screen, QStringLiteral("entryAdultConsultation")));
    } else if (data.screenId == QLatin1String("pregnancy")) {
        QVERIFY(m_controller->showPregnancy());
        clickIn(child(m_screen, QStringLiteral("entryPregnancyConsultation")));
    } else {
        QVERIFY(m_controller->showPediatric());
        clickIn(child(m_screen, QStringLiteral("entryPediatricConsultation")));
    }
    for (int i = 0; i < 100 && (consultationWindow() == nullptr || !consultationWindow()->isVisible());
         ++i)
        QTest::qWait(20);
    QVERIFY(consultationWindow() != nullptr && consultationWindow()->isVisible());
    QVERIFY(QTest::qWaitForWindowExposed(consultationWindow()));
    QVERIFY(consultation() != nullptr);
    QVERIFY(consultation()->loaded());
}

// Clicks Delete and answers the confirmation. The offscreen plugin can
// swallow the click aimed at a freshly opened modal popup, so the answer is
// retried until the popup closes.
void TestQmlConsultations::confirmDelete(bool yes) {
    auto* confirm =
        consultationWindow()->findChild<QObject*>(QStringLiteral("deleteConsultationConfirm"));
    QVERIFY(confirm != nullptr);
    clickIn(inConsultation(QStringLiteral("deleteConsultationButton")));
    QTRY_VERIFY_WITH_TIMEOUT(confirm->property("visible").toBool(), 2000);
    // The Yes/No row is laid out once the popup shows: aiming before it
    // settles hits the neighbouring button.
    QTest::qWait(200);
    auto* button = confirm->findChild<QQuickItem*>(
        yes ? QStringLiteral("dialogYesButton") : QStringLiteral("dialogNoButton"));
    QVERIFY(button != nullptr);
    for (int attempt = 0; attempt < 3 && confirm->property("visible").toBool(); ++attempt) {
        clickIn(button);
        QTest::qWait(150);
    }
    QTRY_VERIFY(!confirm->property("visible").toBool());
}

void TestQmlConsultations::opensWithANewConsultation_data() {
    QTest::addColumn<int>("row");
    for (int i = 0; i < screenData().size(); ++i)
        QTest::newRow(qPrintable(screenData().at(i).screenId)) << i;
}

void TestQmlConsultations::opensWithANewConsultation() {
    QFETCH(int, row);
    const ScreenData data = screenData().at(row);
    open(data, 2);

    // The patient screen behind fades under its veil.
    QQuickItem* veil = child(m_window, QStringLiteral("modalVeil"));
    QVERIFY(veil != nullptr);
    QTRY_COMPARE(veil->opacity(), 1.0);
    QVERIFY(veil->isVisible());
    QVERIFY(veil->z() > child(m_window, QStringLiteral("titleBar"))->z());

    // Close-only title bar.
    auto* bar = inConsultation(QStringLiteral("consultationTitleBar"));
    QVERIFY(bar != nullptr);
    QVERIFY(bar->property("closeOnly").toBool());
    QVERIFY(!child(bar, QStringLiteral("minimizeButton"))->isVisible());
    QVERIFY(!child(bar, QStringLiteral("maximizeButton"))->isVisible());
    QVERIFY(child(bar, QStringLiteral("closeButton"))->isVisible());
    // The Help menu belongs to the main window only.
    QVERIFY(!child(bar, QStringLiteral("helpButton"))->isVisible());

    // The list of the patient, most recent first, and a new consultation.
    ConsultationController* c = consultation();
    QCOMPARE(c->patientName(), m_controller->patientName());
    QCOMPARE(c->consultations().size(), 2);
    QCOMPARE(c->consultations().first().toMap().value(QStringLiteral("reason")).toString(),
             QStringLiteral("Earlier 2"));
    QVERIFY(c->isNew());
    QCOMPARE(c->dateText(), QDate::currentDate().toString(QStringLiteral("dd/MM/yyyy")));
    QVERIFY(inConsultation(QStringLiteral("saveConsultationButton"))->property("enabled").toBool());
    QVERIFY(!inConsultation(QStringLiteral("newConsultationButton"))->property("enabled").toBool());
    QVERIFY(!inConsultation(QStringLiteral("deleteConsultationButton"))->property("enabled").toBool());

    // The reason field has the focus.
    QQuickItem* reason = inConsultation(QStringLiteral("reason"));
    QVERIFY(reason != nullptr);
    QTRY_VERIFY(consultationWindow()->activeFocusItem() != nullptr);
    QVERIFY(reason->isAncestorOf(consultationWindow()->activeFocusItem()));
}

void TestQmlConsultations::everyTextShowsWhole_data() {
    QTest::addColumn<int>("row");
    QTest::addColumn<QString>("language");
    for (const QString& language :
         {QStringLiteral("en"), QStringLiteral("es"), QStringLiteral("pt")}) {
        for (int i = 0; i < screenData().size(); ++i) {
            QTest::addRow("%s-%s", qPrintable(screenData().at(i).screenId), qPrintable(language))
                << i << language;
        }
    }
}

void TestQmlConsultations::everyTextShowsWhole() {
    QFETCH(int, row);
    QFETCH(QString, language);
    open(screenData().at(row));
    auto* settings = m_engine->singletonInstance<InterfaceSettings*>(
        QStringLiteral("Gambasse"), QStringLiteral("InterfaceSettings"));
    settings->setLanguage(language);
    QTest::qWait(100);
    const QStringList cut = cutTextsOnEveryTab(consultationWindow()->contentItem());
    settings->setLanguage(QStringLiteral("en"));
    QVERIFY2(cut.isEmpty(), qPrintable(cut.join(QStringLiteral(" | "))));
}

void TestQmlConsultations::formRendersEveryField_data() {
    opensWithANewConsultation_data();
}

void TestQmlConsultations::formRendersEveryField() {
    QFETCH(int, row);
    open(screenData().at(row));
    auto* form = inConsultation(QStringLiteral("consultationForm"));
    QVERIFY(form != nullptr);
    // Every field the controller publishes must have its control in the form:
    // a typo in a field name would silently lose data.
    QSet<QString> rendered;
    const std::function<void(QQuickItem*)> collect = [&](QQuickItem* parent) {
        for (QQuickItem* c : parent->childItems()) {
            if (isFieldControl(c))
                rendered.insert(c->property("field").toString());
            collect(c);
        }
    };
    collect(form);
    for (const QString& field : consultation()->fieldNames())
        QVERIFY2(rendered.contains(field), qPrintable(field + QStringLiteral(" has no control")));
}

void TestQmlConsultations::fullCycle_data() {
    opensWithANewConsultation_data();
}

void TestQmlConsultations::fullCycle() {
    QFETCH(int, row);
    const ScreenData data = screenData().at(row);
    open(data, 1);
    ConsultationController* c = consultation();

    c->setValue(QStringLiteral("reason"), QStringLiteral(" Fever and cough "));
    c->setValue(data.numberField, data.numberValue);
    c->setValue(data.checkField, true);

    // Save inserts the consultation and keeps the window open, with the saved
    // consultation first in the list and selected.
    clickIn(inConsultation(QStringLiteral("saveConsultationButton")));
    QTRY_COMPARE(stored(data), 2);
    QVERIFY(consultationWindow()->isVisible());
    QCOMPARE(c->consultations().size(), 2);
    QCOMPARE(c->currentIndex(), 0);
    QCOMPARE(c->value(QStringLiteral("reason")).toString(), QStringLiteral("Fever and cough"));
    QTRY_VERIFY(inConsultation(QStringLiteral("newConsultationButton"))->property("enabled").toBool());
    QVERIFY(inConsultation(QStringLiteral("deleteConsultationButton"))->property("enabled").toBool());
    const QString savedDate =
        c->consultations().first().toMap().value(QStringLiteral("date")).toString();

    // Editing updates the same consultation, keeping its date.
    c->setValue(QStringLiteral("reason"), QStringLiteral("Fever"));
    clickIn(inConsultation(QStringLiteral("saveConsultationButton")));
    QTRY_COMPARE(c->consultations().first().toMap().value(QStringLiteral("reason")).toString(),
                 QStringLiteral("Fever"));
    QCOMPARE(stored(data), 2);
    QCOMPARE(c->consultations().first().toMap().value(QStringLiteral("date")).toString(),
             savedDate);

    // New clears the form.
    clickIn(inConsultation(QStringLiteral("newConsultationButton")));
    QTRY_VERIFY(c->isNew());
    QCOMPARE(c->value(QStringLiteral("reason")).toString(), QString());
    QCOMPARE(c->value(data.checkField).toBool(), false);
    QVERIFY(!inConsultation(QStringLiteral("newConsultationButton"))->property("enabled").toBool());

    // Clicking the first row of the list edits it again.
    QQuickItem* firstRow = inConsultation(QStringLiteral("consultationRow"));
    QVERIFY(firstRow != nullptr);
    clickIn(firstRow);
    QTRY_COMPARE(c->currentIndex(), 0);
    QCOMPARE(c->value(QStringLiteral("reason")).toString(), QStringLiteral("Fever"));
    QCOMPARE(c->value(data.numberField).toString().remove(QLatin1Char(',')).toInt(),
             QString(data.numberValue).remove(QLatin1Char(',')).toInt());
    QCOMPARE(c->value(data.checkField).toBool(), true);

    // Delete asks for confirmation and leaves a new consultation.
    confirmDelete(true);
    QTRY_COMPARE(stored(data), 1);
    QVERIFY(c->isNew());
    QCOMPARE(c->consultations().size(), 1);
    QVERIFY(consultationWindow()->isVisible());
    QVERIFY(!inConsultation(QStringLiteral("deleteConsultationButton"))->property("enabled").toBool());
}

void TestQmlConsultations::cancelledDeleteKeepsTheConsultation_data() {
    opensWithANewConsultation_data();
}

void TestQmlConsultations::cancelledDeleteKeepsTheConsultation() {
    QFETCH(int, row);
    const ScreenData data = screenData().at(row);
    open(data, 1);
    consultation()->select(0);
    QTRY_VERIFY(inConsultation(QStringLiteral("deleteConsultationButton"))->property("enabled").toBool());
    confirmDelete(false);
    QCOMPARE(stored(data), 1);
    QCOMPARE(consultation()->currentIndex(), 0);
    QVERIFY(!consultation()->isNew());
}

void TestQmlConsultations::exitClosesWithoutSaving_data() {
    opensWithANewConsultation_data();
}

void TestQmlConsultations::exitClosesWithoutSaving() {
    QFETCH(int, row);
    const ScreenData data = screenData().at(row);
    open(data);
    consultation()->setValue(QStringLiteral("reason"), QStringLiteral("Not saved"));
    clickIn(inConsultation(QStringLiteral("exitConsultationButton")));
    QTRY_VERIFY(!consultationWindow()->isVisible());
    QCOMPARE(stored(data), 0);
    // The veil fades away with the window.
    QQuickItem* veil = child(m_window, QStringLiteral("modalVeil"));
    QVERIFY(veil != nullptr);
    QTRY_VERIFY(!veil->isVisible());
}

void TestQmlConsultations::titleBadgesLeaveControlsClear_data() {
    opensWithANewConsultation_data();
}

void TestQmlConsultations::titleBadgesLeaveControlsClear() {
    QFETCH(int, row);
    open(screenData().at(row));
    auto* form = inConsultation(QStringLiteral("consultationForm"));
    QVERIFY(form != nullptr);
    const QString problems = badgeProblems(form);
    QVERIFY2(problems.isEmpty(), qPrintable(problems));
}

void TestQmlConsultations::pregnancyFixedRowsAreReadOnly() {
    // Iron, folic acid and deworming are shown as a fixed prescription: read
    // only, not controller fields, so never saved.
    open(screenData().at(1));
    for (const QString& name : consultation()->fieldNames())
        QVERIFY2(!name.contains(QStringLiteral("Iron")) && !name.contains(QStringLiteral("Folic"))
                     && !name.contains(QStringLiteral("Deworming")),
                 qPrintable(name));
    auto* form = inConsultation(QStringLiteral("consultationForm"));
    QVERIFY(form != nullptr);
    QStringList fixedTexts;
    const std::function<void(QQuickItem*)> collect = [&](QQuickItem* parent) {
        for (QQuickItem* c : parent->childItems()) {
            if (isFieldControl(c) && c->metaObject()->indexOfProperty("staticText") >= 0
                && !c->property("staticText").toString().isEmpty()
                && c->property("field").toString().isEmpty()
                && c->objectName() != QLatin1String("nameField")
                && c->objectName() != QLatin1String("dateField")) {
                QVERIFY(c->property("readOnly").toBool());
                fixedTexts.append(c->property("staticText").toString());
            }
            collect(c);
        }
    };
    collect(form);
    QCOMPARE(fixedTexts.size(), 9);
    QVERIFY(fixedTexts.contains(QStringLiteral("Ferrous sulfate")));
    QVERIFY(fixedTexts.contains(QStringLiteral("Single dose")));
}

void TestQmlConsultations::pediatricCoughAndCodesPersist() {
    const ScreenData data = screenData().at(2);
    open(data);
    ConsultationController* c = consultation();
    c->setValue(QStringLiteral("cough"), true);
    c->setValue(QStringLiteral("coughDays"), QStringLiteral("3"));
    c->setValue(QStringLiteral("treatmentMedication7"), 1001);
    c->setValue(QStringLiteral("recommendation2"), 4);
    clickIn(inConsultation(QStringLiteral("saveConsultationButton")));
    QTRY_COMPARE(stored(data), 1);

    // Reopened in Portuguese, the same codes show their Portuguese labels.
    auto* settings = m_engine->singletonInstance<InterfaceSettings*>(
        QStringLiteral("Gambasse"), QStringLiteral("InterfaceSettings"));
    QVERIFY(settings != nullptr);
    consultationWindow()->close();
    settings->setLanguage(QStringLiteral("pt"));
    openConsultation(data);
    ConsultationController* reopened = consultation();
    QVERIFY(reopened != c);
    reopened->select(0);
    QCOMPARE(reopened->value(QStringLiteral("cough")).toBool(), true);
    QCOMPARE(reopened->value(QStringLiteral("coughDays")).toInt(), 3);
    QCOMPARE(reopened->value(QStringLiteral("treatmentMedication7")).toInt(), 1001);
    auto* examTabs = inConsultation(QStringLiteral("treatmentTabs"));
    QVERIFY(examTabs != nullptr);
    examTabs->setProperty("currentIndex", 1);
    QQuickItem* combo = inConsultation(QStringLiteral("treatmentMedication7"));
    QVERIFY(combo != nullptr);
    QTRY_COMPARE(combo->property("stored").toInt(), 1001);
    QQuickItem* shown = visualChild(combo, QStringLiteral("comboText"));
    QVERIFY(shown != nullptr);
    QTRY_COMPARE(shown->property("text").toString(), QStringLiteral("Desinfetante:"));
    settings->setLanguage(QStringLiteral("en"));
}

// The window is reused between screens: opening the pediatric consultation
// after another one must not hand the previous controller to the pediatric
// form, whose controller property has the pediatric type.
void TestQmlConsultations::reusedWindowKeepsTheFormControllerTyped() {
    const ScreenData adult = screenData().at(0);
    open(adult, 1);
    consultationWindow()->close();
    QSqlQuery history(Database::instance().connection());
    history.prepare(QStringLiteral("INSERT INTO b05_pediatric_history (patient_id) VALUES (?)"));
    history.addBindValue(m_patientId);
    QVERIFY2(history.exec(), qPrintable(history.lastError().text()));

    QTest::failOnWarning(QRegularExpression(QStringLiteral("Cannot assign")));
    m_controller->openPediatricConsultation();
    QTRY_VERIFY(consultationWindow()->isVisible());
    QVERIFY(qobject_cast<PediatricConsultationController*>(
                consultationWindow()->property("controller").value<QObject*>())
            != nullptr);
    auto* form = consultationWindow()->findChild<QQuickItem*>(QStringLiteral("consultationForm"));
    QVERIFY(form != nullptr);
    QCOMPARE(form->property("item").value<QObject*>()->property("controller").value<QObject*>(),
             consultationWindow()->property("controller").value<QObject*>());
}

void TestQmlConsultations::bodyDiagramLettersFollowTheLanguage_data() {
    QTest::addColumn<QString>("language");
    QTest::addColumn<QString>("letters");
    // The patient's right and left on the front view, then left and right on
    // the back view.
    QTest::newRow("en") << QStringLiteral("en") << QStringLiteral("R L L R");
    QTest::newRow("es") << QStringLiteral("es") << QStringLiteral("D I I D");
    QTest::newRow("pt") << QStringLiteral("pt") << QStringLiteral("D E E D");
}

void TestQmlConsultations::bodyDiagramLettersFollowTheLanguage() {
    QFETCH(QString, language);
    QFETCH(QString, letters);
    open(screenData().at(0));
    QQuickItem* diagram = inConsultation(QStringLiteral("bodyDiagram"));
    QVERIFY(diagram != nullptr);
    auto* settings = m_engine->singletonInstance<InterfaceSettings*>(
        QStringLiteral("Gambasse"), QStringLiteral("InterfaceSettings"));
    settings->setLanguage(language);
    QStringList shown;
    for (const QQuickItem* letter : sideLetters(diagram))
        shown.append(letter->property("text").toString());
    settings->setLanguage(QStringLiteral("en"));
    QCOMPARE(shown.join(u' '), letters);
}

// The picture is drawn at the size it shows, not stretched, and its letters stay
// on the same spot of it.
void TestQmlConsultations::bodyDiagramFollowsItsSize() {
    open(screenData().at(0));
    QQuickItem* diagram = inConsultation(QStringLiteral("bodyDiagram"));
    QVERIFY(diagram != nullptr);
    QQuickItem* picture = visualChild(diagram, QStringLiteral("bodyRegions"));
    QVERIFY(picture != nullptr);
    const int ready = 1; // Image.Ready
    QCOMPARE(picture->property("status").toInt(), ready);
    QCOMPARE(picture->property("sourceSize").toSize(), picture->size().toSize());

    const QList<QQuickItem*> letters = sideLetters(diagram);
    QCOMPARE(letters.size(), 4);
    const auto center = [](const QQuickItem* letter) {
        return QPointF(letter->x() + letter->width() / 2.0, letter->y() + letter->height() / 2.0);
    };
    const auto fontSize = [](const QQuickItem* letter) {
        return letter->property("font").value<QFont>().pixelSize();
    };
    QList<QPointF> centers;
    for (const QQuickItem* letter : letters)
        centers.append(center(letter));
    const int smallFont = fontSize(letters.first());

    diagram->setSize(diagram->size() * 2.0);

    QTRY_COMPARE(picture->property("status").toInt(), ready);
    QCOMPARE(picture->property("sourceSize").toSize(), picture->size().toSize());
    QVERIFY(fontSize(letters.first()) > smallFont);
    for (int i = 0; i < letters.size(); ++i) {
        const QPointF moved = center(letters.at(i)) - centers.at(i) * 2.0;
        QVERIFY2(moved.manhattanLength() < 1.0,
                 qPrintable(letters.at(i)->property("text").toString()));
    }
}

void TestQmlConsultations::pediatricTabsShowWholeCaptions_data() {
    QTest::addColumn<QString>("language");
    QTest::newRow("en") << QStringLiteral("en");
    QTest::newRow("es") << QStringLiteral("es");
    QTest::newRow("pt") << QStringLiteral("pt");
}

void TestQmlConsultations::pediatricTabsShowWholeCaptions() {
    QFETCH(QString, language);
    const ScreenData data = screenData().at(2);
    open(data);
    auto* settings = m_engine->singletonInstance<InterfaceSettings*>(
        QStringLiteral("Gambasse"), QStringLiteral("InterfaceSettings"));
    settings->setLanguage(language);
    QTest::qWait(100);
    for (const char* name : {"examTabs", "treatmentTabs"}) {
        QQuickItem* tabs = inConsultation(QLatin1String(name));
        QVERIFY(tabs != nullptr);
        const int count = tabs->property("titles").toStringList().size();
        qreal right = 0;
        for (int i = 0; i < count; ++i) {
            QQuickItem* tab = visualChild(tabs, QStringLiteral("tab%1").arg(i));
            QVERIFY(tab != nullptr);
            // Every caption fits its tab and every tab fits the zone.
            QQuickItem* text = tab->property("contentItem").value<QQuickItem*>();
            QVERIFY(text != nullptr);
            QVERIFY2(!text->property("truncated").toBool()
                         && text->property("implicitWidth").toReal() <= text->width() + 0.5,
                     qPrintable(tab->property("text").toString()));
            right = qMax(right, tab->mapToItem(tabs, QPointF(tab->width(), 0)).x());
        }
        QVERIFY2(right <= tabs->width(),
                 qPrintable(QStringLiteral("%1: tabs end at %2 of %3")
                                .arg(QLatin1String(name)).arg(right).arg(tabs->width())));
    }
    settings->setLanguage(QStringLiteral("en"));
}


void TestQmlConsultations::zoomScalesTheWindow_data() {
    opensWithANewConsultation_data();
}

// The whole window is its base layout scaled, the four buttons included.
void TestQmlConsultations::zoomScalesTheWindow() {
    QFETCH(int, row);
    open(screenData().at(row), 0, zoomtest::wideArea);
    const auto restore = qScopeGuard([this] { settings()->setLetterSize(13); });
    QQuickWindow* window = consultationWindow();
    QQuickItem* stage = zoomtest::stageOf(window);
    QVERIFY(stage != nullptr);
    const bool low = screenData().at(row).screenId == QLatin1String("pregnancy");
    QCOMPARE(window->size(), QSize(1020, low ? 388 : 608));
    QCOMPARE(stage->scale(), 1.0);

    QList<QQuickItem*> items;
    for (const char* name : {"consultationTitleBar", "exitConsultationButton",
                             "saveConsultationButton", "newConsultationButton",
                             "deleteConsultationButton", "consultationForm"})
        items.append(inConsultation(QLatin1String(name)));
    QVERIFY(!items.contains(nullptr));
    QList<QRectF> base;
    for (QQuickItem* item : items)
        base.append(zoomtest::sceneRect(item));

    settings()->setLetterSize(15);
    QTRY_COMPARE(window->size(), low ? QSize(1177, 448) : QSize(1177, 702));
    QCOMPARE(stage->scale(), 15.0 / 13.0);
    for (int i = 0; i < items.size(); ++i) {
        const QRectF now = zoomtest::sceneRect(items.at(i));
        QVERIFY2(zoomtest::sameScaled(base.at(i), now, 15.0 / 13.0),
                 qPrintable(QStringLiteral("%1: %2 from %3")
                                .arg(items.at(i)->objectName(), zoomtest::describe(now),
                                     zoomtest::describe(base.at(i)))));
    }
}

// A zoom made while the consultation window is closed (in a history window)
// centres it over the main window when it opens again.
void TestQmlConsultations::zoomElsewhereCentresTheWindowOnOpen() {
    const ScreenData data = screenData().at(0);
    open(data, 0, zoomtest::wideArea);
    const auto restore = qScopeGuard([this] { settings()->setLetterSize(13); });
    QQuickWindow* window = consultationWindow();
    QVERIFY(zoomtest::dragTo(m_window, QPoint(400, 200)));
    QVERIFY(zoomtest::dragTo(window, QPoint(10, 10)));
    window->close();
    QTRY_VERIFY(!window->isVisible());

    // Opened again with the same size: where it was left.
    openConsultation(data);
    QCOMPARE(window->position(), QPoint(10, 10));
    window->close();
    QTRY_VERIFY(!window->isVisible());

    settings()->setLetterSize(16);
    QCOMPARE(window->position(), QPoint(10, 10)); // hidden: not moved yet
    openConsultation(data);
    QTRY_COMPARE(window->size(), zoomtest::zoomOf(window)->windowSize());
    QTRY_VERIFY(zoomtest::centredOver(window, m_window));
}

void TestQmlConsultations::everyTextShowsWholeZoomed_data() {
    everyTextShowsWhole_data();
}

// Measured in the stage at 17 px, every text reads whole as at the base size.
void TestQmlConsultations::everyTextShowsWholeZoomed() {
    QFETCH(int, row);
    QFETCH(QString, language);
    open(screenData().at(row), 0, zoomtest::wideArea);
    const auto restore = qScopeGuard([this] {
        settings()->setLetterSize(13);
        settings()->setLanguage(QStringLiteral("en"));
    });
    settings()->setLanguage(language);
    settings()->setLetterSize(17);
    QQuickWindow* window = consultationWindow();
    QCOMPARE(zoomtest::zoomOf(window)->shownLetterSize(), 17);
    QTRY_COMPARE(window->size(), zoomtest::zoomOf(window)->windowSize());
    QTest::qWait(100);
    const QStringList cut = cutTextsOnEveryTab(zoomtest::stageOf(window));
    QVERIFY2(cut.isEmpty(), qPrintable(cut.join(QStringLiteral(" | "))));
}


// At 17 px, the diagnosis list of the pediatric consultation and the delete
// confirmation are scaled like the window: the list against its combo, the
// confirmation centred.
void TestQmlConsultations::popupsFollowTheZoom() {
    open(screenData().at(2), 0, zoomtest::wideArea); // pediatric
    const auto restore = qScopeGuard([this] { settings()->setLetterSize(13); });
    settings()->setLetterSize(17);
    QQuickWindow* window = consultationWindow();
    const QSizeF size(zoomtest::zoomOf(window)->windowSize());
    QTRY_COMPARE(window->contentItem()->size(), size);
    const qreal factor = 17.0 / 13.0;

    QQuickItem* combo = inConsultation(QStringLiteral("comboBox"));
    QVERIFY(combo != nullptr);
    auto* popup = combo->property("popup").value<QObject*>();
    QVERIFY(QMetaObject::invokeMethod(popup, "open"));
    QTRY_VERIFY(popup->property("opened").toBool());
    const QString problems = zoomtest::popupProblems(popup, combo, combo->width(), factor);
    QVERIFY2(problems.isEmpty(), qPrintable(problems));
    QVERIFY(QMetaObject::invokeMethod(popup, "close"));
    QTRY_VERIFY(!popup->property("visible").toBool());

    QObject* confirm = window->findChild<QObject*>(QStringLiteral("deleteConsultationConfirm"));
    QVERIFY(confirm != nullptr);
    confirm->setProperty("messageText", QStringLiteral("Delete?"));
    confirm->setProperty("yesNo", true);
    QVERIFY(QMetaObject::invokeMethod(confirm, "present"));
    QTRY_VERIFY(confirm->property("opened").toBool());
    const QRectF drawn = zoomtest::sceneRect(zoomtest::drawnItemOf(confirm));
    QVERIFY(qAbs(drawn.width() - 420 * factor) < 0.6);
    QVERIFY2(qAbs(drawn.center().x() - size.width() / 2.0) <= 1
                 && qAbs(drawn.center().y() - size.height() / 2.0) <= 1,
             qPrintable(zoomtest::describe(drawn)));
    QVERIFY(QMetaObject::invokeMethod(confirm, "close"));
}


// The body diagram is rendered at the size it is seen, so a zoomed window
// shows it sharp, not as an enlarged bitmap.
void TestQmlConsultations::bodyDiagramIsRenderedAtItsZoomedSize() {
    open(screenData().at(0), 0, zoomtest::wideArea); // adult
    const auto restore = qScopeGuard([this] { settings()->setLetterSize(13); });
    QQuickItem* image = inConsultation(QStringLiteral("bodyRegions"));
    QVERIFY(image != nullptr);
    QCOMPARE(image->property("sourceSize").toSize(), QSize(240, 295));
    settings()->setLetterSize(20);
    QTRY_COMPARE(consultationWindow()->size(), zoomtest::zoomOf(consultationWindow())->windowSize());
    QCOMPARE(image->property("sourceSize").toSize(),
             QSize(qCeil(240 * 20 / 13.0), qCeil(295 * 20 / 13.0)));
    // Laid out as at the base size: only the rendering grew.
    QCOMPARE(QSizeF(image->width(), image->height()), QSizeF(240, 295));
}

} // namespace gambasse

int main(int argc, char** argv) {
    QGuiApplication::setQuitOnLastWindowClosed(false);
    QGuiApplication app(argc, argv);
    gambasse::TestQmlConsultations test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_qmlconsultations.moc"
