#include <QGuiApplication>
#include <QSignalSpy>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <data/common/Database.h>
#include <data/model/Patient.h>
#include <logic/PatientService.h>
#include <ui/controller/PatientController.h>

namespace gambasse {
namespace {

Patient makePatient(const QString& name, Patient::Sex sex = Patient::Sex::Home) {
    Patient p;
    p.name = name;
    p.sex = sex;
    p.birthDate = QDate(2000, 1, 2);
    p.ageRange = 24;
    p.address = QStringLiteral("rua x");
    return p;
}

// create() fills the identifier, so the patient is a named lvalue inside.
bool createPatient(PatientService& service, const QString& name,
                   Patient::Sex sex = Patient::Sex::Home) {
    Patient patient = makePatient(name, sex);
    return service.create(patient) == PatientService::SaveResult::Saved;
}

int storedCount() {
    QSqlQuery query(Database::instance().connection());
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM b01_patient")) || !query.next())
        return -1;
    return query.value(0).toInt();
}

// Cell as the grid shows it for the given proxy row.
QString gridText(PatientController& controller, int proxyRow, int column) {
    const QModelIndex idx = controller.gridModel()->index(proxyRow, column);
    return controller.gridModel()->data(idx, Qt::DisplayRole).toString();
}

constexpr int kSaved = static_cast<int>(PatientService::SaveResult::Saved);
constexpr int kNameRequired = static_cast<int>(PatientService::SaveResult::NameRequired);
constexpr int kDuplicate = static_cast<int>(PatientService::SaveResult::Duplicate);
constexpr int kAscending = static_cast<int>(Qt::AscendingOrder);
constexpr int kDescending = static_cast<int>(Qt::DescendingOrder);

} // namespace

// Verifies the QML-facing bridge of the patient screen: model exposure,
// selection, filtering, sorting, the edit session and the save/remove
// results, all delegating the rules to PatientService and ClinicalContext.
class TestPatientController : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    PatientService m_service;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void loadSortsByNameAndSelectsFirst();
    void columnsAndHeadersFollowLanguage();
    void selectionLoadsDetail();
    void selectionIsKeptValidWhenFiltering();
    void sortTogglesOrderOnRepeat();
    void addEditSaveAndReselect();
    void saveRejectsBlankNameAndDuplicates();
    void removeDeletesAndReselects();
    void historyAvailabilityFollowsSelection();
    void editingLocksTheSelection();
};

void TestPatientController::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));
}

// An empty table keeps every slot independent, also when an assertion returns
// early and would skip a per-slot cleanup.
void TestPatientController::init() {
    QSqlQuery erase(Database::instance().connection());
    QVERIFY2(erase.exec(QStringLiteral("DELETE FROM b01_patient")), qPrintable(erase.lastError().text()));
    QCOMPARE(storedCount(), 0);
}

void TestPatientController::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestPatientController::loadSortsByNameAndSelectsFirst() {
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_C")));
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_A")));
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_B")));

    PatientController controller;
    QSignalSpy sortSpy(&controller, &PatientController::sortChanged);
    QVERIFY(controller.load());
    QCOMPARE(controller.gridModel()->rowCount(), 3);
    QCOMPARE(controller.sortColumn(), PatientsModel::ColumnName);
    QCOMPARE(controller.sortOrder(), kAscending);
    QCOMPARE(sortSpy.count(), 1);
    QCOMPARE(gridText(controller, 0, PatientsModel::ColumnName), QStringLiteral("PCONTROL_A"));
    QCOMPARE(controller.currentRow(), 0);
    QVERIFY(controller.hasSelection());
    QCOMPARE(controller.patientName(), QStringLiteral("PCONTROL_A"));
}

void TestPatientController::columnsAndHeadersFollowLanguage() {
    PatientController controller;
    QVERIFY(controller.load());
    QCOMPARE(controller.columnCount(), PatientsModel::ColumnCount);
    QCOMPARE(controller.columnNames().size(), PatientsModel::ColumnCount);
    for (const QString& name : controller.columnNames())
        QVERIFY2(!name.isEmpty(), "every column has a translated header");

    // Sex labels and the weekday convention are exposed for the date input.
    QVERIFY(!controller.sexHomeLabel().isEmpty());
    QVERIFY(!controller.sexMullerLabel().isEmpty());
    QCOMPARE(controller.sexLabelText(static_cast<int>(Patient::Sex::Home)),
             controller.sexHomeLabel());
    QCOMPARE(controller.sexLabelText(static_cast<int>(Patient::Sex::Muller)),
             controller.sexMullerLabel());
    // Unknown codes fall back to Home, like the Widgets combo box.
    QCOMPARE(controller.sexLabelText(99), controller.sexHomeLabel());
}

