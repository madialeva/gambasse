#include <QGuiApplication>
#include <QPersistentModelIndex>
#include <QStandardItemModel>
#include <QtTest>

#include <logic/InterfaceLocale.h>
#include <ui/filter/ColumnFilterProxy.h>

namespace gambasse {

// First unit test: exercises the column filter semantics documented in
// ColumnFilterProxy (prefix match, '*' wildcard, AND across columns).
class TestColumnFilterProxy : public QObject {
    Q_OBJECT

private slots:
    void noFilterAcceptsAll();
    void prefixMatchIsCaseInsensitive();
    void wildcardMatchesAnySequence();
    void allFilteredColumnsMustMatch();
    void emptyTextClearsColumnFilter();
    void clearFiltersRestoresAllRows();
    void sortsTextInTheLocaleOrder();
    void sortsNumbersByValue();
    void localeChangeSortsAgain();
};

// Names in column 0 and numbers in column 1, as in the patient grid.
static QStandardItemModel* makeSortModel(QObject* parent) {
    const QStringList names = QString::fromUtf8("OLIVEIRA|ÑANDÚ|maria|NUNO|ÁLVARO|ALVES").split(u'|');
    const QList<int> codes = {10, 2, 33, 7, 1, 100};
    auto* model = new QStandardItemModel(int(names.size()), 2, parent);
    for (int row = 0; row < names.size(); ++row) {
        model->setData(model->index(row, 0), names.at(row));
        model->setData(model->index(row, 1), codes.at(row));
    }
    return model;
}

static QStringList column(const QAbstractItemModel& model, int column) {
    QStringList values;
    for (int row = 0; row < model.rowCount(); ++row)
        values.append(model.data(model.index(row, column)).toString());
    return values;
}

static QStandardItemModel* makeModel(QObject* parent) {
    auto* model = new QStandardItemModel(3, 2, parent);
    model->setData(model->index(0, 0), QStringLiteral("Maria"));
    model->setData(model->index(0, 1), QStringLiteral("Bissau"));
    model->setData(model->index(1, 0), QStringLiteral("Mamadou"));
    model->setData(model->index(1, 1), QStringLiteral("Bafata"));
    model->setData(model->index(2, 0), QStringLiteral("Premar"));
    model->setData(model->index(2, 1), QStringLiteral("Bissau"));
    return model;
}

void TestColumnFilterProxy::noFilterAcceptsAll() {
    ColumnFilterProxy proxy;
    QStandardItemModel* model = makeModel(&proxy);
    proxy.setSourceModel(model);
    QCOMPARE(proxy.rowCount(), 3);
}

void TestColumnFilterProxy::prefixMatchIsCaseInsensitive() {
    ColumnFilterProxy proxy;
    QStandardItemModel* model = makeModel(&proxy);
    proxy.setSourceModel(model);

    proxy.setColumnFilter(0, QStringLiteral("mar"));
    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.data(proxy.index(0, 0)).toString(), QStringLiteral("Maria"));
}

void TestColumnFilterProxy::wildcardMatchesAnySequence() {
    ColumnFilterProxy proxy;
    QStandardItemModel* model = makeModel(&proxy);
    proxy.setSourceModel(model);

    proxy.setColumnFilter(0, QStringLiteral("*mar"));
    QCOMPARE(proxy.rowCount(), 2);
}

void TestColumnFilterProxy::allFilteredColumnsMustMatch() {
    ColumnFilterProxy proxy;
    QStandardItemModel* model = makeModel(&proxy);
    proxy.setSourceModel(model);

    proxy.setColumnFilter(0, QStringLiteral("*mar"));
    proxy.setColumnFilter(1, QStringLiteral("Bis"));
    QCOMPARE(proxy.rowCount(), 2);

    proxy.setColumnFilter(1, QStringLiteral("Baf"));
    QCOMPARE(proxy.rowCount(), 0);
}

void TestColumnFilterProxy::emptyTextClearsColumnFilter() {
    ColumnFilterProxy proxy;
    QStandardItemModel* model = makeModel(&proxy);
    proxy.setSourceModel(model);

    proxy.setColumnFilter(0, QStringLiteral("mar"));
    QCOMPARE(proxy.rowCount(), 1);
    proxy.setColumnFilter(0, QString());
    QCOMPARE(proxy.rowCount(), 3);
}

void TestColumnFilterProxy::clearFiltersRestoresAllRows() {
    ColumnFilterProxy proxy;
    QStandardItemModel* model = makeModel(&proxy);
    proxy.setSourceModel(model);

    proxy.setColumnFilter(0, QStringLiteral("*mar"));
    proxy.setColumnFilter(1, QStringLiteral("Bis"));
    QCOMPARE(proxy.rowCount(), 2);
    proxy.clearFilters();
    QCOMPARE(proxy.rowCount(), 3);
}

void TestColumnFilterProxy::sortsTextInTheLocaleOrder() {
    ColumnFilterProxy proxy;
    proxy.setSourceModel(makeSortModel(&proxy));
    proxy.setSortLocale(interfaceLocale(QStringLiteral("es")));
    proxy.sort(0, Qt::AscendingOrder);
    QCOMPARE(column(proxy, 0), QString::fromUtf8("ÁLVARO|ALVES|maria|NUNO|ÑANDÚ|OLIVEIRA").split(u'|'));
    proxy.sort(0, Qt::DescendingOrder);
    QCOMPARE(column(proxy, 0), QString::fromUtf8("OLIVEIRA|ÑANDÚ|NUNO|maria|ALVES|ÁLVARO").split(u'|'));
}

void TestColumnFilterProxy::sortsNumbersByValue() {
    ColumnFilterProxy proxy;
    proxy.setSourceModel(makeSortModel(&proxy));
    proxy.sort(1, Qt::AscendingOrder);
    QCOMPARE(column(proxy, 1), QStringList({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("7"),
                                            QStringLiteral("10"), QStringLiteral("33"),
                                            QStringLiteral("100")}));
}

void TestColumnFilterProxy::localeChangeSortsAgain() {
    ColumnFilterProxy proxy;
    proxy.setSourceModel(makeSortModel(&proxy));
    proxy.setSortLocale(interfaceLocale(QStringLiteral("es")));
    proxy.sort(0, Qt::AscendingOrder);
    const QPersistentModelIndex enye = proxy.index(4, 0);
    QCOMPARE(enye.data().toString(), QString::fromUtf8("ÑANDÚ"));

    // In Portuguese Ñ sorts as an N: same column and order, new places.
    proxy.setSortLocale(interfaceLocale(QStringLiteral("pt")));
    QCOMPARE(column(proxy, 0), QString::fromUtf8("ÁLVARO|ALVES|maria|ÑANDÚ|NUNO|OLIVEIRA").split(u'|'));
    QCOMPARE(proxy.sortColumn(), 0);
    QCOMPARE(proxy.sortOrder(), Qt::AscendingOrder);
    // Indexes the views hold follow their rows.
    QCOMPARE(enye.row(), 3);
    QCOMPARE(enye.data().toString(), QString::fromUtf8("ÑANDÚ"));
}

} // namespace gambasse

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    gambasse::TestColumnFilterProxy test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_columnfilterproxy.moc"
