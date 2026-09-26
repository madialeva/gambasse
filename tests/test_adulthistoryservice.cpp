#include <QCoreApplication>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <data/common/Database.h>
#include <data/model/AdultHistory.h>
#include <data/model/Patient.h>
#include <logic/AdultHistoryService.h>

namespace gambasse {
namespace {

Patient makePatient(const QString& name) {
    Patient p;
    p.name = name;
    p.birthDate = QDate(1990, 5, 6);
    p.sex = Patient::Sex::Home;
    p.ageRange = 36;
    return p;
}

} // namespace

// Verifies the adult history round trip: new defaults, insert, reload,
// update and removal, plus language-independent persisted values.
class TestAdultHistoryService : public QObject {
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

void TestAdultHistoryService::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));

    Patient p = makePatient(QStringLiteral("AHISTORY_ADULT"));
    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "INSERT INTO b01_patient (id, name, birth_date, sex, approximate_age) "
        "VALUES (?,?,?,?,?)"));
    q.addBindValue(4244);
    q.addBindValue(p.name);
    q.addBindValue(p.birthDate.toString(QStringLiteral("yyyy-MM-dd")));
    q.addBindValue(static_cast<int>(p.sex));
    q.addBindValue(p.ageRange);
    QVERIFY(q.exec());
    m_patientId = 4244;
}

void TestAdultHistoryService::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestAdultHistoryService::newHistoryDefaults() {
    AdultHistoryService service;
    AdultHistory h;
    bool isNew = false;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(isNew);
    QCOMPARE(h.patientId, m_patientId);
    QCOMPARE(h.openingDate, QDate::currentDate());
    QVERIFY(!h.historyDiabetes);
    QVERIFY(!h.housingSanitaryControl);
    QCOMPARE(h.physicalExamWeight, 0.0);
}

void TestAdultHistoryService::createThenReload() {
    AdultHistoryService service;
    AdultHistory h;
    bool isNew = false;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(isNew);

    h.allergies = QStringLiteral("Penicillin");
    h.historyDiabetes = true;
    h.historyDiabetesTreatment = QStringLiteral("Metformin");
    h.historyHepatitisType = QStringLiteral("B");
    h.historyChildCount = 3;
    h.housingDomesticAnimals = PediatricHistory::DomesticAnimals::StableAndDwelling;
    h.housingSanitaryControl = true;
    h.physicalExamWeight = 72.5;
    h.physicalExamTemperature = 36.6;
    h.physicalExamBmi = 24.35;
    h.physicalExamConsciousness = QStringLiteral("Alert");
    QCOMPARE(service.save(isNew, h), AdultHistoryService::SaveResult::Saved);

    AdultHistory reloaded;
    bool reloadIsNew = true;
    QVERIFY(service.load(m_patientId, reloaded, reloadIsNew));
    QVERIFY(!reloadIsNew);
    QCOMPARE(reloaded.allergies, QStringLiteral("Penicillin"));
    QVERIFY(reloaded.historyDiabetes);
    QCOMPARE(reloaded.historyDiabetesTreatment, QStringLiteral("Metformin"));
    QCOMPARE(reloaded.historyHepatitisType, QStringLiteral("B"));
    QCOMPARE(reloaded.historyChildCount, 3);
    QCOMPARE(reloaded.housingDomesticAnimals,
             PediatricHistory::DomesticAnimals::StableAndDwelling);
    QVERIFY(reloaded.housingSanitaryControl);
    QCOMPARE(reloaded.physicalExamWeight, 72.5);
    QCOMPARE(reloaded.physicalExamTemperature, 36.6);
    QCOMPARE(reloaded.physicalExamBmi, 24.35);
    QCOMPARE(reloaded.physicalExamConsciousness, QStringLiteral("Alert"));
}

void TestAdultHistoryService::updateThenReload() {
    AdultHistoryService service;
    AdultHistory h;
    bool isNew = true;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(!isNew);

    h.historyDiabetes = false;
    h.physicalExamWeight = 73.0;
    QCOMPARE(service.save(isNew, h), AdultHistoryService::SaveResult::Saved);

    AdultHistory reloaded;
    QVERIFY(service.load(m_patientId, reloaded, isNew));
    QVERIFY(!isNew);
    QVERIFY(!reloaded.historyDiabetes);
    QCOMPARE(reloaded.physicalExamWeight, 73.0);
    // Untouched values survive the update.
    QCOMPARE(reloaded.allergies, QStringLiteral("Penicillin"));
    QVERIFY(reloaded.housingSanitaryControl);
}

void TestAdultHistoryService::removeDeletesHistory() {
    AdultHistoryService service;
    QVERIFY(service.remove(m_patientId));
    QVERIFY(!Database::instance().hasAdultHistory(m_patientId));

    AdultHistory h;
    bool isNew = false;
    QVERIFY(service.load(m_patientId, h, isNew));
    QVERIFY(isNew);
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestAdultHistoryService test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_adulthistoryservice.moc"
