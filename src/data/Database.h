#pragma once

#include <QString>
#include <QSqlDatabase>

#include <data/Patient.h>

namespace gambasse {

// SQLite database access facade. SQLite only; no multi-engine abstraction.
class Database {
public:
    static Database& instance();

    // Opens the database. Its path comes from config.ini (database/path), with
    // "./database.db" as the default. Returns false and sets errorMessage on failure.
    bool open(QString* errorMessage);

    bool isOpen() const;
    QSqlDatabase& connection() { return m_db; }

    // Read-only history checks used by the main-window button logic.
    bool hasPediatricHistory(qlonglong patientId) const;
    bool hasAdultHistory(qlonglong patientId) const;
    bool hasPregnancyHistory(qlonglong patientId) const;

    // Write operations on b01_paciente.
    qlonglong nextId() const;
    bool hasDuplicate(const Patient& p, qlonglong exceptId) const;
    bool insert(Patient& p);                 // assigns the new identifier to p.id
    bool update(const Patient& p);
    bool remove(qlonglong patientId);

private:
    Database() = default;
    Q_DISABLE_COPY(Database)

    bool exists(const QString& table, const QString& foreignKeyColumn, qlonglong id) const;

    QSqlDatabase m_db;
};

} // namespace gambasse
