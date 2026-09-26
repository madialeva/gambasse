#include <data/common/Database.h>

#include <Paths.h>
#include <data/common/SchemaMigrator.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaType>
#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtMath>

namespace gambasse {

Database& Database::instance() {
    static Database inst;
    return inst;
}

bool Database::open(QString* errorMessage) {
    // Database path: config.ini at the base path, under [database] path.
    // The default is "database.db" relative to the base path.
    const QString baseDirectory = basePath();
    const QString configPath = QDir(baseDirectory).filePath(QStringLiteral("config.ini"));
    QSettings cfg(configPath, QSettings::IniFormat);
    QString databasePath = cfg.value(QStringLiteral("database/path"),
                                     QStringLiteral("database.db")).toString();

    // Resolve relative paths from the base path.
    if (QFileInfo(databasePath).isRelative())
        databasePath = QDir(baseDirectory).filePath(databasePath);

    // The database file is created on first run; make sure its directory exists.
    const QString databaseDirectory = QFileInfo(databasePath).absolutePath();
    if (!QDir().mkpath(databaseDirectory)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Could not create the database directory:\n%1")
                            .arg(databaseDirectory);
        return false;
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    m_db.setDatabaseName(databasePath);
    // Fixed connection credentials for now. The stock SQLite driver ignores
    // them, but the connection is ready for the future first-run mechanism.
    m_db.setUserName(cfg.value(QStringLiteral("database/user"),
                               QStringLiteral("gambasse")).toString());
    m_db.setPassword(cfg.value(QStringLiteral("database/password"),
                               QStringLiteral("gambasse")).toString());
    if (!m_db.open()) {
        if (errorMessage)
            *errorMessage = QObject::tr("Could not open the database:\n%1")
                            .arg(m_db.lastError().text());
        return false;
    }

    // SQLite requires foreign keys to be enabled per connection for cascade
    // deletion (FK ON DELETE CASCADE) to work.
    QSqlQuery(m_db).exec(QStringLiteral("PRAGMA foreign_keys = ON"));

    // Create the database or bring its schema up to date before it is used.
    if (!SchemaMigrator::migrate(m_db, errorMessage)) {
        m_db.close();
        return false;
    }
    return true;
}

bool Database::isOpen() const {
    return m_db.isOpen();
}

bool Database::exists(const QString& table, const QString& foreignKeyColumn, qlonglong id) const {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT 1 FROM %1 WHERE %2 = ? LIMIT 1").arg(table, foreignKeyColumn));
    q.addBindValue(id);
    if (!q.exec())
        return false;
    return q.next();
}

bool Database::hasPediatricHistory(qlonglong patientId) const {
    return exists(QStringLiteral("b05_pediatric_history"),
                  QStringLiteral("patient_id"), patientId);
}

bool Database::hasAdultHistory(qlonglong patientId) const {
    return exists(QStringLiteral("b03_adult_history"),
                  QStringLiteral("patient_id"), patientId);
}

bool Database::hasPregnancyHistory(qlonglong patientId) const {
    return exists(QStringLiteral("b04_pregnancy_history"),
                  QStringLiteral("patient_id"), patientId);
}

// --- Write operations ---

qlonglong Database::nextId() const {
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COALESCE(MAX(id),0)+1 FROM b01_patient")) && q.next())
        return q.value(0).toLongLong();
    return 1;
}

bool Database::hasDuplicate(const Patient& p, qlonglong exceptId) const {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT 1 FROM b01_patient WHERE name=? AND birth_date=? "
        "AND sex=? AND approximate_age=? AND id<>? LIMIT 1"));
    q.addBindValue(p.name);
    q.addBindValue(p.birthDate.toString(QStringLiteral("yyyy-MM-dd")));
    q.addBindValue(static_cast<int>(p.sex));
    q.addBindValue(p.ageRange);
    q.addBindValue(exceptId);
    return q.exec() && q.next();
}

// Prevent binding null QString values as NULL in TEXT NOT NULL columns by
// converting them to empty strings.
static QString nonNull(const QString& s) {
    return s.isNull() ? QString(QLatin1String("")) : s;
}

bool Database::insert(Patient& p) {
    p.id = nextId();
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO b01_patient "
        "(id, name, birth_date, sex, approximate_age, "
        " address, cohabitants, contact_person, sibling_count) "
        "VALUES (?,?,?,?,?,?,?,?,?)"));
    q.addBindValue(p.id);
    q.addBindValue(nonNull(p.name));
    q.addBindValue(p.birthDate.toString(QStringLiteral("yyyy-MM-dd")));
    q.addBindValue(static_cast<int>(p.sex));
    q.addBindValue(p.ageRange);
    q.addBindValue(nonNull(p.address));
    q.addBindValue(nonNull(p.cohabitants));
    q.addBindValue(nonNull(p.contactPerson));
    q.addBindValue(p.siblingCount);
    return q.exec();
}

