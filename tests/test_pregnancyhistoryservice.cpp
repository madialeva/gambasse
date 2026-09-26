#include <QCoreApplication>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <data/common/Database.h>
#include <data/model/Patient.h>
#include <data/model/PregnancyHistory.h>
#include <logic/PregnancyHistoryService.h>

namespace gambasse {
namespace {

Patient makePatient(const QString& name) {
    Patient p;
    p.name = name;
    p.birthDate = QDate(2000, 5, 6);
    p.sex = Patient::Sex::Muller;
    p.ageRange = 26;
    return p;
}

} // namespace

// Verifies the pregnancy history round trip: new defaults, insert, reload,
// update and removal, plus nullable dates and language-independent values.
class TestPregnancyHistoryService : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    qlonglong m_patientId = 0;

private slots:
    void initTestCase();
    void cleanupTestCase();

    void newHistoryDefaults();
    void createThenReload();
    void updateThenReload();
    void removeDeletesHistory();
};

void TestPregnancyHistoryService::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));

    Patient p = makePatient(QStringLiteral("PHISTORY_PREGNANT"));
    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "INSERT INTO b01_patient (id, name, birth_date, sex, approximate_age) "
        "VALUES (?,?,?,?,?)"));
    q.addBindValue(4245);
    q.addBindValue(p.name);
    q.addBindValue(p.birthDate.toString(QStringLiteral("yyyy-MM-dd")));
    q.addBindValue(static_cast<int>(p.sex));
    q.addBindValue(p.ageRange);
    QVERIFY(q.exec());
    m_patientId = 4245;
}

void TestPregnancyHistoryService::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestPregnancyHistoryService::newHistoryDefaults() {
    PregnancyHistoryService service;
    PregnancyHistory h;
    bool isNew = false;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(isNew);
    QCOMPARE(h.patientId, m_patientId);
    QCOMPARE(h.openingDate, QDate::currentDate());
    QVERIFY(!h.obstetricLastMenstruation.isValid());
    QVERIFY(!h.supplementDate1.isValid());
    QVERIFY(h.supplementCalciumD.isEmpty());
}

void TestPregnancyHistoryService::createThenReload() {
    PregnancyHistoryService service;
    PregnancyHistory h;
    bool isNew = false;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(isNew);

    h.allergies = QStringLiteral("None known");
    h.obstetricLastMenstruation = QDate(2025, 11, 3);
    h.obstetricExpectedDelivery = QDate(2026, 8, 10);
    h.obstetricDeliveryCount = 1;
    h.obstetricHivWoman = false;
    h.obstetricHivMan = true;
    h.medicalHypertension = true;
    h.medicalHypertensionTreatment = QStringLiteral("Methyldopa");
    h.housingDomesticAnimals = PediatricHistory::DomesticAnimals::Dwelling;
    h.housingMosquitoNetChildren = true;
    h.physicalExamWeight = 68.5;
    h.physicalExamUterineHeightWeeks = 24;
    h.supplementDate1 = QDate(2026, 1, 12);
    h.supplementMedication1 = QStringLiteral("Folic acid");
    h.supplementDose1 = QStringLiteral("4 mg a day");
    h.supplementCalciumD = QStringLiteral("500 mg");
    h.supplementMalariaProphylaxisPd = QStringLiteral("SP dose 2");
    QCOMPARE(service.save(isNew, h), PregnancyHistoryService::SaveResult::Saved);

    PregnancyHistory reloaded;
    bool reloadIsNew = true;
    QVERIFY(service.load(m_patientId, reloaded, reloadIsNew));
    QVERIFY(!reloadIsNew);
    QCOMPARE(reloaded.allergies, QStringLiteral("None known"));
    QCOMPARE(reloaded.obstetricLastMenstruation, QDate(2025, 11, 3));
    QCOMPARE(reloaded.obstetricExpectedDelivery, QDate(2026, 8, 10));
    QCOMPARE(reloaded.obstetricDeliveryCount, 1);
    QVERIFY(!reloaded.obstetricHivWoman);
    QVERIFY(reloaded.obstetricHivMan);
    QVERIFY(reloaded.medicalHypertension);
    QCOMPARE(reloaded.medicalHypertensionTreatment, QStringLiteral("Methyldopa"));
    QCOMPARE(reloaded.housingDomesticAnimals,
             PediatricHistory::DomesticAnimals::Dwelling);
    QVERIFY(reloaded.housingMosquitoNetChildren);
    QCOMPARE(reloaded.physicalExamWeight, 68.5);
    QCOMPARE(reloaded.physicalExamUterineHeightWeeks, 24);
    QCOMPARE(reloaded.supplementDate1, QDate(2026, 1, 12));
    QVERIFY(!reloaded.supplementDate2.isValid());
    QCOMPARE(reloaded.supplementMedication1, QStringLiteral("Folic acid"));
    QCOMPARE(reloaded.supplementDose1, QStringLiteral("4 mg a day"));
    QCOMPARE(reloaded.supplementCalciumD, QStringLiteral("500 mg"));
    QCOMPARE(reloaded.supplementMalariaProphylaxisPd, QStringLiteral("SP dose 2"));
}

void TestPregnancyHistoryService::updateThenReload() {
    PregnancyHistoryService service;
    PregnancyHistory h;
    bool isNew = true;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(!isNew);

    h.medicalHypertension = false;
    h.supplementMalariaProphylaxisPd = QStringLiteral("SP dose 3");
    QCOMPARE(service.save(isNew, h), PregnancyHistoryService::SaveResult::Saved);

    PregnancyHistory reloaded;
    QVERIFY(service.load(m_patientId, reloaded, isNew));
    QVERIFY(!isNew);
    QVERIFY(!reloaded.medicalHypertension);
    QCOMPARE(reloaded.supplementMalariaProphylaxisPd, QStringLiteral("SP dose 3"));
    // Untouched values survive the update.
    QCOMPARE(reloaded.allergies, QStringLiteral("None known"));
    QCOMPARE(reloaded.obstetricExpectedDelivery, QDate(2026, 8, 10));
}

void TestPregnancyHistoryService::removeDeletesHistory() {
    PregnancyHistoryService service;
    QVERIFY(service.remove(m_patientId));
    QVERIFY(!Database::instance().hasPregnancyHistory(m_patientId));

    PregnancyHistory h;
    bool isNew = false;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(isNew);
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestPregnancyHistoryService test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_pregnancyhistoryservice.moc"
