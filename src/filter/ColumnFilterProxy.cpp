#include "ColumnFilterProxy.h"

#include <QModelIndex>
#include <QRegularExpression>

namespace gambasse {

ColumnFilterProxy::ColumnFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent) {}

void ColumnFilterProxy::setColumnFilter(int column, const QString& text) {
    if (text.isEmpty())
        m_filters.remove(column);
    else
        m_filters.insert(column, text);
    invalidateFilter();
}

void ColumnFilterProxy::clearFilters() {
    m_filters.clear();
    invalidateFilter();
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