void TestPatientController::selectionLoadsDetail() {
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_A_FIRST")));
    Patient patient = makePatient(QStringLiteral("PCONTROL_Z_DETAIL"), Patient::Sex::Muller);
    patient.address = QStringLiteral("rua detail");
    patient.cohabitants = QStringLiteral("4");
    patient.contactPerson = QStringLiteral("maria");
    patient.siblingCount = 3;
    QCOMPARE(static_cast<int>(m_service.create(patient)), kSaved);

    PatientController controller;
    QVERIFY(controller.load());
    // load() sorts by name, selects the first row and fills the detail.
    QCOMPARE(controller.currentRow(), 0);
    QCOMPARE(controller.patientName(), QStringLiteral("PCONTROL_A_FIRST"));

    QSignalSpy detailSpy(&controller, &PatientController::detailChanged);
    QSignalSpy currentSpy(&controller, &PatientController::currentChanged);

    // Selecting the row already current is a no-op, so nothing is emitted.
    controller.selectRow(0);
    QCOMPARE(currentSpy.count(), 0);
    QCOMPARE(detailSpy.count(), 0);

    controller.selectRow(1);
    QCOMPARE(currentSpy.count(), 1);
    QCOMPARE(detailSpy.count(), 1);
    QCOMPARE(controller.patientId(), patient.id);
    QCOMPARE(controller.patientSex(), static_cast<int>(Patient::Sex::Muller));
    QCOMPARE(controller.birthDateText(), QStringLiteral("02/01/2000"));
    QCOMPARE(controller.ageRange(), 24);
    QCOMPARE(controller.address(), QStringLiteral("rua detail"));
    QCOMPARE(controller.cohabitants(), QStringLiteral("4"));
    QCOMPARE(controller.contactPerson(), QStringLiteral("maria"));
    QCOMPARE(controller.siblingCount(), 3);
    QVERIFY(!controller.photoSource().isEmpty());

    // Out-of-range rows clear the selection instead of crashing.
    controller.selectRow(-1);
    QVERIFY(!controller.hasSelection());
    QCOMPARE(controller.currentRow(), -1);
    QCOMPARE(controller.patientId(), qlonglong(0));
    QCOMPARE(controller.patientName(), QString());
}

void TestPatientController::selectionIsKeptValidWhenFiltering() {
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_ALFA")));
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_BETA")));

    PatientController controller;
    QVERIFY(controller.load());
    QCOMPARE(controller.gridModel()->rowCount(), 2);

    // Filters are anchored at the start, so '*' widens the match.
    controller.setColumnFilter(PatientsModel::ColumnName, QStringLiteral("*BETA"));
    QCOMPARE(controller.gridModel()->rowCount(), 1);
    QVERIFY(controller.hasSelection());

    // A filter with no matches clears the selection and the detail.
    controller.setColumnFilter(PatientsModel::ColumnName, QStringLiteral("*ZZZ"));
    QCOMPARE(controller.gridModel()->rowCount(), 0);
    QVERIFY(!controller.hasSelection());
    QCOMPARE(controller.patientId(), qlonglong(0));

    controller.clearFilters();
    QCOMPARE(controller.gridModel()->rowCount(), 2);
    QCOMPARE(controller.currentRow(), 0);
    QVERIFY(controller.hasSelection());
}

void TestPatientController::sortTogglesOrderOnRepeat() {
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_ALFA")));
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_BETA")));

    PatientController controller;
    QVERIFY(controller.load());
    QCOMPARE(gridText(controller, 0, PatientsModel::ColumnName), QStringLiteral("PCONTROL_ALFA"));

    QSignalSpy sortSpy(&controller, &PatientController::sortChanged);
    controller.sortByColumn(PatientsModel::ColumnName); // same column -> descending
    QCOMPARE(sortSpy.count(), 1);
    QCOMPARE(controller.sortOrder(), kDescending);
    QCOMPARE(gridText(controller, 0, PatientsModel::ColumnName), QStringLiteral("PCONTROL_BETA"));

    controller.sortByColumn(PatientsModel::ColumnName); // and back to ascending
    QCOMPARE(controller.sortOrder(), kAscending);
    QCOMPARE(gridText(controller, 0, PatientsModel::ColumnName), QStringLiteral("PCONTROL_ALFA"));

    controller.sortByColumn(PatientsModel::ColumnAddress); // a new column resets the order
    QCOMPARE(controller.sortColumn(), PatientsModel::ColumnAddress);
    QCOMPARE(controller.sortOrder(), kAscending);
}

