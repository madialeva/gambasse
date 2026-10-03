#include <QCoreApplication>
#include <QTranslator>
#include <QtTest>

#include <logic/PediatricConsultationLabels.h>

namespace gambasse {

// Verifies the pediatric consultation domains against the VB.NET lists
// (LISTA_TIPOS_* in ConsultaPediatrica.vb): number of options and codes per
// domain, the empty option first, and labels that follow the translator.
class TestPediatricConsultationLabels : public QObject {
    Q_OBJECT

private slots:
    void everyDomainMatchesTheVbList_data();
    void everyDomainMatchesTheVbList();
    void activeIngredientsKeepTheirCodes();
    void unknownDomainIsEmpty();
    void labelsFollowTheTranslator();
};

void TestPediatricConsultationLabels::everyDomainMatchesTheVbList_data() {
    QTest::addColumn<QString>("domain");
    QTest::addColumn<int>("count");
    const QList<QPair<const char*, int>> domains{
        {"nursingDiagnosis", 8},  {"consciousness", 5},     {"mentalStatus", 9},
        {"armColor", 5},          {"test", 3},              {"ratio", 10},
        {"retractionType", 4},    {"severity", 4},          {"secretions", 4},
        {"stool", 5},             {"vomiting", 6},          {"urine", 7},
        {"capillaryRefill", 3},   {"cyanosis", 3},          {"exanthema", 4},
        {"infection", 3},         {"edema", 3},             {"eyeDischarge", 4},
        {"earPain", 5},           {"earDischarge", 5},      {"otoscopy", 6},
        {"tonsils", 6},           {"mouthPain", 3},         {"abdominalPainSigns", 7},
        {"careRecommendation", 14}, {"activeIngredient", 153}, {"administrationRoute", 16},
        {"administrationDays", 18}};
    for (const auto& d : domains)
        QTest::newRow(d.first) << QString::fromLatin1(d.first) << d.second;
}

void TestPediatricConsultationLabels::everyDomainMatchesTheVbList() {
    QFETCH(QString, domain);
    QFETCH(int, count);
    QVERIFY(pediatricDomains().contains(domain));
    const QList<DomainOption> options = pediatricDomainOptions(domain);
    QCOMPARE(options.size(), count);
    QCOMPARE(pediatricDomainCodes(domain).size(), count);
    QCOMPARE(options.first().code, 0);
    QCOMPARE(options.first().label, QStringLiteral("-"));
    // Codes are unique and, but for the active ingredients, 0..count-1.
    QSet<int> seen;
    for (int i = 0; i < options.size(); ++i) {
        QVERIFY(!seen.contains(options.at(i).code));
        seen.insert(options.at(i).code);
        QVERIFY(!options.at(i).label.isEmpty());
        if (domain != QLatin1String("activeIngredient"))
            QCOMPARE(options.at(i).code, i);
    }
}

void TestPediatricConsultationLabels::activeIngredientsKeepTheirCodes() {
    QCOMPARE(pediatricDomains().size(), 28);
    const QList<int> codes = pediatricDomainCodes(QStringLiteral("activeIngredient"));
    QCOMPARE(codes.at(149), 149);
    QCOMPARE(codes.at(150), 1000);
    QCOMPARE(codes.at(151), 1001);
    QCOMPARE(codes.at(152), 1002);
    const QList<DomainOption> options = pediatricDomainOptions(QStringLiteral("activeIngredient"));
    QCOMPARE(options.at(151).label, QStringLiteral("Disinfectant:"));
    QCOMPARE(options.at(118).label, QStringLiteral("Paracetamol"));
}

void TestPediatricConsultationLabels::unknownDomainIsEmpty() {
    QVERIFY(pediatricDomainOptions(QStringLiteral("nope")).isEmpty());
    QVERIFY(pediatricDomainCodes(QStringLiteral("nope")).isEmpty());
}

void TestPediatricConsultationLabels::labelsFollowTheTranslator() {
    const auto label = [](const char* domain, int index) {
        return pediatricDomainOptions(QLatin1String(domain)).at(index).label;
    };
    QCOMPARE(label("severity", 1), QStringLiteral("SEVERE"));
    QCOMPARE(label("activeIngredient", 151), QStringLiteral("Disinfectant:"));

    QTranslator portuguese;
    QVERIFY(portuguese.load(QStringLiteral("gambasse_pt"), QStringLiteral(":/i18n")));
    QCoreApplication::installTranslator(&portuguese);
    // Portuguese keeps the text the VB.NET form showed.
    QCOMPARE(label("severity", 1), QStringLiteral("SERIO"));
    QCOMPARE(label("armColor", 1), QStringLiteral("VERMELLO"));
    QCOMPARE(label("activeIngredient", 151), QStringLiteral("Desinfectante:"));
    QCoreApplication::removeTranslator(&portuguese);

    QTranslator spanish;
    QVERIFY(spanish.load(QStringLiteral("gambasse_es"), QStringLiteral(":/i18n")));
    QCoreApplication::installTranslator(&spanish);
    QCOMPARE(label("armColor", 1), QStringLiteral("ROJO"));
    QCOMPARE(label("stool", 4), QStringLiteral("ESTREÑIMIENTO"));
    QCOMPARE(label("administrationDays", 17), QStringLiteral("2 MESES"));
    QCoreApplication::removeTranslator(&spanish);
}

} // namespace gambasse

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    gambasse::TestPediatricConsultationLabels test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_pediatricconsultationlabels.moc"
