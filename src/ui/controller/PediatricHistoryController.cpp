#include <ui/controller/PediatricHistoryController.h>

namespace gambasse {

PediatricHistoryController::PediatricHistoryController(QObject* parent) : HistoryController(parent) {
    buildFields();
}

const QList<HistoryController::Field>& PediatricHistoryController::fields() const {
    return m_fields;
}

bool PediatricHistoryController::serviceLoad() {
    return m_service.load(patientId(), m_history, m_isNewFlag());
}

int PediatricHistoryController::serviceSave() {
    m_history.patientId = patientId();
    return static_cast<int>(m_service.save(isNew(), m_history));
}

bool PediatricHistoryController::serviceRemove() {
    return m_service.remove(patientId());
}

void PediatricHistoryController::buildFields() {
    // Personal data.
    addDate(QStringLiteral("openingDate"), m_history, &PediatricHistory::openingDate);
    addText(QStringLiteral("allergies"), m_history, &PediatricHistory::allergies, 100);

    // Birth history.
    addInt(QStringLiteral("neonatalWeight"), m_history, &PediatricHistory::neonatalWeight, 0, 99999);
    addInt(QStringLiteral("neonatalHeight"), m_history, &PediatricHistory::neonatalHeight, 0, 999);
    addText(QStringLiteral("neonatalDeliveryIncidents"), m_history,
            &PediatricHistory::neonatalDeliveryIncidents, 100);
    addText(QStringLiteral("neonatalMalformations"), m_history,
            &PediatricHistory::neonatalMalformations, 100);

    // Child history.
    addText(QStringLiteral("clinicalDiseaseType"), m_history, &PediatricHistory::clinicalDiseaseType,
            3);
    addText(QStringLiteral("clinicalDiseaseTreatment"), m_history,
            &PediatricHistory::clinicalDiseaseTreatment, 100);
    addText(QStringLiteral("otherDiseases"), m_history, &PediatricHistory::otherDiseases, 200);

    // Housing.
    addBool(QStringLiteral("latrine"), m_history, &PediatricHistory::latrine);
    addBool(QStringLiteral("mosquitoNet"), m_history, &PediatricHistory::mosquitoNet);
    addBool(QStringLiteral("netUse"), m_history, &PediatricHistory::netUse);
    addBool(QStringLiteral("domesticHygiene"), m_history, &PediatricHistory::domesticHygiene);
    addEnum(QStringLiteral("domesticAnimals"), m_history, &PediatricHistory::domesticAnimals, 3);
    addText(QStringLiteral("conflicts"), m_history, &PediatricHistory::conflicts, 200);
    addText(QStringLiteral("economicSituation"), m_history, &PediatricHistory::economicSituation);

    // Feeding.
    addBool(QStringLiteral("feedingBreastfeeding"), m_history, &PediatricHistory::feedingBreastfeeding);
    addBool(QStringLiteral("feedingTreatedWater"), m_history, &PediatricHistory::feedingTreatedWater);
    addText(QStringLiteral("feedingTreatedWaterDetails"), m_history,
            &PediatricHistory::feedingTreatedWaterDetails);
    addText(QStringLiteral("feedingFeedingProblems"), m_history,
            &PediatricHistory::feedingFeedingProblems, 100);
    addText(QStringLiteral("feedingAdditionalFeeding"), m_history,
            &PediatricHistory::feedingAdditionalFeeding, 100);

    // Previous treatments: one date and one medication per row.
    addDate(QStringLiteral("supplementDate1"), m_history, &PediatricHistory::supplementDate1);
    addDate(QStringLiteral("supplementDate2"), m_history, &PediatricHistory::supplementDate2);
    addDate(QStringLiteral("supplementDate3"), m_history, &PediatricHistory::supplementDate3);
    addDate(QStringLiteral("supplementDate4"), m_history, &PediatricHistory::supplementDate4);
    addEnum(QStringLiteral("supplementTreatment1"), m_history,
            &PediatricHistory::supplementTreatment1, 7);
    addEnum(QStringLiteral("supplementTreatment2"), m_history,
            &PediatricHistory::supplementTreatment2, 7);
    addEnum(QStringLiteral("supplementTreatment3"), m_history,
            &PediatricHistory::supplementTreatment3, 7);
    addEnum(QStringLiteral("supplementTreatment4"), m_history,
            &PediatricHistory::supplementTreatment4, 7);

    // Vaccines.
    addBool(QStringLiteral("vaccineBcg"), m_history, &PediatricHistory::vaccineBcg);
    addBool(QStringLiteral("vaccineOpv0"), m_history, &PediatricHistory::vaccineOpv0);
    addBool(QStringLiteral("vaccineOpv1"), m_history, &PediatricHistory::vaccineOpv1);
    addBool(QStringLiteral("vaccineOpv2"), m_history, &PediatricHistory::vaccineOpv2);
    addBool(QStringLiteral("vaccineOpv3"), m_history, &PediatricHistory::vaccineOpv3);
    addBool(QStringLiteral("vaccineDptHib1"), m_history, &PediatricHistory::vaccineDptHib1);
    addBool(QStringLiteral("vaccineDptHib2"), m_history, &PediatricHistory::vaccineDptHib2);
    addBool(QStringLiteral("vaccineDptHib3"), m_history, &PediatricHistory::vaccineDptHib3);
    addBool(QStringLiteral("vaccineHepB1"), m_history, &PediatricHistory::vaccineHepB1);
    addBool(QStringLiteral("vaccineHepB2"), m_history, &PediatricHistory::vaccineHepB2);
    addBool(QStringLiteral("vaccineHepB3"), m_history, &PediatricHistory::vaccineHepB3);
    addBool(QStringLiteral("vaccineMeasles9"), m_history, &PediatricHistory::vaccineMeasles9);
    addBool(QStringLiteral("vaccineMeasles10"), m_history, &PediatricHistory::vaccineMeasles10);
}

} // namespace gambasse