bool Database::update(const Patient& p) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE b01_patient SET name=?, birth_date=?, sex=?, "
        "approximate_age=?, address=?, cohabitants=?, "
        "contact_person=?, sibling_count=? WHERE id=?"));
    q.addBindValue(nonNull(p.name));
    q.addBindValue(p.birthDate.toString(QStringLiteral("yyyy-MM-dd")));
    q.addBindValue(static_cast<int>(p.sex));
    q.addBindValue(p.ageRange);
    q.addBindValue(nonNull(p.address));
    q.addBindValue(nonNull(p.cohabitants));
    q.addBindValue(nonNull(p.contactPerson));
    q.addBindValue(p.siblingCount);
    q.addBindValue(p.id);
    return q.exec();
}

bool Database::remove(qlonglong patientId) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM b01_patient WHERE id=?"));
    q.addBindValue(patientId);
    return q.exec();
}

// --- Pediatric history (columns managed by the history window) ---

namespace {

// Form-managed columns of b05_pediatric_history, in binding order.
QStringList pediatricColumns() {
    return {
        QStringLiteral("allergies"),
        QStringLiteral("opening_date"),
        QStringLiteral("neonatal_weight"),
        QStringLiteral("neonatal_height"),
        QStringLiteral("neonatal_delivery_incidents"),
        QStringLiteral("neonatal_malformations"),
        QStringLiteral("clinical_disease_type"),
        QStringLiteral("clinical_disease_treatment"),
        QStringLiteral("other_diseases"),
        QStringLiteral("conflicts"),
        QStringLiteral("feeding_breastfeeding"),
        QStringLiteral("feeding_additional_feeding"),
        QStringLiteral("feeding_feeding_problems"),
        QStringLiteral("feeding_treated_water"),
        QStringLiteral("feeding_treated_water_details"),
        QStringLiteral("vaccine_bcg"),
        QStringLiteral("vaccine_opv0"),
        QStringLiteral("vaccine_opv1"),
        QStringLiteral("vaccine_opv2"),
        QStringLiteral("vaccine_opv3"),
        QStringLiteral("vaccine_dpt_hib1"),
        QStringLiteral("vaccine_dpt_hib2"),
        QStringLiteral("vaccine_dpt_hib3"),
        QStringLiteral("vaccine_hep_b1"),
        QStringLiteral("vaccine_hep_b2"),
        QStringLiteral("vaccine_hep_b3"),
        QStringLiteral("vaccine_measles9"),
        QStringLiteral("vaccine_measles10"),
        QStringLiteral("latrine"),
        QStringLiteral("domestic_hygiene"),
        QStringLiteral("mosquito_net"),
        QStringLiteral("net_use"),
        QStringLiteral("domestic_animals"),
        QStringLiteral("economic_situation"),
        QStringLiteral("supplement_date1"),
        QStringLiteral("supplement_date2"),
        QStringLiteral("supplement_date3"),
        QStringLiteral("supplement_date4"),
        QStringLiteral("supplement_treatment1"),
        QStringLiteral("supplement_treatment2"),
        QStringLiteral("supplement_treatment3"),
        QStringLiteral("supplement_treatment4"),
    };
}

QVariant dateOrNull(const QDate& date) {
    if (!date.isValid())
        return QVariant(QMetaType(QMetaType::QString));
    return date.toString(QStringLiteral("yyyy-MM-dd"));
}

QDate dateFromValue(const QVariant& value) {
    if (value.isNull())
        return QDate();
    return QDate::fromString(value.toString(), QStringLiteral("yyyy-MM-dd"));
}

void bindPediatricHistory(QSqlQuery& q, const PediatricHistory& h) {
    q.addBindValue(nonNull(h.allergies));
    q.addBindValue(h.openingDate.isValid()
                       ? h.openingDate.toString(QStringLiteral("yyyy-MM-dd"))
                       : QStringLiteral("1900-01-01"));
    q.addBindValue(h.neonatalWeight);
    q.addBindValue(h.neonatalHeight);
    q.addBindValue(nonNull(h.neonatalDeliveryIncidents));
    q.addBindValue(nonNull(h.neonatalMalformations));
    q.addBindValue(nonNull(h.clinicalDiseaseType));
    q.addBindValue(nonNull(h.clinicalDiseaseTreatment));
    q.addBindValue(nonNull(h.otherDiseases));
    q.addBindValue(nonNull(h.conflicts));
    q.addBindValue(h.feedingBreastfeeding ? 1 : 0);
    q.addBindValue(nonNull(h.feedingAdditionalFeeding));
    q.addBindValue(nonNull(h.feedingFeedingProblems));
    q.addBindValue(h.feedingTreatedWater ? 1 : 0);
    q.addBindValue(nonNull(h.feedingTreatedWaterDetails));
    q.addBindValue(h.vaccineBcg ? 1 : 0);
    q.addBindValue(h.vaccineOpv0 ? 1 : 0);
    q.addBindValue(h.vaccineOpv1 ? 1 : 0);
    q.addBindValue(h.vaccineOpv2 ? 1 : 0);
    q.addBindValue(h.vaccineOpv3 ? 1 : 0);
    q.addBindValue(h.vaccineDptHib1 ? 1 : 0);
    q.addBindValue(h.vaccineDptHib2 ? 1 : 0);
    q.addBindValue(h.vaccineDptHib3 ? 1 : 0);
    q.addBindValue(h.vaccineHepB1 ? 1 : 0);
    q.addBindValue(h.vaccineHepB2 ? 1 : 0);
    q.addBindValue(h.vaccineHepB3 ? 1 : 0);
    q.addBindValue(h.vaccineMeasles9 ? 1 : 0);
    q.addBindValue(h.vaccineMeasles10 ? 1 : 0);
    q.addBindValue(h.latrine ? 1 : 0);
    q.addBindValue(h.domesticHygiene ? 1 : 0);
    q.addBindValue(h.mosquitoNet ? 1 : 0);
    q.addBindValue(h.netUse ? 1 : 0);
    q.addBindValue(static_cast<int>(h.domesticAnimals));
    q.addBindValue(nonNull(h.economicSituation));
    q.addBindValue(dateOrNull(h.supplementDate1));
    q.addBindValue(dateOrNull(h.supplementDate2));
    q.addBindValue(dateOrNull(h.supplementDate3));
    q.addBindValue(dateOrNull(h.supplementDate4));
    q.addBindValue(static_cast<int>(h.supplementTreatment1));
    q.addBindValue(static_cast<int>(h.supplementTreatment2));
    q.addBindValue(static_cast<int>(h.supplementTreatment3));
    q.addBindValue(static_cast<int>(h.supplementTreatment4));
}

void readPediatricHistory(QSqlQuery& q, PediatricHistory& h) {
    int i = 0;
    h.allergies = q.value(i++).toString();
    h.openingDate = dateFromValue(q.value(i++));
    if (!h.openingDate.isValid())
        h.openingDate = QDate(1900, 1, 1);
    h.neonatalWeight = q.value(i++).toInt();
    h.neonatalHeight = q.value(i++).toInt();
    h.neonatalDeliveryIncidents = q.value(i++).toString();
    h.neonatalMalformations = q.value(i++).toString();
    h.clinicalDiseaseType = q.value(i++).toString();
    h.clinicalDiseaseTreatment = q.value(i++).toString();
    h.otherDiseases = q.value(i++).toString();
    h.conflicts = q.value(i++).toString();
    h.feedingBreastfeeding = q.value(i++).toInt() != 0;
    h.feedingAdditionalFeeding = q.value(i++).toString();
    h.feedingFeedingProblems = q.value(i++).toString();
    h.feedingTreatedWater = q.value(i++).toInt() != 0;
    h.feedingTreatedWaterDetails = q.value(i++).toString();
    h.vaccineBcg = q.value(i++).toInt() != 0;
    h.vaccineOpv0 = q.value(i++).toInt() != 0;
    h.vaccineOpv1 = q.value(i++).toInt() != 0;
    h.vaccineOpv2 = q.value(i++).toInt() != 0;
    h.vaccineOpv3 = q.value(i++).toInt() != 0;
    h.vaccineDptHib1 = q.value(i++).toInt() != 0;
    h.vaccineDptHib2 = q.value(i++).toInt() != 0;
    h.vaccineDptHib3 = q.value(i++).toInt() != 0;
    h.vaccineHepB1 = q.value(i++).toInt() != 0;
    h.vaccineHepB2 = q.value(i++).toInt() != 0;
    h.vaccineHepB3 = q.value(i++).toInt() != 0;
    h.vaccineMeasles9 = q.value(i++).toInt() != 0;
    h.vaccineMeasles10 = q.value(i++).toInt() != 0;
    h.latrine = q.value(i++).toInt() != 0;
    h.domesticHygiene = q.value(i++).toInt() != 0;
    h.mosquitoNet = q.value(i++).toInt() != 0;
    h.netUse = q.value(i++).toInt() != 0;
    h.domesticAnimals =
        static_cast<PediatricHistory::DomesticAnimals>(q.value(i++).toInt());
    h.economicSituation = q.value(i++).toString();
    h.supplementDate1 = dateFromValue(q.value(i++));
    h.supplementDate2 = dateFromValue(q.value(i++));
    h.supplementDate3 = dateFromValue(q.value(i++));
    h.supplementDate4 = dateFromValue(q.value(i++));
    h.supplementTreatment1 =
        static_cast<PediatricHistory::TreatmentType>(q.value(i++).toInt());
    h.supplementTreatment2 =
        static_cast<PediatricHistory::TreatmentType>(q.value(i++).toInt());
    h.supplementTreatment3 =
        static_cast<PediatricHistory::TreatmentType>(q.value(i++).toInt());
    h.supplementTreatment4 =
        static_cast<PediatricHistory::TreatmentType>(q.value(i++).toInt());
}

} // namespace

