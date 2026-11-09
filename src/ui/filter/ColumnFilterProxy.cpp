#include <ui/filter/ColumnFilterProxy.h>

#include <QModelIndex>
#include <QRegularExpression>

#include <logic/TextOrder.h>

namespace gambasse {

ColumnFilterProxy::ColumnFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent) {}

void ColumnFilterProxy::setSortLocale(const QLocale& locale) {
    if (locale == m_sortLocale)
        return;
    m_sortLocale = locale;
    // sort() does nothing while the column and the order stay the same;
    // invalidate() sorts again and keeps the persistent indexes.
    if (sortColumn() >= 0)
        invalidate();
}

bool ColumnFilterProxy::lessThan(const QModelIndex& sourceLeft,
                                 const QModelIndex& sourceRight) const {
    const QVariant left = sourceLeft.data(sortRole());
    const QVariant right = sourceRight.data(sortRole());
    if (left.typeId() == QMetaType::QString && right.typeId() == QMetaType::QString)
        return compareText(left.toString(), right.toString(), m_sortLocale) < 0;
    // Numbers (code, age) by their value.
    return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
}

void ColumnFilterProxy::setColumnFilter(int column, const QString& text) {
    beginFilterChange();
    if (text.isEmpty())
        m_filters.remove(column);
    else
        m_filters.insert(column, text);
    endFilterChange();
}

void ColumnFilterProxy::clearFilters() {
    beginFilterChange();
    m_filters.clear();
    endFilterChange();
}

bool ColumnFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if (m_filters.isEmpty())
        return true;

    const QAbstractItemModel* model = sourceModel();
    for (auto it = m_filters.constBegin(); it != m_filters.constEnd(); ++it) {
        const int column = it.key();
        const QString& userPattern = it.value();

        const QModelIndex index = model->index(sourceRow, column, sourceParent);
        const QString cellText = model->data(index, Qt::DisplayRole).toString();

        // Build a case-insensitive regular expression anchored at the start,
        // translating '*' to '.*'. Without an end anchor, "mar" matches
        // "Maria" as a prefix and "*mar" matches "premar".
        QString pattern = QRegularExpression::escape(userPattern);
        pattern.replace(QStringLiteral("\\*"), QStringLiteral(".*"));
        const QRegularExpression expression(QStringLiteral("^") + pattern,
                                    QRegularExpression::CaseInsensitiveOption);

        if (!expression.match(cellText).hasMatch())
            return false;
    }
    return true;
}

} // namespace gambasse
