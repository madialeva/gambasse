#include <ui/controller/AdultConsultationController.h>

namespace gambasse {

AdultConsultationController::AdultConsultationController(QObject* parent)
    : ConsultationController(std::make_unique<ConsultationStore<AdultConsultation>>(), parent) {
    buildFields();
}

void AdultConsultationController::buildFields() {
    using M = AdultConsultation;
    M& c = store<M>().current;

    // The reason lives in the Consultation base: bound by pointer.
    addText(QStringLiteral("reason"), &c.reason, 500);

    // Lesions: region number (2 digits, like the VB.NET field) and impairment.
    for (int i = 0; i < 8; ++i) {
        addInt(QStringLiteral("lesionRegion%1").arg(i + 1), &c.lesionRegion[i], 0, 99);
        addText(QStringLiteral("lesionImpairment%1").arg(i + 1), &c.lesionImpairment[i], 100);
    }

    // Physical examination: respiratory, circulatory, skin and mucosa.
    addBool(QStringLiteral("nasalFlaring"), c, &M::nasalFlaring);
    addBool(QStringLiteral("stridor"), c, &M::stridor);
    addBool(QStringLiteral("wheezing"), c, &M::wheezing);
    addBool(QStringLiteral("retraction"), c, &M::retraction);
    addBool(QStringLiteral("arrhythmia"), c, &M::arrhythmia);
    addBool(QStringLiteral("capillaryRefill"), c, &M::capillaryRefill);
    addBool(QStringLiteral("absentPulse"), c, &M::absentPulse);
    addBool(QStringLiteral("edema"), c, &M::edema);
    addBool(QStringLiteral("skinFolds"), c, &M::skinFolds);
    addBool(QStringLiteral("paleMucosa"), c, &M::paleMucosa);
    addBool(QStringLiteral("sunkenEyes"), c, &M::sunkenEyes);
    addBool(QStringLiteral("palePalms"), c, &M::palePalms);

    // Endocrine-metabolic.
    addBool(QStringLiteral("organomegaly"), c, &M::organomegaly);
    addText(QStringLiteral("organomegalyDescription"), c, &M::organomegalyDescription, 50);
    addBool(QStringLiteral("lymphadenopathy"), c, &M::lymphadenopathy);
    addText(QStringLiteral("lymphadenopathyDescription"), c, &M::lymphadenopathyDescription, 50);

    // Elimination.
    addBool(QStringLiteral("secretion"), c, &M::secretion);
    addBool(QStringLiteral("stoolChanges"), c, &M::stoolChanges);
    addBool(QStringLiteral("cough"), c, &M::cough);
    addText(QStringLiteral("coughDescription"), c, &M::coughDescription, 50);
    addBool(QStringLiteral("urineChanges"), c, &M::urineChanges);
    addText(QStringLiteral("urineChangesDescription"), c, &M::urineChangesDescription, 50);
    addBool(QStringLiteral("vomiting"), c, &M::vomiting);

    // Health disorders and conduct.
    addText(QStringLiteral("dehydrationDegree"), c, &M::dehydrationDegree, 50);
    addText(QStringLiteral("malnutritionDegree"), c, &M::malnutritionDegree, 50);
    addText(QStringLiteral("other"), c, &M::other, 50);
    addBool(QStringLiteral("irritable"), c, &M::irritable);
    addText(QStringLiteral("otherCode"), c, &M::otherCode, 100);
}

} // namespace gambasse
