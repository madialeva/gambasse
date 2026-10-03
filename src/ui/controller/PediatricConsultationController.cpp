#include <ui/controller/PediatricConsultationController.h>

#include <logic/PediatricConsultationLabels.h>

namespace gambasse {

PediatricConsultationController::PediatricConsultationController(QObject* parent)
    : ConsultationController(std::make_unique<ConsultationStore<PediatricConsultation>>(), parent) {
    buildFields();
    refreshLanguage();
}

QVariantMap PediatricConsultationController::domainOptions() const {
    return m_domainOptions;
}

void PediatricConsultationController::refreshLanguage() {
    m_domainOptions.clear();
    for (const QString& domain : pediatricDomains()) {
        QVariantList options;
        for (const DomainOption& option : pediatricDomainOptions(domain))
            options.append(QVariantMap{{QStringLiteral("value"), option.code},
                                       {QStringLiteral("label"), option.label}});
        m_domainOptions.insert(domain, options);
    }
    emit domainOptionsChanged();
}

void PediatricConsultationController::buildFields() {
    using M = PediatricConsultation;
    M& c = store<M>().current;
    // Combo fields keep only the codes of their domain.
    const auto code = [this, &c](const QString& name, int M::*member, const char* domain) {
        addCode(name, &(c.*member), pediatricDomainCodes(QLatin1String(domain)));
    };
    // Text fields without a length in the VB.NET form get the limit of their
    // equivalents: 100 on one line, 500 multiline.
    const auto text = [this, &c](const QString& name, QString M::*member, int maxLength) {
        addText(name, &(c.*member), maxLength);
    };
    const auto check = [this, &c](const QString& name, bool M::*member) {
        addBool(name, &(c.*member));
    };
    const auto integer = [this, &c](const QString& name, int M::*member, int high) {
        addInt(name, &(c.*member), 0, high);
    };

    // The reason lives in the Consultation base: bound by pointer.
    addText(QStringLiteral("reason"), &c.reason, 500);
    code(QStringLiteral("nursingDiagnosis"), &M::nursingDiagnosis, "nursingDiagnosis");
    text(QStringLiteral("otherDiagnosis"), &M::otherDiagnosis, 500);

    // Clinical signs.
    addDecimal(QStringLiteral("weight"), c, &M::weight, 2, 99.99);
    integer(QStringLiteral("length"), &M::length, 999);
    code(QStringLiteral("weightRatio"), &M::weightRatio, "ratio");
    code(QStringLiteral("heightRatio"), &M::heightRatio, "ratio");
    code(QStringLiteral("consciousness"), &M::consciousness, "consciousness");
    addDecimal(QStringLiteral("temperature"), c, &M::temperature, 1, 999.9);
    integer(QStringLiteral("bloodGlucose"), &M::bloodGlucose, 999);
    code(QStringLiteral("mentalStatus"), &M::mentalStatus, "mentalStatus");
    code(QStringLiteral("armColor"), &M::armColor, "armColor");
    code(QStringLiteral("appetiteTest"), &M::appetiteTest, "test");
    code(QStringLiteral("malariaTest"), &M::malariaTest, "test");
    check(QStringLiteral("convulsion"), &M::convulsion);
    check(QStringLiteral("bradypnea"), &M::bradypnea);
    check(QStringLiteral("drinkingEating"), &M::drinkingEating);

    // Respiratory.
    code(QStringLiteral("retractionType"), &M::retractionType, "retractionType");
    code(QStringLiteral("retractionSeverity"), &M::retractionSeverity, "severity");
    code(QStringLiteral("tachypnea"), &M::tachypnea, "severity");
    check(QStringLiteral("burning"), &M::burning);
    check(QStringLiteral("grunting"), &M::grunting);
    check(QStringLiteral("runnyNose"), &M::runnyNose);
    code(QStringLiteral("wheeze"), &M::wheeze, "severity");
    code(QStringLiteral("secretions"), &M::secretions, "secretions");
    code(QStringLiteral("stridor"), &M::stridor, "severity");
    check(QStringLiteral("congestion"), &M::congestion);
    check(QStringLiteral("tearing"), &M::tearing);
    check(QStringLiteral("soreThroat"), &M::soreThroat);
    check(QStringLiteral("cough"), &M::cough);
    integer(QStringLiteral("coughDays"), &M::coughDays, 99);
    integer(QStringLiteral("oxygenSaturation"), &M::oxygenSaturation, 999);

    // Elimination.
    code(QStringLiteral("stool"), &M::stool, "stool");
    text(QStringLiteral("bleeding"), &M::bleeding, 100);
    code(QStringLiteral("urine"), &M::urine, "urine");
    code(QStringLiteral("vomiting"), &M::vomiting, "vomiting");

    // Circulatory.
    code(QStringLiteral("capillaryRefill"), &M::capillaryRefill, "capillaryRefill");
    check(QStringLiteral("absentPulse"), &M::absentPulse);
    code(QStringLiteral("cyanosis"), &M::cyanosis, "cyanosis");
    text(QStringLiteral("edema"), &M::edema, 100);
    integer(QStringLiteral("heartRate"), &M::heartRate, 999);
    text(QStringLiteral("arrhythmia"), &M::arrhythmia, 100);

    // Skin.
    check(QStringLiteral("skinFolds"), &M::skinFolds);
    check(QStringLiteral("palePalms"), &M::palePalms);
    check(QStringLiteral("drySkin"), &M::drySkin);
    text(QStringLiteral("bodyLesions"), &M::bodyLesions, 500);
    check(QStringLiteral("petechiae"), &M::petechiae);
    check(QStringLiteral("pustules"), &M::pustules);
    check(QStringLiteral("jaundice"), &M::jaundice);
    code(QStringLiteral("exanthema"), &M::exanthema, "exanthema");
    code(QStringLiteral("infection"), &M::infection, "infection");
    code(QStringLiteral("skinEdema"), &M::skinEdema, "edema");

    // Eye.
    code(QStringLiteral("eyeDischarge"), &M::eyeDischarge, "eyeDischarge");
    check(QStringLiteral("eyelidEdema"), &M::eyelidEdema);
    check(QStringLiteral("stuckEyelids"), &M::stuckEyelids);
    check(QStringLiteral("redConjunctiva"), &M::redConjunctiva);
    check(QStringLiteral("paleConjunctiva"), &M::paleConjunctiva);
    check(QStringLiteral("sunkenEyes"), &M::sunkenEyes);

    // Ear.
    check(QStringLiteral("swellingBehindEar"), &M::swellingBehindEar);
    code(QStringLiteral("earPain"), &M::earPain, "earPain");
    code(QStringLiteral("earDischarge"), &M::earDischarge, "earDischarge");
    check(QStringLiteral("itchyEarCanal"), &M::itchyEarCanal);
    code(QStringLiteral("otoscopy"), &M::otoscopy, "otoscopy");

    // Mouth.
    code(QStringLiteral("tonsils"), &M::tonsils, "tonsils");
    code(QStringLiteral("mouthPain"), &M::mouthPain, "mouthPain");
    text(QStringLiteral("lesionType"), &M::lesionType, 100);
    check(QStringLiteral("koplikSpots"), &M::koplikSpots);
    check(QStringLiteral("whitePatches"), &M::whitePatches);
    check(QStringLiteral("redTongue"), &M::redTongue);
    check(QStringLiteral("palatalPetechiae"), &M::palatalPetechiae);
    check(QStringLiteral("cervicalNodes"), &M::cervicalNodes);

    // Abdomen / pelvis.
    code(QStringLiteral("abdominalPainSigns"), &M::abdominalPainSigns, "abdominalPainSigns");

    // Neurological.
    check(QStringLiteral("severeHeadache"), &M::severeHeadache);
    check(QStringLiteral("photophobia"), &M::photophobia);
    check(QStringLiteral("brudzinskiSign"), &M::brudzinskiSign);
    check(QStringLiteral("kernigSign"), &M::kernigSign);
    check(QStringLiteral("neckStiffness"), &M::neckStiffness);
    check(QStringLiteral("bulgingFontanelle"), &M::bulgingFontanelle);
    check(QStringLiteral("developmentalProblems"), &M::developmentalProblems);
    text(QStringLiteral("developmentalProblemsDescription"), &M::developmentalProblemsDescription,
         100);

    // Endocrine.
    check(QStringLiteral("excessiveHunger"), &M::excessiveHunger);
    check(QStringLiteral("excessiveThirst"), &M::excessiveThirst);
    check(QStringLiteral("asthenia"), &M::asthenia);
    text(QStringLiteral("lymphadenopathy"), &M::lymphadenopathy, 50);
    text(QStringLiteral("rapidWeightLoss"), &M::rapidWeightLoss, 100);
    text(QStringLiteral("breathOdor"), &M::breathOdor, 100);

    // Indicated treatments, rows 1-10.
    const QList<int> ingredients = pediatricDomainCodes(QStringLiteral("activeIngredient"));
    const QList<int> routes = pediatricDomainCodes(QStringLiteral("administrationRoute"));
    const QList<int> days = pediatricDomainCodes(QStringLiteral("administrationDays"));
    for (int i = 0; i < 10; ++i) {
        const QString n = QString::number(i + 1);
        PediatricConsultation::Treatment& row = c.treatments[i];
        addCode(QStringLiteral("treatmentMedication") + n, &row.medication, ingredients);
        addText(QStringLiteral("treatmentDescription") + n, &row.description, 100);
        addCode(QStringLiteral("treatmentRoute") + n, &row.route, routes);
        addCode(QStringLiteral("treatmentDays") + n, &row.days, days);
        addText(QStringLiteral("treatmentDose") + n, &row.dose, 100);
        addText(QStringLiteral("treatmentFrequency") + n, &row.frequency, 100);
        addInt(QStringLiteral("treatmentPerfusion") + n, &row.perfusion, 0, 99999);
    }

    // Child care recommendations.
    const QList<int> recommendations = pediatricDomainCodes(QStringLiteral("careRecommendation"));
    for (int i = 0; i < 5; ++i)
        addCode(QStringLiteral("recommendation%1").arg(i + 1), &c.recommendations[i],
                recommendations);
}

} // namespace gambasse