void TestPatientController::addEditSaveAndReselect() {
    PatientController controller;
    QVERIFY(controller.load());
    QCOMPARE(controller.gridModel()->rowCount(), 0);

    // Adding clears the fields but reserves the next identifier, like the
    // Widgets form shows in the read-only code field.
    controller.startAdd();
    QVERIFY(controller.editing());
    QVERIFY(controller.patientId() > 0);
    QVERIFY(controller.patientName().isEmpty());
    QCOMPARE(controller.birthDateText(), QStringLiteral("01/01/1900"));
    QCOMPARE(controller.ageRange(), 0);

    QSignalSpy editingSpy(&controller, &PatientController::editingChanged);
    QCOMPARE(controller.saveDraft(QStringLiteral("  PCONTROL_NEW  "),
                                  static_cast<int>(Patient::Sex::Muller),
                                  QStringLiteral("05/03/1999"), 30, QStringLiteral(" rua nova "),
                                  QStringLiteral("2"), QStringLiteral("joao"), 4),
             kSaved);
    QCOMPARE(editingSpy.count(), 1);
    QVERIFY(!controller.editing());
    QCOMPARE(controller.gridModel()->rowCount(), 1);
    // Values are trimmed and the saved patient becomes the selection.
    QCOMPARE(controller.patientName(), QStringLiteral("PCONTROL_NEW"));
    QCOMPARE(controller.patientSex(), static_cast<int>(Patient::Sex::Muller));
    QCOMPARE(controller.birthDateText(), QStringLiteral("05/03/1999"));
    QCOMPARE(controller.address(), QStringLiteral("rua nova"));
    QCOMPARE(controller.siblingCount(), 4);

    // Editing the selection and saving updates it in place.
    controller.startEdit();
    QVERIFY(controller.editing());
    QCOMPARE(controller.saveDraft(QStringLiteral("PCONTROL_RENAMED"),
                                  static_cast<int>(Patient::Sex::Home),
                                  QStringLiteral("05/03/1999"), 31, QStringLiteral("rua nova"),
                                  QStringLiteral("2"), QStringLiteral("joao"), 4),
             kSaved);
    QCOMPARE(controller.gridModel()->rowCount(), 1);
    QCOMPARE(controller.patientName(), QStringLiteral("PCONTROL_RENAMED"));
    QCOMPARE(controller.patientSex(), static_cast<int>(Patient::Sex::Home));
    QCOMPARE(controller.ageRange(), 31);
}

void TestPatientController::saveRejectsBlankNameAndDuplicates() {
    PatientController controller;
    QVERIFY(controller.load());

    controller.startAdd();
    QCOMPARE(controller.saveDraft(QStringLiteral("   "), static_cast<int>(Patient::Sex::Home),
                                  QStringLiteral("01/01/1990"), 10, QString(), QString(),
                                  QString(), 0),
             kNameRequired);
    // A rejected save keeps the form open for correction.
    QVERIFY(controller.editing());
    QCOMPARE(storedCount(), 0);

    QCOMPARE(controller.saveDraft(QStringLiteral("PCONTROL_DUP"),
                                  static_cast<int>(Patient::Sex::Home),
                                  QStringLiteral("01/01/1990"), 10, QString(), QString(),
                                  QString(), 0),
             kSaved);
    QCOMPARE(storedCount(), 1);

    controller.startAdd();
    QCOMPARE(controller.saveDraft(QStringLiteral("PCONTROL_DUP"),
                                  static_cast<int>(Patient::Sex::Home),
                                  QStringLiteral("01/01/1990"), 10, QString(), QString(),
                                  QString(), 0),
             kDuplicate);
    QVERIFY(controller.editing());
    QCOMPARE(storedCount(), 1);

    // An invalid birth date and out-of-range numbers are coerced, not rejected.
    controller.cancelEdit();
    QVERIFY(!controller.editing());
    controller.startAdd();
    QCOMPARE(controller.saveDraft(QStringLiteral("PCONTROL_COERCED"),
                                  static_cast<int>(Patient::Sex::Home),
                                  QStringLiteral("not a date"), 500, QString(), QString(),
                                  QString(), -3),
             kSaved);
    QCOMPARE(controller.birthDateText(), QStringLiteral("01/01/1900"));
    QCOMPARE(controller.ageRange(), 130);
    QCOMPARE(controller.siblingCount(), 0);
}

