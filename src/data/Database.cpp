#include <data/Database.h>

#include <Paths.h>
#include <data/SchemaMigrator.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
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
    return exists(QStringLiteral("b05_historias_pediatrica"),
                  QStringLiteral("b05_b01_id"), patientId);
}

bool Database::hasAdultHistory(qlonglong patientId) const {
    return exists(QStringLiteral("b03_historias_adulto"),
                  QStringLiteral("b03_b01_id"), patientId);
}

bool Database::hasPregnancyHistory(qlonglong patientId) const {
    return exists(QStringLiteral("b04_historias_embarazada"),
                  QStringLiteral("b04_b01_id"), patientId);
}

// --- Write operations ---

qlonglong Database::nextId() const {
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COALESCE(MAX(b01_id),0)+1 FROM b01_paciente")) && q.next())
        return q.value(0).toLongLong();
    return 1;
}

bool Database::hasDuplicate(const Patient& p, qlonglong exceptId) const {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT 1 FROM b01_paciente WHERE b01_nome=? AND b01_datanascimento=? "
        "AND b01_sexo=? AND b01_anosaproximados=? AND b01_id<>? LIMIT 1"));
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
        "INSERT INTO b01_paciente "
        "(b01_id, b01_nome, b01_datanascimento, b01_sexo, b01_anosaproximados, "
        " b01_e_enderezo, b01_e_coabitantes, b01_e_pessoacontacto, b01_e_numero_irmaos) "
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
        "UPDATE b01_paciente SET b01_nome=?, b01_datanascimento=?, b01_sexo=?, "
        "b01_anosaproximados=?, b01_e_enderezo=?, b01_e_coabitantes=?, "
        "b01_e_pessoacontacto=?, b01_e_numero_irmaos=? WHERE b01_id=?"));
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
    q.prepare(QStringLiteral("DELETE FROM b01_paciente WHERE b01_id=?"));
    q.addBindValue(patientId);
    return q.exec();
}

} // namespace gambasse
