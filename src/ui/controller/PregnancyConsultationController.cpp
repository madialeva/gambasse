#include <ui/controller/PregnancyConsultationController.h>

namespace gambasse {

PregnancyConsultationController::PregnancyConsultationController(QObject* parent)
    : ConsultationController(std::make_unique<ConsultationStore<PregnancyConsultation>>(), parent) {
    buildFields();
}

void PregnancyConsultationController::buildFields() {
    using M = PregnancyConsultation;
    M& c = store<M>().current;

    // The reason lives in the Consultation base: bound by pointer.
    addText(QStringLiteral("reason"), &c.reason, 500);

    // Treatments given now: dose, medication and next dose per row.
    const auto supplement = [this](const QString& name, M::Supplement& row) {
        addText(name + QStringLiteral("D"), &row.dose, 50);
        addText(name + QStringLiteral("M"), &row.medication, 50);
        addText(name + QStringLiteral("Pd"), &row.nextDose, 50);
    };
    supplement(QStringLiteral("supplementCalcium"), c.calcium);
    supplement(QStringLiteral("supplementVitaminC"), c.vitaminC);
    supplement(QStringLiteral("supplementOtherVitamins"), c.otherVitamins);
    supplement(QStringLiteral("supplementMalariaProphylaxis"), c.malariaProphylaxis);

    // Physical examination.
    addDecimal(QStringLiteral("weight"), c, &M::weight, 1, 999.9);
    addDecimal(QStringLiteral("temperature"), c, &M::temperature, 1, 99.9);
    addInt(QStringLiteral("height"), c, &M::height, 0, 999);
    addInt(QStringLiteral("abdominalPerimeter"), c, &M::abdominalPerimeter, 0, 999);
    addText(QStringLiteral("vomiting"), c, &M::vomiting, 100);
    addText(QStringLiteral("fetalAuscultation"), c, &M::fetalAuscultation, 100);
    addText(QStringLiteral("fetalPosition"), c, &M::fetalPosition, 100);
    addInt(QStringLiteral("uterineHeightWeeks"), c, &M::uterineHeightWeeks, 0, 999);
    addInt(QStringLiteral("uterineHeightCm"), c, &M::uterineHeightCm, 0, 999);
    addBool(QStringLiteral("leukocytosis"), c, &M::leukocytosis);
    addBool(QStringLiteral("proteinuria"), c, &M::proteinuria);
    addBool(QStringLiteral("edemaLegs"), c, &M::edemaLegs);
    addBool(QStringLiteral("edemaFace"), c, &M::edemaFace);
    addBool(QStringLiteral("edemaGeneral"), c, &M::edemaGeneral);
}

} // namespace gambasse
