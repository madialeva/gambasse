#include <QCoreApplication>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <ConsultationTestSupport.h>
#include <Paths.h>
#include <data/common/Database.h>
#include <logic/ConsultationService.h>

namespace gambasse {

// Verifies the pediatric consultation persistence: round trip of every managed
// column (the ten treatment rows and five recommendations included), the
// legacy scaling, the cough check bound to its column, columns the window
// does not manage kept on update, list order and removal.
class TestPediatricConsultationService : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    const qlonglong m_patientId = 5301;

    QList<PediatricConsultation> stored() const;
    QVariant column(const PediatricConsultation& c, const char* name) const;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void roundTripEveryColumn();
    void columnsMapToTheirSchemaColumns();
    void unmanagedColumnsSurviveAnUpdate();
    void listIsMostRecentFirst();
    void removeDeletesTheConsultation();
};

QList<PediatricConsultation> TestPediatricConsultationService::stored() const {
    QList<PediatricConsultation> list;
    PediatricConsultationService().list(m_patientId, list);
    return list;
}

QVariant TestPediatricConsultationService::column(const PediatricConsultation& c,
                                                  const char* name) const {
    return storedColumn(QStringLiteral("b08_pediatric_consultation"),
                        QStringLiteral("consultation_date"), m_patientId, c.originalKey,
                        QLatin1String(name));
}

void TestPediatricConsultationService::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));
    QVERIFY(insertPatientWithHistory(m_patientId, QStringLiteral("b05_pediatric_history")));
}

void TestPediatricConsultationService::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestPediatricConsultationService::cleanup() {
    QSqlQuery q(Database::instance().connection());
    QVERIFY(q.exec(QStringLiteral("DELETE FROM b08_pediatric_consultation")));
}

void TestPediatricConsultationService::roundTripEveryColumn() {
    // Every control of the VB.NET form but name and date: 58 inputs, 39 checks
    // (the cough one included) and 64 combos. That is the reason, 2 diagnosis
    // fields, 83 exam fields, 10 x 7 treatment fields and 5 recommendations.
    QCOMPARE(consultationTable<PediatricConsultation>().columns.size(), 58 + 39 + 64);
    QCOMPARE(consultationTable<PediatricConsultation>().columns.size(), 1 + 2 + 83 + 70 + 5);
    PediatricConsultationService service;
    PediatricConsultation c = service.newConsultation(m_patientId);
    fillDistinct(c);
    QCOMPARE(service.save(c), PediatricConsultationService::SaveResult::Saved);
    const QList<PediatricConsultation> list = stored();
    QCOMPARE(list.size(), 1);
    compareColumns(list.first(), c);
}

void TestPediatricConsultationService::columnsMapToTheirSchemaColumns() {
    PediatricConsultationService service;
    PediatricConsultation c = service.newConsultation(m_patientId);
    c.weight = 12.34;
    c.temperature = 38.5;
    c.cough = true;
    c.coughDays = 3;
    c.lymphadenopathy = QStringLiteral("Cervical");
    c.treatments[9].medication = 1001;
    c.treatments[9].perfusion = 250;
    c.treatments[0].dose = QStringLiteral("5 ml");
    c.recommendations[4] = 13;
    c.nursingDiagnosis = 7;
    QCOMPARE(service.save(c), PediatricConsultationService::SaveResult::Saved);
    QCOMPARE(column(c, "physical_exam_weight").toInt(), 1234);
    QCOMPARE(column(c, "physical_exam_temperature").toInt(), 385);
    QCOMPARE(column(c, "physical_exam_cough").toInt(), 1);
    QCOMPARE(column(c, "physical_exam_respiratory_duration").toInt(), 3);
    QCOMPARE(column(c, "physical_exam_lymphadenopathy_description").toString(),
             QStringLiteral("Cervical"));
    QCOMPARE(column(c, "treatment_medication10").toInt(), 1001);
    QCOMPARE(column(c, "treatment_perfusion10").toInt(), 250);
    QCOMPARE(column(c, "treatment_dose1").toString(), QStringLiteral("5 ml"));
    QCOMPARE(column(c, "recommendation5").toInt(), 13);
    QCOMPARE(column(c, "nursing_diagnosis").toInt(), 7);
    const PediatricConsultation back = stored().first();
    QCOMPARE(back.weight, 12.34);
    QCOMPARE(back.temperature, 38.5);
}

void TestPediatricConsultationService::unmanagedColumnsSurviveAnUpdate() {
    PediatricConsultationService service;
    PediatricConsultation c = service.newConsultation(m_patientId);
    QCOMPARE(service.save(c), PediatricConsultationService::SaveResult::Saved);
    // Legacy columns no window manages (supplements, lesions, text vitals).
    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "UPDATE b08_pediatric_consultation SET supplement_iron_d = 'legacy', lesion_region3 = 12, "
        "physical_exam_heart_rate = 'fast' WHERE patient_id = ?"));
    q.addBindValue(m_patientId);
    QVERIFY(q.exec());

    PediatricConsultation edited = stored().first();
    edited.reason = QStringLiteral("Follow-up");
    QCOMPARE(service.save(edited), PediatricConsultationService::SaveResult::Saved);
    QCOMPARE(column(edited, "reason").toString(), QStringLiteral("Follow-up"));
    QCOMPARE(column(edited, "supplement_iron_d").toString(), QStringLiteral("legacy"));
    QCOMPARE(column(edited, "lesion_region3").toInt(), 12);
    QCOMPARE(column(edited, "physical_exam_heart_rate").toString(), QStringLiteral("fast"));
}

void TestPediatricConsultationService::listIsMostRecentFirst() {
    PediatricConsultationService service;
    for (int hour : {9, 17, 12}) {
        PediatricConsultation c = service.newConsultation(m_patientId);
        c.date = QDateTime(QDate(2026, 5, 5), QTime(hour, 0));
        c.reason = QStringLiteral("at %1").arg(hour);
        QCOMPARE(service.save(c), PediatricConsultationService::SaveResult::Saved);
    }
    const QList<PediatricConsultation> list = stored();
    QCOMPARE(list.size(), 3);
    QCOMPARE(list.at(0).reason, QStringLiteral("at 17"));
    QCOMPARE(list.at(1).reason, QStringLiteral("at 12"));
    QCOMPARE(list.at(2).reason, QStringLiteral("at 9"));
}

void TestPediatricConsultationService::removeDeletesTheConsultation() {
    PediatricConsultationService service;
    PediatricConsultation c = service.newConsultation(m_patientId);
    QCOMPARE(service.save(c), PediatricConsultationService::SaveResult::Saved);
    QVERIFY(service.remove(stored().first()));
    QVERIFY(stored().isEmpty());
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestPediatricConsultationService test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_pediatricconsultationservice.moc"
