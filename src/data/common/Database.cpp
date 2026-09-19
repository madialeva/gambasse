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

} // namespace gambasse
