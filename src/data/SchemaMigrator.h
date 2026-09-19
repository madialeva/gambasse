#pragma once

#include <QString>
#include <QSqlDatabase>

namespace gambasse {

// Applies the SQL schema patches embedded in the executable resources.
//
// A patch file is named "<x.y.z.n>_<slug>.sql", where x.y.z is the program
// version and n is a sequence that restarts for every program version. Applied
// patches are recorded in b00_bd_migrations, which is the source of truth for
// what is still pending. A legacy database (clinical tables present but no
// history table) is baselined by recording the initial patch without running
// it, so existing data is preserved.
class SchemaMigrator {
public:
    // Applies the pending patches found under patchesDir (for example ":/bd").
    // Returns false and sets errorMessage on failure; the database is left in
    // its previous state when a patch fails.
    static bool migrate(QSqlDatabase& database, const QString& patchesDir,
                        QString* errorMessage);
    static bool migrate(QSqlDatabase& database, QString* errorMessage);
};

} // namespace gambasse