bool Database::loadPediatricHistory(qlonglong patientId, PediatricHistory& history) const {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT %1 FROM b05_pediatric_history WHERE patient_id=?")
                  .arg(pediatricColumns().join(QChar(','))));
    q.addBindValue(patientId);
    if (!q.exec() || !q.next())
        return false;
    history = PediatricHistory();
    history.patientId = patientId;
    readPediatricHistory(q, history);
    return true;
}

bool Database::insertPediatricHistory(const PediatricHistory& history) {
    const QStringList columns = pediatricColumns();
    QStringList placeholders;
    for (int i = 0; i < columns.size(); ++i)
        placeholders.append(QStringLiteral("?"));
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO b05_pediatric_history (patient_id,%1) VALUES (?,%2)")
                  .arg(columns.join(QChar(',')), placeholders.join(QChar(','))));
    q.addBindValue(history.patientId);
    bindPediatricHistory(q, history);
    return q.exec();
}

bool Database::updatePediatricHistory(const PediatricHistory& history) {
    QStringList assignments;
    for (const QString& column : pediatricColumns())
        assignments.append(column + QStringLiteral("=?"));
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE b05_pediatric_history SET %1 WHERE patient_id=?")
                  .arg(assignments.join(QChar(','))));
    bindPediatricHistory(q, history);
    q.addBindValue(history.patientId);
    return q.exec();
}

