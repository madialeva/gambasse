#include <QCoreApplication>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <ConsultationTestSupport.h>
#include <Paths.h>
#include <data/common/Database.h>
#include <logic/ConsultationService.h>

namespace gambasse {

// Verifies the adult consultation persistence: new consultations dated now,
// round trip of every managed column, list order, updates keyed by the stored
// timestamp, removal, same-second saves and legacy timestamp keys.
class TestAdultConsultationService : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    const qlonglong m_patientId = 5101;

    QList<AdultConsultation> stored() const;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void newConsultationIsDatedNow();
    void roundTripEveryColumn();
    void columnsMapToTheirSchemaColumns();
    void listIsMostRecentFirst();
    void updateKeepsTheTimestamp();
    void removeDeletesOnlyThatConsultation();
    void sameSecondSavesDoNotCollide();
    void legacyTimestampKeyIsFound();
};

QList<AdultConsultation> TestAdultConsultationService::stored() const {
    QList<AdultConsultation> list;
    AdultConsultationService().list(m_patientId, list);
    return list;
}

void TestAdultConsultationService::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));
    QVERIFY(insertPatientWithHistory(m_patientId, QStringLiteral("b03_adult_history")));
}

void TestAdultConsultationService::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestAdultConsultationService::cleanup() {
    QSqlQuery q(Database::instance().connection());
    QVERIFY(q.exec(QStringLiteral("DELETE FROM b06_adult_consultation")));
}

void TestAdultConsultationService::newConsultationIsDatedNow() {
    const QDateTime before = QDateTime::currentDateTime().addSecs(-1);
    const AdultConsultation c = AdultConsultationService().newConsultation(m_patientId);
    QCOMPARE(c.patientId, m_patientId);
    QVERIFY(!c.isStored());
    QVERIFY(c.date >= before && c.date <= QDateTime::currentDateTime());
    QCOMPARE(c.date.time().msec(), 0);
    QVERIFY(c.reason.isEmpty());
    QCOMPARE(c.lesionRegion[0], 0);
    QVERIFY(!c.irritable);
}

void TestAdultConsultationService::roundTripEveryColumn() {
    // The 44 fields of the form besides the date, plus the reason.
    QCOMPARE(consultationTable<AdultConsultation>().columns.size(), 45);
    AdultConsultationService service;
    AdultConsultation c = service.newConsultation(m_patientId);
    fillDistinct(c);
    QCOMPARE(service.save(c), AdultConsultationService::SaveResult::Saved);
    QVERIFY(c.isStored());
    const QList<AdultConsultation> list = stored();
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.first().date, c.date);
    QCOMPARE(list.first().originalKey, c.originalKey);
    compareColumns(list.first(), c);
}

void TestAdultConsultationService::columnsMapToTheirSchemaColumns() {
    AdultConsultationService service;
    AdultConsultation c = service.newConsultation(m_patientId);
    c.reason = QStringLiteral("Fever");
    c.lesionRegion[7] = 64;
    c.lesionImpairment[7] = QStringLiteral("Burn");
    c.otherCode = QStringLiteral("Refer");
    c.nasalFlaring = true;
    QCOMPARE(service.save(c), AdultConsultationService::SaveResult::Saved);
    const auto column = [&](const char* name) {
        return storedColumn(QStringLiteral("b06_adult_consultation"), QStringLiteral("date"),
                            m_patientId, c.originalKey, QLatin1String(name));
    };
    QCOMPARE(column("reason").toString(), QStringLiteral("Fever"));
    QCOMPARE(column("lesion_region8").toInt(), 64);
    QCOMPARE(column("lesion_impairment8").toString(), QStringLiteral("Burn"));
    QCOMPARE(column("physical_exam_other_code").toString(), QStringLiteral("Refer"));
    QCOMPARE(column("physical_exam_nasal_flaring").toInt(), 1);
    QCOMPARE(column("physical_exam_stridor").toInt(), 0);
}

