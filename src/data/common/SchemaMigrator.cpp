#include <data/common/SchemaMigrator.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QList>
#include <QObject>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>
#include <algorithm>

namespace gambasse {
namespace {

// Version of a patch: the program version (x.y.z) plus a per-version sequence.
struct PatchVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int seq = 0;

    QString programVersion() const {
        return QStringLiteral("%1.%2.%3").arg(major).arg(minor).arg(patch);
    }
    QString full() const {
        return QStringLiteral("%1.%2.%3.%4")
            .arg(major).arg(minor).arg(patch).arg(seq);
    }
};

bool versionLess(const PatchVersion& a, const PatchVersion& b) {
    if (a.major != b.major)
        return a.major < b.major;
    if (a.minor != b.minor)
        return a.minor < b.minor;
    if (a.patch != b.patch)
        return a.patch < b.patch;
    return a.seq < b.seq;
}

struct Patch {
    PatchVersion version;
    QString name;    // file name
    QString content; // SQL text
};

struct AppliedPatch {
    PatchVersion version;
    QString checksum;
};

QString checksumOf(const QString& content) {
    return QString::fromLatin1(
        QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Sha256).toHex());
}

bool parseProgramVersion(const QString& text, PatchVersion* version) {
    static const QRegularExpression pattern(
        QStringLiteral("^(\\d+)\\.(\\d+)\\.(\\d+)$"));
    const QRegularExpressionMatch match = pattern.match(text);
    if (!match.hasMatch())
        return false;
    version->major = match.captured(1).toInt();
    version->minor = match.captured(2).toInt();
    version->patch = match.captured(3).toInt();
    return true;
}

// Splits a script into individual statements. Semicolons inside single/double
// quoted strings, backtick identifiers and comments are ignored, so a patch may
// contain multiple statements with comments.
QStringList splitStatements(const QString& script) {
    enum State { Normal, SingleQuote, DoubleQuote, Backtick, LineComment, BlockComment };
    QStringList statements;
    QString current;
    State state = Normal;
    const int length = script.size();

    for (int i = 0; i < length; ++i) {
        const QChar c = script.at(i);
        const QChar next = (i + 1 < length) ? script.at(i + 1) : QChar();

        switch (state) {
        case Normal:
            if (c == QLatin1Char('\'')) {
                state = SingleQuote;
                current += c;
            } else if (c == QLatin1Char('"')) {
                state = DoubleQuote;
                current += c;
            } else if (c == QLatin1Char('`')) {
                state = Backtick;
                current += c;
            } else if (c == QLatin1Char('-') && next == QLatin1Char('-')) {
                state = LineComment;
                current += c;
            } else if (c == QLatin1Char('/') && next == QLatin1Char('*')) {
                state = BlockComment;
                current += c;
            } else if (c == QLatin1Char(';')) {
                statements << current;
                current.clear();
            } else {
                current += c;
            }
            break;
        case SingleQuote:
            current += c;
            if (c == QLatin1Char('\'')) {
                if (next == QLatin1Char('\'')) { // escaped quote
                    current += next;
                    ++i;
                } else {
                    state = Normal;
                }
            }
            break;
        case DoubleQuote:
            current += c;
            if (c == QLatin1Char('"')) {
                if (next == QLatin1Char('"')) {
                    current += next;
                    ++i;
                } else {
                    state = Normal;
                }
            }
            break;
        case Backtick:
            current += c;
            if (c == QLatin1Char('`'))
                state = Normal;
            break;
        case LineComment:
            current += c;
            if (c == QLatin1Char('\n'))
                state = Normal;
            break;
        case BlockComment:
            current += c;
            if (c == QLatin1Char('*') && next == QLatin1Char('/')) {
                current += next;
                ++i;
                state = Normal;
            }
            break;
        }
    }

    if (!current.trimmed().isEmpty())
        statements << current;
    return statements;
}

