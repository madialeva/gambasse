#include <QApplication>
#include <QStandardItemModel>
#include <QtTest>

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
};

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

} // namespace gambasse

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    gambasse::TestColumnFilterProxy test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_columnfilterproxy.moc"
