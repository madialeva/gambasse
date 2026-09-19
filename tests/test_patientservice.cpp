#include <QCoreApplication>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <data/common/Database.h>
#include <data/model/Patient.h>
#include <logic/PatientService.h>

namespace gambasse {
namespace {

Patient makePatient(const QString& name) {
    Patient p;
    p.name = name;
    p.birthDate = QDate(2000, 1, 2);
    p.sex = Patient::Sex::Home;
    p.ageRange = 24;
    return p;
}

int patientCount() {
    QSqlQuery query(Database::instance().connection());
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM b01_paciente")) || !query.next())
        return -1;
    return query.value(0).toInt();
}

QString addressOf(qlonglong id) {
    QSqlQuery query(Database::instance().connection());
    query.prepare(QStringLiteral("SELECT b01_e_enderezo FROM b01_paciente WHERE b01_id=?"));
    query.addBindValue(id);
    if (!query.exec() || !query.next())
        return QString();
    return query.value(0).toString();
}

} // namespace

// Verifies patient validation, duplicate detection and the CRUD operations.
class TestPatientService : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;

private slots:
    void initTestCase();
    void cleanupTestCase();

    void nameIsRequired();
    void duplicateIsRejected();
    void createThenUpdate();
    void removeDeletesPatient();
};

void TestPatientService::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));
}

void TestPatientService::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestPatientService::nameIsRequired() {
    Patient p;
    QCOMPARE(PatientService().create(p), PatientService::SaveResult::NameRequired);
}

void TestPatientService::duplicateIsRejected() {
    PatientService service;
    Patient first = makePatient(QStringLiteral("PSERVICE_DUP"));
    QCOMPARE(service.create(first), PatientService::SaveResult::Saved);

    Patient second = makePatient(QStringLiteral("PSERVICE_DUP"));
    QCOMPARE(service.create(second), PatientService::SaveResult::Duplicate);
}

void TestPatientService::createThenUpdate() {
    PatientService service;
    Patient p = makePatient(QStringLiteral("PSERVICE_EDIT"));
    QCOMPARE(service.create(p), PatientService::SaveResult::Saved);
    QVERIFY(p.id > 0);

    const Patient previous = p;
    p.address = QStringLiteral("Rua X");
    QCOMPARE(service.update(previous, p), PatientService::SaveResult::Saved);
    QCOMPARE(p.id, previous.id);
    QCOMPARE(addressOf(p.id), QStringLiteral("Rua X"));
}

void TestPatientService::removeDeletesPatient() {
    PatientService service;
    Patient p = makePatient(QStringLiteral("PSERVICE_DEL"));
    QCOMPARE(service.create(p), PatientService::SaveResult::Saved);

    const int before = patientCount();
    QVERIFY(service.remove(p));
    QCOMPARE(patientCount(), before - 1);
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestPatientService test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_patientservice.moc"