bool discoverPatches(const QString& directory, QList<Patch>* patches,
                     QString* errorMessage) {
    static const QRegularExpression namePattern(
        QStringLiteral("^(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)_(.+)\\.sql$"));
    const QDir dir(directory);
    const QStringList files = dir.entryList(QStringList() << QStringLiteral("*.sql"),
                                            QDir::Files, QDir::Name);
    for (const QString& file : files) {
        const QRegularExpressionMatch match = namePattern.match(file);
        if (!match.hasMatch())
            continue; // ignore files that do not follow the convention

        QFile handle(dir.filePath(file));
        if (!handle.open(QIODevice::ReadOnly)) {
            if (errorMessage)
                *errorMessage = QObject::tr("Could not read schema patch %1").arg(file);
            return false;
        }

        Patch patch;
        patch.version.major = match.captured(1).toInt();
        patch.version.minor = match.captured(2).toInt();
        patch.version.patch = match.captured(3).toInt();
        patch.version.seq = match.captured(4).toInt();
        patch.name = file;
        patch.content = QString::fromUtf8(handle.readAll());
        patches->append(patch);
    }

    std::sort(patches->begin(), patches->end(),
              [](const Patch& a, const Patch& b) {
                  return versionLess(a.version, b.version);
              });
    return true;
}

bool tableExists(QSqlDatabase& db, const QString& table) {
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=? LIMIT 1"));
    query.addBindValue(table);
    return query.exec() && query.next();
}

bool ensureHistoryTable(QSqlDatabase& db, QString* errorMessage) {
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS b00_bd_migrations ("
            " b00m_program_version TEXT NOT NULL,"
            " b00m_seq INTEGER NOT NULL,"
            " b00m_name TEXT NOT NULL,"
            " b00m_applied_at TEXT NOT NULL,"
            " b00m_checksum TEXT NOT NULL,"
            " CONSTRAINT pk_b00 PRIMARY KEY (b00m_program_version, b00m_seq))"))) {
        if (errorMessage)
            *errorMessage = QObject::tr("Could not create the schema history table:\n%1")
                                .arg(query.lastError().text());
        return false;
    }
    return true;
}

bool loadAppliedPatches(QSqlDatabase& db, QList<AppliedPatch>* applied,
                        QString* errorMessage) {
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "SELECT b00m_program_version, b00m_seq, b00m_checksum FROM b00_bd_migrations"))) {
        if (errorMessage)
            *errorMessage = QObject::tr("Could not read the schema history:\n%1")
                                .arg(query.lastError().text());
        return false;
    }
    while (query.next()) {
        AppliedPatch entry;
        if (!parseProgramVersion(query.value(0).toString(), &entry.version)) {
            if (errorMessage)
                *errorMessage = QObject::tr("Invalid program version in the schema history: %1")
                                    .arg(query.value(0).toString());
            return false;
        }
        entry.version.seq = query.value(1).toInt();
        entry.checksum = query.value(2).toString();
        applied->append(entry);
    }
    return true;
}

bool insertHistoryRow(QSqlDatabase& db, const Patch& patch, QString* errorMessage) {
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO b00_bd_migrations "
        "(b00m_program_version, b00m_seq, b00m_name, b00m_applied_at, b00m_checksum) "
        "VALUES (?,?,?,?,?)"));
    query.addBindValue(patch.version.programVersion());
    query.addBindValue(patch.version.seq);
    query.addBindValue(patch.name);
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(checksumOf(patch.content));
    if (!query.exec()) {
        if (errorMessage)
            *errorMessage = QObject::tr("Could not record schema patch %1:\n%2")
                                .arg(patch.name, query.lastError().text());
        return false;
    }
    return true;
}

bool applyPatch(QSqlDatabase& db, const Patch& patch, QString* errorMessage) {
    if (!db.transaction()) {
        if (errorMessage)
            *errorMessage = QObject::tr("Could not start a transaction for %1:\n%2")
                                .arg(patch.name, db.lastError().text());
        return false;
    }

    const QStringList statements = splitStatements(patch.content);
    for (const QString& statement : statements) {
        if (statement.trimmed().isEmpty())
            continue;
        QSqlQuery query(db);
        if (!query.exec(statement)) {
            if (errorMessage)
                *errorMessage = QObject::tr("Failed to apply schema patch %1:\n%2")
                                    .arg(patch.name, query.lastError().text());
            db.rollback();
            return false;
        }
    }

    if (!insertHistoryRow(db, patch, errorMessage)) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        if (errorMessage)
            *errorMessage = QObject::tr("Could not commit schema patch %1:\n%2")
                                .arg(patch.name, db.lastError().text());
        db.rollback();
        return false;
    }
    return true;
}

