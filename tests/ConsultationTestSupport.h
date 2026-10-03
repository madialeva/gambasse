#pragma once

// Helpers shared by the consultation service tests: a patient with the
// history each consultation table references, and a round trip over every
// column of a consultation table.

#include <QSqlQuery>
#include <QtTest>

#include <data/common/ConsultationTable.h>
#include <data/common/Database.h>

namespace gambasse {

// Inserts a patient (and the history row the consultation foreign key needs,
// when historyTable is not empty).
inline bool insertPatientWithHistory(qlonglong id, const QString& historyTable) {
    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral(
        "INSERT INTO b01_patient (id, name, birth_date, sex, approximate_age) "
        "VALUES (?, ?, '2015-05-06', 0, 30)"));
    q.addBindValue(id);
    q.addBindValue(QStringLiteral("CONSULTATION_%1").arg(id));
    if (!q.exec())
        return false;
    if (historyTable.isEmpty())
        return true;
    QSqlQuery h(Database::instance().connection());
    h.prepare(QStringLiteral("INSERT INTO %1 (patient_id) VALUES (?)").arg(historyTable));
    h.addBindValue(id);
    return h.exec();
}

// A value that differs per column and from every schema default: texts carry
// the column index, integers are index + 1 (a legal domain code range is not
// needed at this layer), booleans are true.
template <typename M>
void fillDistinct(M& consultation) {
    const auto& columns = consultationTable<M>().columns;
    for (int i = 0; i < columns.size(); ++i) {
        const QVariant current = columns.at(i).read(consultation);
        if (current.typeId() == QMetaType::QString)
            columns.at(i).write(consultation, QStringLiteral("v%1").arg(i));
        else
            columns.at(i).write(consultation, i + 1);
    }
}

// Compares every column of two consultations through the column map.
template <typename M>
void compareColumns(const M& actual, const M& expected) {
    const auto& columns = consultationTable<M>().columns;
    for (const auto& column : columns) {
        QVERIFY2(column.read(actual) == column.read(expected),
                 qPrintable(QStringLiteral("%1: %2 != %3")
                                .arg(column.name, column.read(actual).toString(),
                                     column.read(expected).toString())));
    }
}

// Value of one column of the stored row, straight from SQL.
inline QVariant storedColumn(const QString& table, const QString& dateColumn, qlonglong patientId,
                             const QString& key, const QString& column) {
    QSqlQuery q(Database::instance().connection());
    q.prepare(QStringLiteral("SELECT %1 FROM %2 WHERE patient_id = ? AND %3 = ?")
                  .arg(column, table, dateColumn));
    q.addBindValue(patientId);
    q.addBindValue(key);
    if (!q.exec() || !q.next())
        return QVariant();
    return q.value(0);
}

} // namespace gambasse