bool Database::removePediatricHistory(qlonglong patientId) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM b05_pediatric_history WHERE patient_id=?"));
    q.addBindValue(patientId);
    return q.exec();
}

// --- Adult history (columns managed by the history window) ---

namespace {

// Form-managed columns of b03_adult_history, in binding order.
QStringList adultColumns() {
    return {
        QStringLiteral("allergies"),
        QStringLiteral("opening_date"),
        QStringLiteral("history_diabetes"),
        QStringLiteral("history_diabetes_treatment"),
        QStringLiteral("history_hepatitis"),
        QStringLiteral("history_hepatitis_type"),
        QStringLiteral("history_hepatitis_treatment"),
        QStringLiteral("history_hiv"),
        QStringLiteral("history_hiv_treatment"),
        QStringLiteral("history_other_diseases"),
        QStringLiteral("history_disabilities"),
        QStringLiteral("history_conflicts"),
        QStringLiteral("history_occupation"),
        QStringLiteral("history_child_count"),
        QStringLiteral("housing_potable_water"),
        QStringLiteral("housing_latrine"),
        QStringLiteral("housing_domestic_hygiene"),
        QStringLiteral("housing_domestic_animals"),
        QStringLiteral("housing_mosquito_net"),
        QStringLiteral("housing_mosquito_net_use"),
        QStringLiteral("housing_sanitary_control"),
        QStringLiteral("physical_exam_weight"),
        QStringLiteral("physical_exam_height"),
        QStringLiteral("physical_exam_temperature"),
        QStringLiteral("physical_exam_abdominal_perimeter"),
        QStringLiteral("physical_exam_bmi"),
        QStringLiteral("physical_exam_consciousness"),
        QStringLiteral("physical_exam_capillary_glucose"),
        QStringLiteral("physical_exam_heart_rate"),
        QStringLiteral("physical_exam_respiratory_rate"),
    };
}

void bindAdultHistory(QSqlQuery& q, const AdultHistory& h) {
    q.addBindValue(nonNull(h.allergies));
    q.addBindValue(h.openingDate.isValid()
                       ? h.openingDate.toString(QStringLiteral("yyyy-MM-dd"))
                       : QStringLiteral("1900-01-01"));
    q.addBindValue(h.historyDiabetes ? 1 : 0);
    q.addBindValue(nonNull(h.historyDiabetesTreatment));
    q.addBindValue(h.historyHepatitis ? 1 : 0);
    q.addBindValue(nonNull(h.historyHepatitisType));
    q.addBindValue(nonNull(h.historyHepatitisTreatment));
    q.addBindValue(h.historyHiv ? 1 : 0);
    q.addBindValue(nonNull(h.historyHivTreatment));
    q.addBindValue(nonNull(h.historyOtherDiseases));
    q.addBindValue(nonNull(h.historyDisabilities));
    q.addBindValue(nonNull(h.historyConflicts));
    q.addBindValue(nonNull(h.historyOccupation));
    q.addBindValue(h.historyChildCount);
    q.addBindValue(h.housingPotableWater ? 1 : 0);
    q.addBindValue(h.housingLatrine ? 1 : 0);
    q.addBindValue(h.housingDomesticHygiene ? 1 : 0);
    q.addBindValue(static_cast<int>(h.housingDomesticAnimals));
    q.addBindValue(h.housingMosquitoNet ? 1 : 0);
    q.addBindValue(h.housingMosquitoNetUse ? 1 : 0);
    q.addBindValue(h.housingSanitaryControl ? 1 : 0);
    q.addBindValue(qRound(h.physicalExamWeight * 10.0));
    q.addBindValue(h.physicalExamHeight);
    q.addBindValue(qRound(h.physicalExamTemperature * 10.0));
    q.addBindValue(h.physicalExamAbdominalPerimeter);
    q.addBindValue(qRound(h.physicalExamBmi * 100.0));
    q.addBindValue(nonNull(h.physicalExamConsciousness));
    q.addBindValue(nonNull(h.physicalExamCapillaryGlucose));
    q.addBindValue(nonNull(h.physicalExamHeartRate));
    q.addBindValue(nonNull(h.physicalExamRespiratoryRate));
}

void readAdultHistory(QSqlQuery& q, AdultHistory& h) {
    int i = 0;
    h.allergies = q.value(i++).toString();
    h.openingDate = dateFromValue(q.value(i++));
    if (!h.openingDate.isValid())
        h.openingDate = QDate(1900, 1, 1);
    h.historyDiabetes = q.value(i++).toInt() != 0;
    h.historyDiabetesTreatment = q.value(i++).toString();
    h.historyHepatitis = q.value(i++).toInt() != 0;
    h.historyHepatitisType = q.value(i++).toString();
    h.historyHepatitisTreatment = q.value(i++).toString();
    h.historyHiv = q.value(i++).toInt() != 0;
    h.historyHivTreatment = q.value(i++).toString();
    h.historyOtherDiseases = q.value(i++).toString();
    h.historyDisabilities = q.value(i++).toString();
    h.historyConflicts = q.value(i++).toString();
    h.historyOccupation = q.value(i++).toString();
    h.historyChildCount = q.value(i++).toInt();
    h.housingPotableWater = q.value(i++).toInt() != 0;
    h.housingLatrine = q.value(i++).toInt() != 0;
    h.housingDomesticHygiene = q.value(i++).toInt() != 0;
    h.housingDomesticAnimals =
        static_cast<PediatricHistory::DomesticAnimals>(q.value(i++).toInt());
    h.housingMosquitoNet = q.value(i++).toInt() != 0;
    h.housingMosquitoNetUse = q.value(i++).toInt() != 0;
    h.housingSanitaryControl = q.value(i++).toInt() != 0;
    h.physicalExamWeight = q.value(i++).toInt() / 10.0;
    h.physicalExamHeight = q.value(i++).toInt();
    h.physicalExamTemperature = q.value(i++).toInt() / 10.0;
    h.physicalExamAbdominalPerimeter = q.value(i++).toInt();
    h.physicalExamBmi = q.value(i++).toInt() / 100.0;
    h.physicalExamConsciousness = q.value(i++).toString();
    h.physicalExamCapillaryGlucose = q.value(i++).toString();
    h.physicalExamHeartRate = q.value(i++).toString();
    h.physicalExamRespiratoryRate = q.value(i++).toString();
}

} // namespace