void TestAdultConsultationService::listIsMostRecentFirst() {
    AdultConsultationService service;
    const QDateTime base(QDate(2026, 3, 1), QTime(9, 0));
    for (int day : {2, 5, 1}) {
        AdultConsultation c = service.newConsultation(m_patientId);
        c.date = base.addDays(day);
        c.reason = QStringLiteral("day %1").arg(day);
        QCOMPARE(service.save(c), AdultConsultationService::SaveResult::Saved);
    }
    const QList<AdultConsultation> list = stored();
    QCOMPARE(list.size(), 3);
    QCOMPARE(list.at(0).reason, QStringLiteral("day 5"));
    QCOMPARE(list.at(1).reason, QStringLiteral("day 2"));
    QCOMPARE(list.at(2).reason, QStringLiteral("day 1"));
}

void TestAdultConsultationService::updateKeepsTheTimestamp() {
    AdultConsultationService service;
    AdultConsultation c = service.newConsultation(m_patientId);
    c.reason = QStringLiteral("First");
    QCOMPARE(service.save(c), AdultConsultationService::SaveResult::Saved);
    AdultConsultation edited = stored().first();
    const QString key = edited.originalKey;
    edited.reason = QStringLiteral("Second");
    edited.cough = true;
    QCOMPARE(service.save(edited), AdultConsultationService::SaveResult::Saved);
    const QList<AdultConsultation> list = stored();
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.first().originalKey, key);
    QCOMPARE(list.first().reason, QStringLiteral("Second"));
    QVERIFY(list.first().cough);
}

void TestAdultConsultationService::removeDeletesOnlyThatConsultation() {
    AdultConsultationService service;
    AdultConsultation first = service.newConsultation(m_patientId);
    first.date = QDateTime(QDate(2026, 1, 1), QTime(8, 0));
    AdultConsultation second = service.newConsultation(m_patientId);
    second.date = QDateTime(QDate(2026, 1, 2), QTime(8, 0));
    QCOMPARE(service.save(first), AdultConsultationService::SaveResult::Saved);
    QCOMPARE(service.save(second), AdultConsultationService::SaveResult::Saved);
    QVERIFY(service.remove(first));
    const QList<AdultConsultation> list = stored();
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.first().originalKey, second.originalKey);
    // A consultation not saved yet cannot be deleted.
    QVERIFY(!service.remove(service.newConsultation(m_patientId)));
}

void TestAdultConsultationService::sameSecondSavesDoNotCollide() {
    AdultConsultationService service;
    const QDateTime moment(QDate(2026, 2, 2), QTime(10, 0, 0));
    AdultConsultation a = service.newConsultation(m_patientId);
    AdultConsultation b = service.newConsultation(m_patientId);
    a.date = moment;
    b.date = moment;
    QCOMPARE(service.save(a), AdultConsultationService::SaveResult::Saved);
    QCOMPARE(service.save(b), AdultConsultationService::SaveResult::Saved);
    QCOMPARE(b.date, moment.addSecs(1));
    QCOMPARE(stored().size(), 2);
}

void TestAdultConsultationService::legacyTimestampKeyIsFound() {
    // A row written by the legacy application: ISO "T" separator and seven
    // fractional digits, the format of every key in the production database.
    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "INSERT INTO b06_adult_consultation (patient_id, date, reason) VALUES (?, ?, ?)"));
    q.addBindValue(m_patientId);
    q.addBindValue(QStringLiteral("2019-07-08T11:12:13.4560000"));
    q.addBindValue(QStringLiteral("Legacy"));
    QVERIFY(q.exec());

    AdultConsultationService service;
    AdultConsultation legacy = stored().first();
    QCOMPARE(legacy.date, QDateTime(QDate(2019, 7, 8), QTime(11, 12, 13, 456)));
    QCOMPARE(legacy.date.date(), QDate(2019, 7, 8));
    legacy.reason = QStringLiteral("Legacy, edited");
    QCOMPARE(service.save(legacy), AdultConsultationService::SaveResult::Saved);
    QCOMPARE(stored().first().reason, QStringLiteral("Legacy, edited"));
    QCOMPARE(stored().first().originalKey, QStringLiteral("2019-07-08T11:12:13.4560000"));
    QVERIFY(service.remove(stored().first()));
    QVERIFY(stored().isEmpty());
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestAdultConsultationService test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_adultconsultationservice.moc"
