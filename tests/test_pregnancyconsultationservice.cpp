#include <QCoreApplication>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <ConsultationTestSupport.h>
#include <Paths.h>
#include <data/common/Database.h>
#include <logic/ConsultationService.h>

namespace gambasse {

// Verifies the pregnancy consultation persistence: round trip of every managed
// column, the x10 scaling of weight and temperature, list order, updates keyed
// by the stored timestamp and removal. The fixed rows of the window (iron,
// folic acid, deworming) have no column.
class TestPregnancyConsultationService : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    const qlonglong m_patientId = 5201;

    QList<PregnancyConsultation> stored() const;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void roundTripEveryColumn();
    void decimalsKeepTheLegacyScaling();
    void fixedRowsHaveNoColumn();
    void listIsMostRecentFirst();
    void updateKeepsTheTimestamp();
    void removeDeletesTheConsultation();
};

QList<PregnancyConsultation> TestPregnancyConsultationService::stored() const {
    QList<PregnancyConsultation> list;
    PregnancyConsultationService().list(m_patientId, list);
    return list;
}

void TestPregnancyConsultationService::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));
    QVERIFY(insertPatientWithHistory(m_patientId, QStringLiteral("b04_pregnancy_history")));
}

void TestPregnancyConsultationService::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestPregnancyConsultationService::cleanup() {
    QSqlQuery q(Database::instance().connection());
    QVERIFY(q.exec(QStringLiteral("DELETE FROM b07_pregnancy_consultation")));
}

void TestPregnancyConsultationService::roundTripEveryColumn() {
    // The 26 fields of the form besides the date, plus the reason.
    QCOMPARE(consultationTable<PregnancyConsultation>().columns.size(), 27);
    PregnancyConsultationService service;
    PregnancyConsultation c = service.newConsultation(m_patientId);
    fillDistinct(c);
    QCOMPARE(service.save(c), PregnancyConsultationService::SaveResult::Saved);
    const QList<PregnancyConsultation> list = stored();
    QCOMPARE(list.size(), 1);
    compareColumns(list.first(), c);
}

void TestPregnancyConsultationService::decimalsKeepTheLegacyScaling() {
    PregnancyConsultationService service;
    PregnancyConsultation c = service.newConsultation(m_patientId);
    c.weight = 72.5;
    c.temperature = 37.2;
    c.uterineHeightWeeks = 28;
    c.malariaProphylaxis.nextDose = QStringLiteral("In 4 weeks");
    c.edemaFace = true;
    QCOMPARE(service.save(c), PregnancyConsultationService::SaveResult::Saved);
    const auto column = [&](const char* name) {
        return storedColumn(QStringLiteral("b07_pregnancy_consultation"), QStringLiteral("date"),
                            m_patientId, c.originalKey, QLatin1String(name));
    };
    QCOMPARE(column("physical_exam_weight").toInt(), 725);
    QCOMPARE(column("physical_exam_temperature").toInt(), 372);
    QCOMPARE(column("physical_exam_uterine_height_weeks").toInt(), 28);
    QCOMPARE(column("supplement_malaria_prophylaxis_pd").toString(), QStringLiteral("In 4 weeks"));
    QCOMPARE(column("physical_exam_face").toInt(), 1);
    const PregnancyConsultation back = stored().first();
    QCOMPARE(back.weight, 72.5);
    QCOMPARE(back.temperature, 37.2);
}

void TestPregnancyConsultationService::fixedRowsHaveNoColumn() {
    for (const auto& column : consultationTable<PregnancyConsultation>().columns) {
        QVERIFY2(!column.name.contains(QStringLiteral("iron"))
                     && !column.name.contains(QStringLiteral("folic"))
                     && !column.name.contains(QStringLiteral("deworming")),
                 qPrintable(column.name));
    }
}

void TestPregnancyConsultationService::listIsMostRecentFirst() {
    PregnancyConsultationService service;
    for (int month : {3, 1, 2}) {
        PregnancyConsultation c = service.newConsultation(m_patientId);
        c.date = QDateTime(QDate(2026, month, 15), QTime(11, 0));
        c.reason = QStringLiteral("month %1").arg(month);
        QCOMPARE(service.save(c), PregnancyConsultationService::SaveResult::Saved);
    }
    const QList<PregnancyConsultation> list = stored();
    QCOMPARE(list.size(), 3);
    QCOMPARE(list.at(0).reason, QStringLiteral("month 3"));
    QCOMPARE(list.at(2).reason, QStringLiteral("month 1"));
}

void TestPregnancyConsultationService::updateKeepsTheTimestamp() {
    PregnancyConsultationService service;
    PregnancyConsultation c = service.newConsultation(m_patientId);
    QCOMPARE(service.save(c), PregnancyConsultationService::SaveResult::Saved);
    PregnancyConsultation edited = stored().first();
    edited.fetalPosition = QStringLiteral("Cephalic");
    QCOMPARE(service.save(edited), PregnancyConsultationService::SaveResult::Saved);
    QCOMPARE(stored().size(), 1);
    QCOMPARE(stored().first().originalKey, c.originalKey);
    QCOMPARE(stored().first().fetalPosition, QStringLiteral("Cephalic"));
}

void TestPregnancyConsultationService::removeDeletesTheConsultation() {
    PregnancyConsultationService service;
    PregnancyConsultation c = service.newConsultation(m_patientId);
    QCOMPARE(service.save(c), PregnancyConsultationService::SaveResult::Saved);
    QVERIFY(service.remove(stored().first()));
    QVERIFY(stored().isEmpty());
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestPregnancyConsultationService test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_pregnancyconsultationservice.moc"
