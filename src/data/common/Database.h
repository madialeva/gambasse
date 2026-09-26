#pragma once

#include <QString>
#include <QSqlDatabase>

#include <data/model/Patient.h>
#include <data/model/AdultHistory.h>
#include <data/model/PediatricHistory.h>
#include <data/model/PregnancyHistory.h>

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

    // Pediatric history (b05_pediatric_history): only the columns managed by
    // the pediatric history window. Unmanaged columns keep their schema
    // defaults on insert and are left untouched on update.
    bool loadPediatricHistory(qlonglong patientId, PediatricHistory& history) const;
    bool insertPediatricHistory(const PediatricHistory& history);
    bool updatePediatricHistory(const PediatricHistory& history);
    bool removePediatricHistory(qlonglong patientId);

    // Adult history (b03_adult_history): only the columns managed by the
    // adult history window. Decimal vital signs use the VB.NET scaling:
    // weight and temperature x10, BMI x100.
    bool loadAdultHistory(qlonglong patientId, AdultHistory& history) const;
    bool insertAdultHistory(const AdultHistory& history);
    bool updateAdultHistory(const AdultHistory& history);
    bool removeAdultHistory(qlonglong patientId);

    // Pregnancy history (b04_pregnancy_history): only the columns managed by
    // the pregnancy history window. Nullable dates use an invalid QDate for
    // NULL. The fixed TabPage2 rows (iron, folic acid, deworming) carry no
    // column and are never persisted.
    bool loadPregnancyHistory(qlonglong patientId, PregnancyHistory& history) const;
    bool insertPregnancyHistory(const PregnancyHistory& history);
    bool updatePregnancyHistory(const PregnancyHistory& history);
    bool removePregnancyHistory(qlonglong patientId);

    // Write operations on b01_patient.
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
