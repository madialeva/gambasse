#include <QCoreApplication>
#include <QTranslator>
#include <QtTest>

#include <logic/SexLabel.h>

namespace gambasse {

// Verifies the sex label source strings and their translations.
class TestSexLabel : public QObject {
    Q_OBJECT

private slots:
    void sourceStrings();
    void portugueseTranslations();
    void spanishTranslations();
};

void TestSexLabel::sourceStrings() {
    QCOMPARE(sexLabel(Patient::Sex::Home), QStringLiteral("Male"));
    QCOMPARE(sexLabel(Patient::Sex::Muller), QStringLiteral("Female"));
}

void TestSexLabel::portugueseTranslations() {
    QTranslator translator;
    QVERIFY(translator.load(QStringLiteral("gambasse_pt"), QStringLiteral(":/i18n")));
    QCoreApplication::installTranslator(&translator);

    QCOMPARE(sexLabel(Patient::Sex::Home), QStringLiteral("Homem"));
    QCOMPARE(sexLabel(Patient::Sex::Muller), QStringLiteral("Mulher"));

    QCoreApplication::removeTranslator(&translator);
}

void TestSexLabel::spanishTranslations() {
    QTranslator translator;
    QVERIFY(translator.load(QStringLiteral("gambasse_es"), QStringLiteral(":/i18n")));
    QCoreApplication::installTranslator(&translator);

    QCOMPARE(sexLabel(Patient::Sex::Home), QStringLiteral("Hombre"));
    QCOMPARE(sexLabel(Patient::Sex::Muller), QStringLiteral("Mujer"));

    QCoreApplication::removeTranslator(&translator);
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestSexLabel test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_sexlabel.moc"
