#include <QCoreApplication>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <data/common/Database.h>
#include <data/model/Patient.h>
#include <data/model/PediatricHistory.h>
#include <logic/PediatricHistoryService.h>

namespace gambasse {
namespace {

Patient makePatient(const QString& name) {
    Patient p;
    p.name = name;
    p.birthDate = QDate(2018, 5, 6);
    p.sex = Patient::Sex::Home;
    p.ageRange = 8;
    return p;
}

} // namespace

// Verifies the pediatric history round trip: new defaults, insert, reload,
// update and removal, plus language-independent persisted values.
class TestPediatricHistoryService : public QObject {
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

void TestPediatricHistoryService::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));

    Patient p = makePatient(QStringLiteral("PHISTORY_KID"));
    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "INSERT INTO b01_patient (id, name, birth_date, sex, approximate_age) "
        "VALUES (?,?,?,?,?)"));
    q.addBindValue(4242);
    q.addBindValue(p.name);
    q.addBindValue(p.birthDate.toString(QStringLiteral("yyyy-MM-dd")));
    q.addBindValue(static_cast<int>(p.sex));
    q.addBindValue(p.ageRange);
    QVERIFY(q.exec());
    m_patientId = 4242;
}

void TestPediatricHistoryService::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestPediatricHistoryService::newHistoryDefaults() {
    PediatricHistoryService service;
    PediatricHistory h;
    bool isNew = false;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(isNew);
    QCOMPARE(h.patientId, m_patientId);
    QCOMPARE(h.openingDate, QDate::currentDate());
    QVERIFY(!h.vaccineBcg);
    QCOMPARE(h.supplementTreatment1, PediatricHistory::TreatmentType::None);
    QVERIFY(!h.supplementDate1.isValid());
}

void TestPediatricHistoryService::createThenReload() {
    PediatricHistoryService service;
    PediatricHistory h;
    bool isNew = false;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(isNew);

    h.allergies = QStringLiteral("Penicillin");
    h.neonatalWeight = 3200;
    h.neonatalHeight = 49;
    h.vaccineBcg = true;
    h.vaccineOpv1 = true;
    h.vaccineMeasles9 = true;
    h.vaccineMeasles10 = false;
    h.domesticAnimals = PediatricHistory::DomesticAnimals::Dwelling;
    h.supplementTreatment1 = PediatricHistory::TreatmentType::VitaminA;
    h.supplementDate1 = QDate(2024, 3, 4);
    h.economicSituation = QStringLiteral("Stable");
    QCOMPARE(service.save(isNew, h), PediatricHistoryService::SaveResult::Saved);

    PediatricHistory reloaded;
    bool reloadIsNew = true;
    QVERIFY(service.load(m_patientId, reloaded, reloadIsNew));
    QVERIFY(!reloadIsNew);
    QCOMPARE(reloaded.allergies, QStringLiteral("Penicillin"));
    QCOMPARE(reloaded.neonatalWeight, 3200);
    QCOMPARE(reloaded.neonatalHeight, 49);
    QVERIFY(reloaded.vaccineBcg);
    QVERIFY(reloaded.vaccineOpv1);
    QVERIFY(!reloaded.vaccineOpv2);
    QVERIFY(reloaded.vaccineMeasles9);
    QVERIFY(!reloaded.vaccineMeasles10);
    QCOMPARE(reloaded.domesticAnimals, PediatricHistory::DomesticAnimals::Dwelling);
    QCOMPARE(reloaded.supplementTreatment1, PediatricHistory::TreatmentType::VitaminA);
    QCOMPARE(reloaded.supplementDate1, QDate(2024, 3, 4));
    QVERIFY(!reloaded.supplementDate2.isValid());
    QCOMPARE(reloaded.economicSituation, QStringLiteral("Stable"));
}

void TestPediatricHistoryService::updateThenReload() {
    PediatricHistoryService service;
    PediatricHistory h;
    bool isNew = true;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(!isNew);

    h.vaccineBcg = false;
    h.supplementTreatment1 = PediatricHistory::TreatmentType::Iron;
    QCOMPARE(service.save(isNew, h), PediatricHistoryService::SaveResult::Saved);

    PediatricHistory reloaded;
    QVERIFY(service.load(m_patientId, reloaded, isNew));
    QVERIFY(!isNew);
    QVERIFY(!reloaded.vaccineBcg);
    QCOMPARE(reloaded.supplementTreatment1, PediatricHistory::TreatmentType::Iron);
    // Untouched values survive the update.
    QCOMPARE(reloaded.allergies, QStringLiteral("Penicillin"));
    QVERIFY(reloaded.vaccineMeasles9);
}

void TestPediatricHistoryService::removeDeletesHistory() {
    PediatricHistoryService service;
    QVERIFY(service.remove(m_patientId));
    QVERIFY(!Database::instance().hasPediatricHistory(m_patientId));

    PediatricHistory h;
    bool isNew = false;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(isNew);
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestPediatricHistoryService test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_pediatricehistoryservice.moc"
