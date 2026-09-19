#pragma once

#include <QSortFilterProxyModel>
#include <QHash>
#include <QString>

namespace gambasse {

// Column filter proxy. It keeps one text pattern per column and accepts a row
// only if it matches ALL filtered columns.
//
// Semantics:
//   - Without '*': case-insensitive PREFIX matching ("mar" -> "Maria").
//   - '*' is a wildcard for "any sequence" ("*mar" -> "remar", "premar", "Maria").
class ColumnFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit ColumnFilterProxy(QObject* parent = nullptr);

    // Sets (or clears when text is empty) a column filter.
    void setColumnFilter(int column, const QString& text);
    void clearFilters();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QHash<int, QString> m_filters; // column -> text entered by the user
};

} // namespace gambasse