bool Database::loadAdultHistory(qlonglong patientId, AdultHistory& history) const {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT %1 FROM b03_adult_history WHERE patient_id=?")
                  .arg(adultColumns().join(QChar(','))));
    q.addBindValue(patientId);
    if (!q.exec() || !q.next())
        return false;
    history = AdultHistory();
    history.patientId = patientId;
    readAdultHistory(q, history);
    return true;
}

bool Database::insertAdultHistory(const AdultHistory& history) {
    const QStringList columns = adultColumns();
    QStringList placeholders;
    for (int i = 0; i < columns.size(); ++i)
        placeholders.append(QStringLiteral("?"));
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO b03_adult_history (patient_id,%1) VALUES (?,%2)")
                  .arg(columns.join(QChar(',')), placeholders.join(QChar(','))));
    q.addBindValue(history.patientId);
    bindAdultHistory(q, history);
    return q.exec();
}

bool Database::updateAdultHistory(const AdultHistory& history) {
    QStringList assignments;
    for (const QString& column : adultColumns())
        assignments.append(column + QStringLiteral("=?"));
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE b03_adult_history SET %1 WHERE patient_id=?")
                  .arg(assignments.join(QChar(','))));
    bindAdultHistory(q, history);
    q.addBindValue(history.patientId);
    return q.exec();
}

bool Database::removeAdultHistory(qlonglong patientId) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM b03_adult_history WHERE patient_id=?"));
    q.addBindValue(patientId);
    return q.exec();
}

// --- Pregnancy history (columns managed by the history window) ---

