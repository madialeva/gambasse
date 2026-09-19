#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include <data/SchemaMigrator.h>

namespace gambasse {

// Exercises the schema migration engine: embedded patch discovery and order,
// the b00_bd_migrations history, the legacy baseline, checksum validation and
// transactional failure handling.
class TestSchemaMigrator : public QObject {
    Q_OBJECT

private slots:
    void freshDatabaseAppliesEveryPatch();
    void legacyDatabaseIsBaselined();
    void outOfOrderPatchAborts();
    void modifiedAppliedPatchIsRejected();
    void failingPatchIsRolledBack();
};

namespace {

bool writePatch(const QString& directory, const QString& name, const QString& content) {
    QFile file(QDir(directory).filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(content.toUtf8()) == content.toUtf8().size();
}

bool hasTable(QSqlDatabase& db, const QString& table) {
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=? LIMIT 1"));
    query.addBindValue(table);
    return query.exec() && query.next();
}

int historyCount(QSqlDatabase& db) {
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM b00_bd_migrations")) ||
        !query.next())
        return -1;
    return query.value(0).toInt();
}

int rowCount(QSqlDatabase& db, const QString& table) {
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(table)) || !query.next())
        return -1;
    return query.value(0).toInt();
}

} // namespace

void TestSchemaMigrator::freshDatabaseAppliesEveryPatch() {
    QTemporaryDir patches;
    QVERIFY(patches.isValid());
    QVERIFY(writePatch(patches.path(), QStringLiteral("1.0.0.1_a.sql"),
                       QStringLiteral("CREATE TABLE t_a (id INTEGER);")));
    QVERIFY(writePatch(patches.path(), QStringLiteral("1.0.0.2_b.sql"),
                       QStringLiteral("CREATE TABLE t_b (id INTEGER);\n"
                                      "INSERT INTO t_b VALUES (1);")));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString connection = QStringLiteral("fresh");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        db.setDatabaseName(dir.filePath(QStringLiteral("database.db")));
        QVERIFY(db.open());

        QString error;
        QVERIFY2(SchemaMigrator::migrate(db, patches.path(), &error), qPrintable(error));
        QVERIFY(hasTable(db, QStringLiteral("t_a")));
        QVERIFY(hasTable(db, QStringLiteral("t_b")));
        QCOMPARE(historyCount(db), 2);

        // Re-running applies nothing and keeps the history untouched.
        QVERIFY2(SchemaMigrator::migrate(db, patches.path(), &error), qPrintable(error));
        QCOMPARE(historyCount(db), 2);
        db.close();
    }
    QSqlDatabase::removeDatabase(connection);
}

void TestSchemaMigrator::legacyDatabaseIsBaselined() {
    QTemporaryDir patches;
    QVERIFY(patches.isValid());
    QVERIFY(writePatch(patches.path(), QStringLiteral("1.0.0.1_initial.sql"),
                       QStringLiteral("CREATE TABLE initial_ran (id INTEGER);")));
    QVERIFY(writePatch(patches.path(), QStringLiteral("1.0.0.2_extra.sql"),
                       QStringLiteral("CREATE TABLE extra (id INTEGER);")));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString connection = QStringLiteral("legacy");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        db.setDatabaseName(dir.filePath(QStringLiteral("database.db")));
        QVERIFY(db.open());
        // Simulate an existing legacy database: clinical table with data, no history.
        QSqlQuery create(db);
        QVERIFY(create.exec(QStringLiteral("CREATE TABLE b01_paciente (b01_id INTEGER)")));
        QVERIFY(create.exec(QStringLiteral("INSERT INTO b01_paciente VALUES (7)")));

        QString error;
        QVERIFY2(SchemaMigrator::migrate(db, patches.path(), &error), qPrintable(error));
        QCOMPARE(historyCount(db), 2);
        QCOMPARE(rowCount(db, QStringLiteral("b01_paciente")), 1);
        // The initial patch was recorded but NOT executed.
        QVERIFY(!hasTable(db, QStringLiteral("initial_ran")));
        // Later patches are applied as usual.
        QVERIFY(hasTable(db, QStringLiteral("extra")));
        db.close();
    }
    QSqlDatabase::removeDatabase(connection);
}

