#include <QCoreApplication>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <Paths.h>
#include <data/common/Database.h>
#include <data/model/Patient.h>
#include <logic/ClinicalContext.h>

namespace gambasse {

// Verifies the history/consultation availability rules by age, sex and the
// histories that already exist.
class TestClinicalContext : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;

private slots:
    void initTestCase();
    void cleanupTestCase();

    void noPatient();
    void childOnlyPediatric();
    void adultOnlyAdult();
    void adultWomanCanBePregnant();
    void existingHistoryDisablesCreation();
};

void TestClinicalContext::initTestCase() {
    QVERIFY(m_dir.isValid());
    setBasePath(m_dir.path());
    QString error;
    QVERIFY2(Database::instance().open(&error), qPrintable(error));
}

void TestClinicalContext::cleanupTestCase() {
    Database::instance().connection().close();
}

void TestClinicalContext::noPatient() {
    const Availability a = ClinicalContext().availability(nullptr);
    QVERIFY(!a.showPediatric && !a.showAdult && !a.showPregnancy);
    QVERIFY(!a.canCreatePediatric && !a.canCreateAdult && !a.canCreatePregnancy);
}

void TestClinicalContext::childOnlyPediatric() {
    Patient p;
    p.ageRange = 5;

    const Availability a = ClinicalContext().availability(&p);
    QVERIFY(a.canCreatePediatric);
    QVERIFY(!a.canCreateAdult);
    QVERIFY(!a.canCreatePregnancy);
}

void TestClinicalContext::adultOnlyAdult() {
    Patient p;
    p.ageRange = 30;
    p.sex = Patient::Sex::Home;

    const Availability a = ClinicalContext().availability(&p);
    QVERIFY(!a.canCreatePediatric);
    QVERIFY(a.canCreateAdult);
    QVERIFY(!a.canCreatePregnancy);
}

void TestClinicalContext::adultWomanCanBePregnant() {
    Patient p;
    p.ageRange = 30;
    p.sex = Patient::Sex::Muller;

    const Availability a = ClinicalContext().availability(&p);
    QVERIFY(a.canCreateAdult);
    QVERIFY(a.canCreatePregnancy);
}

void TestClinicalContext::existingHistoryDisablesCreation() {
    const qlonglong id = 9001;
    QSqlQuery create(Database::instance().connection());
    QVERIFY(create.exec(QStringLiteral(
        "INSERT INTO b01_patient (id, name) VALUES (9001, 'HIST')")));
    QVERIFY(create.exec(QStringLiteral(
        "INSERT INTO b05_pediatric_history (patient_id) VALUES (9001)")));

    Patient p;
    p.id = id;
    p.ageRange = 5;

    const Availability a = ClinicalContext().availability(&p);
    QVERIFY(a.showPediatric);
    QVERIFY(!a.canCreatePediatric);
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestClinicalContext test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_clinicalcontext.moc"
