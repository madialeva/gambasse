#pragma once

#include <QSortFilterProxyModel>
#include <QHash>
#include <QLocale>
#include <QString>

namespace gambasse {

// Column filter proxy. It keeps one text pattern per column and accepts a row
// only if it matches ALL filtered columns.
//
// Semantics:
//   - Without '*': case-insensitive PREFIX matching ("mar" -> "Maria").
//   - '*' is a wildcard for "any sequence" ("*mar" -> "remar", "premar", "Maria").
//
// Sorting: text in the alphabetical order of the sort locale (compareText(),
// the same on every system), numbers by their value.
class ColumnFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit ColumnFilterProxy(QObject* parent = nullptr);

    // Sets (or clears when text is empty) a column filter.
    void setColumnFilter(int column, const QString& text);
    void clearFilters();

    // The locale whose alphabet orders the text; changing it sorts again.
    QLocale sortLocale() const { return m_sortLocale; }
    void setSortLocale(const QLocale& locale);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight) const override;

private:
    QHash<int, QString> m_filters; // column -> text entered by the user
    QLocale m_sortLocale = QLocale(QLocale::English, QLocale::UnitedKingdom);
};

} // namespace gambasse