void TestPatientController::removeDeletesAndReselects() {
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_R1")));
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_R2")));

    PatientController controller;
    QVERIFY(controller.load());
    QCOMPARE(controller.gridModel()->rowCount(), 2);

    QVERIFY(controller.removeCurrent());
    QCOMPARE(storedCount(), 1);
    QCOMPARE(controller.gridModel()->rowCount(), 1);
    // The remaining row takes the selection.
    QCOMPARE(controller.patientName(), QStringLiteral("PCONTROL_R2"));

    QVERIFY(controller.removeCurrent());
    QCOMPARE(storedCount(), 0);
    QCOMPARE(controller.gridModel()->rowCount(), 0);
    QVERIFY(!controller.hasSelection());

    // Removing without a selection is a no-op, not a failure path.
    QVERIFY(!controller.removeCurrent());
}

void TestPatientController::historyAvailabilityFollowsSelection() {
    Patient adult = makePatient(QStringLiteral("PCONTROL_H_F"), Patient::Sex::Muller);
    adult.ageRange = 24;
    QCOMPARE(static_cast<int>(m_service.create(adult)), kSaved);
    Patient male = makePatient(QStringLiteral("PCONTROL_H_M"), Patient::Sex::Home);
    male.ageRange = 24;
    QCOMPARE(static_cast<int>(m_service.create(male)), kSaved);

    PatientController controller;
    QVERIFY(controller.load());
    QSignalSpy availabilitySpy(&controller, &PatientController::availabilityChanged);

    // Sorted by name, row 0 is the female patient. Without existing histories
    // the "show" flags stay off while the adult and pregnancy entries open.
    controller.selectRow(0);
    QCOMPARE(controller.patientSex(), static_cast<int>(Patient::Sex::Muller));
    QVERIFY(!controller.showPediatric());
    QVERIFY(!controller.showAdult());
    QVERIFY(!controller.showPregnancy());
    QVERIFY(!controller.canCreatePediatric());
    QVERIFY(controller.canCreateAdult());
    QVERIFY(controller.canCreatePregnancy());

    // A Home patient cannot open the pregnancy history.
    controller.selectRow(1);
    QVERIFY(availabilitySpy.count() >= 1);
    QCOMPARE(controller.patientSex(), static_cast<int>(Patient::Sex::Home));
    QVERIFY(controller.canCreateAdult());
    QVERIFY(!controller.canCreatePregnancy());

    // With no selection at all, nothing is available.
    controller.selectRow(-1);
    QVERIFY(!controller.showPediatric());
    QVERIFY(!controller.showAdult());
    QVERIFY(!controller.showPregnancy());
    QVERIFY(!controller.canCreatePediatric());
    QVERIFY(!controller.canCreateAdult());
    QVERIFY(!controller.canCreatePregnancy());
}

void TestPatientController::editingLocksTheSelection() {
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_E1")));
    QVERIFY(createPatient(m_service, QStringLiteral("PCONTROL_E2")));

    PatientController controller;
    QVERIFY(controller.load());
    const int before = controller.currentRow();
    const int other = (before == 0) ? 1 : 0;
    QVERIFY(controller.canCreateAdult());

    controller.startAdd();
    QVERIFY(controller.editing());
    // The grid row stays selected (like the Widgets window), but the detail is
    // empty and no history entry applies to a patient that does not exist yet.
    controller.selectRow(other);
    QCOMPARE(controller.currentRow(), before);
    QVERIFY(controller.patientId() > 0); // the identifier to be assigned
    QVERIFY(controller.patientName().isEmpty());
    QVERIFY(!controller.canCreatePediatric());
    QVERIFY(!controller.canCreateAdult());
    QVERIFY(!controller.canCreatePregnancy());

    controller.startEdit();
    QVERIFY(controller.editing());
    controller.selectRow(other);
    QCOMPARE(controller.currentRow(), before);
    QCOMPARE(controller.patientName(), gridText(controller, before, PatientsModel::ColumnName));

    // Cancelling restores the detail and the history entries of the selection.
    controller.cancelEdit();
    QVERIFY(!controller.editing());
    QCOMPARE(controller.patientName(), gridText(controller, before, PatientsModel::ColumnName));
    QVERIFY(controller.canCreateAdult());
    controller.selectRow(other);
    QCOMPARE(controller.currentRow(), other); // unlocked again
}

} // namespace gambasse

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    gambasse::TestPatientController test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_patientcontroller.moc"
