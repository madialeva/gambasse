#pragma once

#include <QList>
#include <QString>
#include <QVariant>
#include <QtMath>
#include <functional>

namespace gambasse {

// Column map of one consultation table: which column holds which member of
// the model and how it converts. Database reads and writes the consultations
// through it, so the SQL of the three tables is written once. Only the columns
// listed here are touched: the others keep their schema default on insert and
// their value on update.
template <typename M>
struct ConsultationTable {
    struct Column {
        QString name;
        std::function<QVariant(const M&)> read;
        std::function<void(M&, const QVariant&)> write;
    };

    QString table;
    QString dateColumn;
    QList<Column> columns;

    // TEXT NOT NULL: a null QString is bound as an empty string.
    void text(const QString& column, std::function<QString&(M&)> member) {
        columns.append(Column{column,
                              [member](const M& m) {
                                  const QString& s = member(const_cast<M&>(m));
                                  return QVariant(s.isNull() ? QString(QLatin1String("")) : s);
                              },
                              [member](M& m, const QVariant& v) { member(m) = v.toString(); }});
    }
    void integer(const QString& column, std::function<int&(M&)> member) {
        columns.append(Column{column,
                              [member](const M& m) { return QVariant(member(const_cast<M&>(m))); },
                              [member](M& m, const QVariant& v) { member(m) = v.toInt(); }});
    }
    void boolean(const QString& column, std::function<bool&(M&)> member) {
        columns.append(Column{column,
                              [member](const M& m) {
                                  return QVariant(member(const_cast<M&>(m)) ? 1 : 0);
                              },
                              [member](M& m, const QVariant& v) { member(m) = v.toInt() != 0; }});
    }
    // Decimal stored as an integer times `scale` (the VB.NET ORM scaling).
    void scaled(const QString& column, std::function<double&(M&)> member, int scale) {
        columns.append(Column{column,
                              [member, scale](const M& m) {
                                  return QVariant(qRound(member(const_cast<M&>(m)) * scale));
                              },
                              [member, scale](M& m, const QVariant& v) {
                                  member(m) = v.toDouble() / scale;
                              }});
    }
};

// The column map of each consultation model, defined next to the SQL.
template <typename M>
const ConsultationTable<M>& consultationTable();

} // namespace gambasse