namespace {

// Form-managed columns of b04_pregnancy_history, in binding order.
QStringList pregnancyColumns() {
    return {
        QStringLiteral("allergies"),
        QStringLiteral("opening_date"),
        QStringLiteral("obstetric_last_menstruation_date"),
        QStringLiteral("obstetric_expected_delivery_date"),
        QStringLiteral("obstetric_deceased_sibling_count"),
        QStringLiteral("obstetric_delivery_count"),
        QStringLiteral("obstetric_abortion_count"),
        QStringLiteral("obstetric_delivery_problems"),
        QStringLiteral("obstetric_gynecological_diseases"),
        QStringLiteral("obstetric_barrier_methods"),
        QStringLiteral("obstetric_hiv_woman"),
        QStringLiteral("obstetric_hiv_man"),
        QStringLiteral("obstetric_serologies"),
        QStringLiteral("medical_diabetes"),
        QStringLiteral("medical_diabetes_treatment"),
        QStringLiteral("medical_hepatitis"),
        QStringLiteral("medical_hepatitis_type"),
        QStringLiteral("medical_hepatitis_treatment"),
        QStringLiteral("medical_hypertension"),
        QStringLiteral("medical_hypertension_treatment"),
        QStringLiteral("medical_other_diseases"),
        QStringLiteral("medical_disabilities"),
        QStringLiteral("medical_conflicts"),
        QStringLiteral("housing_potable_water"),
        QStringLiteral("housing_treated_water"),
        QStringLiteral("housing_latrine"),
        QStringLiteral("housing_domestic_hygiene"),
        QStringLiteral("housing_domestic_animals"),
        QStringLiteral("housing_mosquito_net_parents"),
        QStringLiteral("housing_mosquito_net_use_parents"),
        QStringLiteral("housing_mosquito_net_children"),
        QStringLiteral("housing_mosquito_net_use_children"),
        QStringLiteral("physical_exam_weight"),
        QStringLiteral("physical_exam_height"),
        QStringLiteral("physical_exam_temperature"),
        QStringLiteral("physical_exam_abdominal_perimeter"),
        QStringLiteral("physical_exam_vomiting"),
        QStringLiteral("physical_exam_fetal_position"),
        QStringLiteral("physical_exam_fetal_auscultation"),
        QStringLiteral("physical_exam_uterine_height_weeks"),
        QStringLiteral("physical_exam_uterine_height_cm"),
        QStringLiteral("physical_exam_leukocytosis"),
        QStringLiteral("physical_exam_proteinuria"),
        QStringLiteral("physical_exam_mmi"),
        QStringLiteral("physical_exam_face"),
        QStringLiteral("physical_exam_general"),
        QStringLiteral("supplement_date1"),
        QStringLiteral("supplement_date2"),
        QStringLiteral("supplement_date3"),
        QStringLiteral("supplement_date4"),
        QStringLiteral("supplement_date5"),
        QStringLiteral("supplement_medication1"),
        QStringLiteral("supplement_medication2"),
        QStringLiteral("supplement_medication3"),
        QStringLiteral("supplement_medication4"),
        QStringLiteral("supplement_medication5"),
        QStringLiteral("supplement_dose1"),
        QStringLiteral("supplement_dose2"),
        QStringLiteral("supplement_dose3"),
        QStringLiteral("supplement_dose4"),
        QStringLiteral("supplement_dose5"),
        QStringLiteral("supplement_calcium_d"),
        QStringLiteral("supplement_calcium_m"),
        QStringLiteral("supplement_calcium_pd"),
        QStringLiteral("supplement_vitamin_c_d"),
        QStringLiteral("supplement_vitamin_c_m"),
        QStringLiteral("supplement_vitamin_c_pd"),
        QStringLiteral("supplement_other_vitamins_d"),
        QStringLiteral("supplement_other_vitamins_m"),
        QStringLiteral("supplement_other_vitamins_pd"),
        QStringLiteral("supplement_malaria_prophylaxis_d"),
        QStringLiteral("supplement_malaria_prophylaxis_m"),
        QStringLiteral("supplement_malaria_prophylaxis_pd"),
    };
}

void bindPregnancyHistory(QSqlQuery& q, const PregnancyHistory& h) {
    q.addBindValue(nonNull(h.allergies));
    q.addBindValue(h.openingDate.isValid()
                       ? h.openingDate.toString(QStringLiteral("yyyy-MM-dd"))
                       : QStringLiteral("1900-01-01"));
    q.addBindValue(dateOrNull(h.obstetricLastMenstruation));
    q.addBindValue(dateOrNull(h.obstetricExpectedDelivery));
    q.addBindValue(h.obstetricDeceasedSiblingCount);
    q.addBindValue(h.obstetricDeliveryCount);
    q.addBindValue(h.obstetricAbortionCount);
    q.addBindValue(nonNull(h.obstetricDeliveryProblems));
    q.addBindValue(nonNull(h.obstetricGynecologicalDiseases));
    q.addBindValue(nonNull(h.obstetricBarrierMethods));
    q.addBindValue(h.obstetricHivWoman ? 1 : 0);
    q.addBindValue(h.obstetricHivMan ? 1 : 0);
    q.addBindValue(nonNull(h.obstetricSerologies));
    q.addBindValue(h.medicalDiabetes ? 1 : 0);
    q.addBindValue(nonNull(h.medicalDiabetesTreatment));
    q.addBindValue(h.medicalHepatitis ? 1 : 0);
    q.addBindValue(nonNull(h.medicalHepatitisType));
    q.addBindValue(nonNull(h.medicalHepatitisTreatment));
    q.addBindValue(h.medicalHypertension ? 1 : 0);
    q.addBindValue(nonNull(h.medicalHypertensionTreatment));
    q.addBindValue(nonNull(h.medicalOtherDiseases));
    q.addBindValue(nonNull(h.medicalDisabilities));
    q.addBindValue(nonNull(h.medicalConflicts));
    q.addBindValue(h.housingPotableWater ? 1 : 0);
    q.addBindValue(h.housingTreatedWater ? 1 : 0);
    q.addBindValue(h.housingLatrine ? 1 : 0);
    q.addBindValue(h.housingDomesticHygiene ? 1 : 0);
    q.addBindValue(static_cast<int>(h.housingDomesticAnimals));
    q.addBindValue(h.housingMosquitoNetParents ? 1 : 0);
    q.addBindValue(h.housingMosquitoNetUseParents ? 1 : 0);
    q.addBindValue(h.housingMosquitoNetChildren ? 1 : 0);
    q.addBindValue(h.housingMosquitoNetUseChildren ? 1 : 0);
    q.addBindValue(qRound(h.physicalExamWeight * 10.0));
    q.addBindValue(h.physicalExamHeight);
    q.addBindValue(qRound(h.physicalExamTemperature * 10.0));
    q.addBindValue(h.physicalExamAbdominalPerimeter);
    q.addBindValue(nonNull(h.physicalExamVomiting));
    q.addBindValue(nonNull(h.physicalExamFetalPosition));
    q.addBindValue(nonNull(h.physicalExamFetalAuscultation));
    q.addBindValue(h.physicalExamUterineHeightWeeks);
    q.addBindValue(h.physicalExamUterineHeightCm);
    q.addBindValue(h.physicalExamLeukocytosis ? 1 : 0);
    q.addBindValue(h.physicalExamProteinuria ? 1 : 0);
    q.addBindValue(h.physicalExamMmi ? 1 : 0);
    q.addBindValue(h.physicalExamFace ? 1 : 0);
    q.addBindValue(h.physicalExamGeneral ? 1 : 0);
    q.addBindValue(dateOrNull(h.supplementDate1));
    q.addBindValue(dateOrNull(h.supplementDate2));
    q.addBindValue(dateOrNull(h.supplementDate3));
    q.addBindValue(dateOrNull(h.supplementDate4));
    q.addBindValue(dateOrNull(h.supplementDate5));
    q.addBindValue(nonNull(h.supplementMedication1));
    q.addBindValue(nonNull(h.supplementMedication2));
    q.addBindValue(nonNull(h.supplementMedication3));
    q.addBindValue(nonNull(h.supplementMedication4));
    q.addBindValue(nonNull(h.supplementMedication5));
    q.addBindValue(nonNull(h.supplementDose1));
    q.addBindValue(nonNull(h.supplementDose2));
    q.addBindValue(nonNull(h.supplementDose3));
    q.addBindValue(nonNull(h.supplementDose4));
    q.addBindValue(nonNull(h.supplementDose5));
    q.addBindValue(nonNull(h.supplementCalciumD));
    q.addBindValue(nonNull(h.supplementCalciumM));
    q.addBindValue(nonNull(h.supplementCalciumPd));
    q.addBindValue(nonNull(h.supplementVitaminCD));
    q.addBindValue(nonNull(h.supplementVitaminCM));
    q.addBindValue(nonNull(h.supplementVitaminCPd));
    q.addBindValue(nonNull(h.supplementOtherVitaminsD));
    q.addBindValue(nonNull(h.supplementOtherVitaminsM));
    q.addBindValue(nonNull(h.supplementOtherVitaminsPd));
    q.addBindValue(nonNull(h.supplementMalariaProphylaxisD));
    q.addBindValue(nonNull(h.supplementMalariaProphylaxisM));
    q.addBindValue(nonNull(h.supplementMalariaProphylaxisPd));
}

void readPregnancyHistory(QSqlQuery& q, PregnancyHistory& h) {
    int i = 0;
    h.allergies = q.value(i++).toString();
    h.openingDate = dateFromValue(q.value(i++));
    if (!h.openingDate.isValid())
        h.openingDate = QDate(1900, 1, 1);
    h.obstetricLastMenstruation = dateFromValue(q.value(i++));
    h.obstetricExpectedDelivery = dateFromValue(q.value(i++));
    h.obstetricDeceasedSiblingCount = q.value(i++).toInt();
    h.obstetricDeliveryCount = q.value(i++).toInt();
    h.obstetricAbortionCount = q.value(i++).toInt();
    h.obstetricDeliveryProblems = q.value(i++).toString();
    h.obstetricGynecologicalDiseases = q.value(i++).toString();
    h.obstetricBarrierMethods = q.value(i++).toString();
    h.obstetricHivWoman = q.value(i++).toInt() != 0;
    h.obstetricHivMan = q.value(i++).toInt() != 0;
    h.obstetricSerologies = q.value(i++).toString();
    h.medicalDiabetes = q.value(i++).toInt() != 0;
    h.medicalDiabetesTreatment = q.value(i++).toString();
    h.medicalHepatitis = q.value(i++).toInt() != 0;
    h.medicalHepatitisType = q.value(i++).toString();
    h.medicalHepatitisTreatment = q.value(i++).toString();
    h.medicalHypertension = q.value(i++).toInt() != 0;
    h.medicalHypertensionTreatment = q.value(i++).toString();
    h.medicalOtherDiseases = q.value(i++).toString();
    h.medicalDisabilities = q.value(i++).toString();
    h.medicalConflicts = q.value(i++).toString();
    h.housingPotableWater = q.value(i++).toInt() != 0;
    h.housingTreatedWater = q.value(i++).toInt() != 0;
    h.housingLatrine = q.value(i++).toInt() != 0;
    h.housingDomesticHygiene = q.value(i++).toInt() != 0;
    h.housingDomesticAnimals =
        static_cast<PediatricHistory::DomesticAnimals>(q.value(i++).toInt());
    h.housingMosquitoNetParents = q.value(i++).toInt() != 0;
    h.housingMosquitoNetUseParents = q.value(i++).toInt() != 0;
    h.housingMosquitoNetChildren = q.value(i++).toInt() != 0;
    h.housingMosquitoNetUseChildren = q.value(i++).toInt() != 0;
    h.physicalExamWeight = q.value(i++).toInt() / 10.0;
    h.physicalExamHeight = q.value(i++).toInt();
    h.physicalExamTemperature = q.value(i++).toInt() / 10.0;
    h.physicalExamAbdominalPerimeter = q.value(i++).toInt();
    h.physicalExamVomiting = q.value(i++).toString();
    h.physicalExamFetalPosition = q.value(i++).toString();
    h.physicalExamFetalAuscultation = q.value(i++).toString();
    h.physicalExamUterineHeightWeeks = q.value(i++).toInt();
    h.physicalExamUterineHeightCm = q.value(i++).toInt();
    h.physicalExamLeukocytosis = q.value(i++).toInt() != 0;
    h.physicalExamProteinuria = q.value(i++).toInt() != 0;
    h.physicalExamMmi = q.value(i++).toInt() != 0;
    h.physicalExamFace = q.value(i++).toInt() != 0;
    h.physicalExamGeneral = q.value(i++).toInt() != 0;
    h.supplementDate1 = dateFromValue(q.value(i++));
    h.supplementDate2 = dateFromValue(q.value(i++));
    h.supplementDate3 = dateFromValue(q.value(i++));
    h.supplementDate4 = dateFromValue(q.value(i++));
    h.supplementDate5 = dateFromValue(q.value(i++));
    h.supplementMedication1 = q.value(i++).toString();
    h.supplementMedication2 = q.value(i++).toString();
    h.supplementMedication3 = q.value(i++).toString();
    h.supplementMedication4 = q.value(i++).toString();
    h.supplementMedication5 = q.value(i++).toString();
    h.supplementDose1 = q.value(i++).toString();
    h.supplementDose2 = q.value(i++).toString();
    h.supplementDose3 = q.value(i++).toString();
    h.supplementDose4 = q.value(i++).toString();
    h.supplementDose5 = q.value(i++).toString();
    h.supplementCalciumD = q.value(i++).toString();
    h.supplementCalciumM = q.value(i++).toString();
    h.supplementCalciumPd = q.value(i++).toString();
    h.supplementVitaminCD = q.value(i++).toString();
    h.supplementVitaminCM = q.value(i++).toString();
    h.supplementVitaminCPd = q.value(i++).toString();
    h.supplementOtherVitaminsD = q.value(i++).toString();
    h.supplementOtherVitaminsM = q.value(i++).toString();
    h.supplementOtherVitaminsPd = q.value(i++).toString();
    h.supplementMalariaProphylaxisD = q.value(i++).toString();
    h.supplementMalariaProphylaxisM = q.value(i++).toString();
    h.supplementMalariaProphylaxisPd = q.value(i++).toString();
}

} // namespace