void TestSchemaMigrator::outOfOrderPatchAborts() {
    QTemporaryDir patches;
    QVERIFY(patches.isValid());
    QVERIFY(writePatch(patches.path(), QStringLiteral("1.0.0.1_a.sql"),
                       QStringLiteral("CREATE TABLE t_a (id INTEGER);")));
    QVERIFY(writePatch(patches.path(), QStringLiteral("1.1.0.1_c.sql"),
                       QStringLiteral("CREATE TABLE t_c (id INTEGER);")));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString connection = QStringLiteral("outoforder");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        db.setDatabaseName(dir.filePath(QStringLiteral("database.db")));
        QVERIFY(db.open());

        QString error;
        QVERIFY2(SchemaMigrator::migrate(db, patches.path(), &error), qPrintable(error));
        QCOMPARE(historyCount(db), 2);

        // A patch older than the highest applied version must abort.
        QVERIFY(writePatch(patches.path(), QStringLiteral("1.0.1.1_b.sql"),
                           QStringLiteral("CREATE TABLE t_b (id INTEGER);")));
        QVERIFY(!SchemaMigrator::migrate(db, patches.path(), &error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(historyCount(db), 2);
        QVERIFY(!hasTable(db, QStringLiteral("t_b")));
        db.close();
    }
    QSqlDatabase::removeDatabase(connection);
}

void TestSchemaMigrator::modifiedAppliedPatchIsRejected() {
    QTemporaryDir patches;
    QVERIFY(patches.isValid());
    QVERIFY(writePatch(patches.path(), QStringLiteral("1.0.0.1_a.sql"),
                       QStringLiteral("CREATE TABLE t_a (id INTEGER);")));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString connection = QStringLiteral("checksum");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        db.setDatabaseName(dir.filePath(QStringLiteral("database.db")));
        QVERIFY(db.open());

        QString error;
        QVERIFY2(SchemaMigrator::migrate(db, patches.path(), &error), qPrintable(error));

        // Editing an applied patch changes its checksum and must be rejected.
        QVERIFY(writePatch(patches.path(), QStringLiteral("1.0.0.1_a.sql"),
                           QStringLiteral("CREATE TABLE t_a (id INTEGER);\n-- edited")));
        QVERIFY(!SchemaMigrator::migrate(db, patches.path(), &error));
        QVERIFY(!error.isEmpty());
        db.close();
    }
    QSqlDatabase::removeDatabase(connection);
}

void TestSchemaMigrator::failingPatchIsRolledBack() {
    QTemporaryDir patches;
    QVERIFY(patches.isValid());
    QVERIFY(writePatch(patches.path(), QStringLiteral("1.0.0.1_a.sql"),
                       QStringLiteral("CREATE TABLE t_a (id INTEGER);")));
    QVERIFY(writePatch(patches.path(), QStringLiteral("1.0.0.2_bad.sql"),
                       QStringLiteral("CREATE TABLE rolled_back (id INTEGER);\n"
                                      "THIS IS NOT VALID SQL;")));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString connection = QStringLiteral("rollback");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        db.setDatabaseName(dir.filePath(QStringLiteral("database.db")));
        QVERIFY(db.open());

        QString error;
        QVERIFY(!SchemaMigrator::migrate(db, patches.path(), &error));
        QVERIFY(!error.isEmpty());
        // The first patch committed, the failing one left no trace.
        QCOMPARE(historyCount(db), 1);
        QVERIFY(hasTable(db, QStringLiteral("t_a")));
        QVERIFY(!hasTable(db, QStringLiteral("rolled_back")));
        db.close();
    }
    QSqlDatabase::removeDatabase(connection);
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestSchemaMigrator test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_schemamigrator.moc"