bool recordBaseline(QSqlDatabase& db, const Patch& patch, QString* errorMessage) {
    if (!db.transaction()) {
        if (errorMessage)
            *errorMessage = QObject::tr("Could not start the baseline transaction:\n%1")
                                .arg(db.lastError().text());
        return false;
    }
    if (!insertHistoryRow(db, patch, errorMessage)) {
        db.rollback();
        return false;
    }
    if (!db.commit()) {
        if (errorMessage)
            *errorMessage = QObject::tr("Could not record the baseline patch:\n%1")
                                .arg(db.lastError().text());
        db.rollback();
        return false;
    }
    return true;
}

bool sameVersion(const PatchVersion& a, const PatchVersion& b) {
    return a.major == b.major && a.minor == b.minor && a.patch == b.patch &&
           a.seq == b.seq;
}

} // namespace

bool SchemaMigrator::migrate(QSqlDatabase& database, const QString& patchesDir,
                             QString* errorMessage) {
    QList<Patch> patches;
    if (!discoverPatches(patchesDir, &patches, errorMessage))
        return false;
    if (patches.isEmpty()) {
        if (errorMessage)
            *errorMessage = QObject::tr("No schema patches found in %1").arg(patchesDir);
        return false;
    }

    if (!ensureHistoryTable(database, errorMessage))
        return false;

    QList<AppliedPatch> applied;
    if (!loadAppliedPatches(database, &applied, errorMessage))
        return false;

    // A patch that was already applied must never change.
    for (const Patch& patch : patches) {
        for (const AppliedPatch& entry : applied) {
            if (sameVersion(entry.version, patch.version) &&
                entry.checksum != checksumOf(patch.content)) {
                if (errorMessage)
                    *errorMessage = QObject::tr(
                        "Schema patch %1 changed after it was applied (checksum mismatch). "
                        "Applied patches must not be edited; add a new patch instead.")
                                        .arg(patch.name);
                return false;
            }
        }
    }

    QList<Patch> pending;
    for (const Patch& patch : patches) {
        const bool alreadyApplied =
            std::any_of(applied.cbegin(), applied.cend(),
                        [&patch](const AppliedPatch& entry) {
                            return sameVersion(entry.version, patch.version);
                        });
        if (!alreadyApplied)
            pending.append(patch);
    }

    // Legacy database: clinical tables exist but there is no history. Record
    // the initial patch as applied without running it, so existing data is
    // preserved, then continue with the remaining patches.
    if (applied.isEmpty() && tableExists(database, QStringLiteral("b01_paciente")) &&
        !pending.isEmpty()) {
        const Patch& initial = pending.first();
        if (!recordBaseline(database, initial, errorMessage))
            return false;
        AppliedPatch entry;
        entry.version = initial.version;
        entry.checksum = checksumOf(initial.content);
        applied.append(entry);
        pending.removeFirst();
    }

    // A pending patch older than the highest applied one would run out of
    // order; abort instead of corrupting the schema.
    if (!applied.isEmpty()) {
        PatchVersion highest = applied.first().version;
        for (const AppliedPatch& entry : applied) {
            if (versionLess(highest, entry.version))
                highest = entry.version;
        }
        for (const Patch& patch : pending) {
            if (versionLess(patch.version, highest)) {
                if (errorMessage)
                    *errorMessage = QObject::tr(
                        "Schema patch %1 is out of order: version %2 is older than the "
                        "already applied %3.")
                                        .arg(patch.name, patch.version.full(), highest.full());
                return false;
            }
        }
    }

    for (const Patch& patch : pending) {
        if (!applyPatch(database, patch, errorMessage))
            return false;
    }
    return true;
}

bool SchemaMigrator::migrate(QSqlDatabase& database, QString* errorMessage) {
    return migrate(database, QStringLiteral(":/bd"), errorMessage);
}

} // namespace gambasse