bool Database::loadPregnancyHistory(qlonglong patientId, PregnancyHistory& history) const {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT %1 FROM b04_pregnancy_history WHERE patient_id=?")
                  .arg(pregnancyColumns().join(QChar(','))));
    q.addBindValue(patientId);
    if (!q.exec() || !q.next())
        return false;
    history = PregnancyHistory();
    history.patientId = patientId;
    readPregnancyHistory(q, history);
    return true;
}

bool Database::insertPregnancyHistory(const PregnancyHistory& history) {
    const QStringList columns = pregnancyColumns();
    QStringList placeholders;
    for (int i = 0; i < columns.size(); ++i)
        placeholders.append(QStringLiteral("?"));
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO b04_pregnancy_history (patient_id,%1) VALUES (?,%2)")
                  .arg(columns.join(QChar(',')), placeholders.join(QChar(','))));
    q.addBindValue(history.patientId);
    bindPregnancyHistory(q, history);
    return q.exec();
}

bool Database::updatePregnancyHistory(const PregnancyHistory& history) {
    QStringList assignments;
    for (const QString& column : pregnancyColumns())
        assignments.append(column + QStringLiteral("=?"));
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE b04_pregnancy_history SET %1 WHERE patient_id=?")
                  .arg(assignments.join(QChar(','))));
    bindPregnancyHistory(q, history);
    q.addBindValue(history.patientId);
    return q.exec();
}

bool Database::removePregnancyHistory(qlonglong patientId) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM b04_pregnancy_history WHERE patient_id=?"));
    q.addBindValue(patientId);
    return q.exec();
}

} // namespace gambasse
