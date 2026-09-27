#include <ui/controller/AdultHistoryController.h>

namespace gambasse {

AdultHistoryController::AdultHistoryController(QObject* parent) : HistoryController(parent) {
    buildFields();
}

const QList<HistoryController::Field>& AdultHistoryController::fields() const {
    return m_fields;
}

bool AdultHistoryController::serviceLoad() {
    // isNew() is the base flag: the service sets it through the reference.
    return m_service.load(patientId(), m_history, m_isNewFlag());
}

int AdultHistoryController::serviceSave() {
    m_history.patientId = patientId();
    return static_cast<int>(m_service.save(isNew(), m_history));
}

bool AdultHistoryController::serviceRemove() {
    return m_service.remove(patientId());
}

void AdultHistoryController::buildFields() {
    // Personal data.
    addDate(QStringLiteral("openingDate"), m_history, &AdultHistory::openingDate);
    addText(QStringLiteral("allergies"), m_history, &AdultHistory::allergies, 100);

    // Adult history.
    addBool(QStringLiteral("historyDiabetes"), m_history, &AdultHistory::historyDiabetes);
    addText(QStringLiteral("historyDiabetesTreatment"), m_history,
            &AdultHistory::historyDiabetesTreatment, 100);
    addBool(QStringLiteral("historyHepatitis"), m_history, &AdultHistory::historyHepatitis);
    addText(QStringLiteral("historyHepatitisType"), m_history, &AdultHistory::historyHepatitisType,
            3);
    addText(QStringLiteral("historyHepatitisTreatment"), m_history,
            &AdultHistory::historyHepatitisTreatment, 100);
    addBool(QStringLiteral("historyHiv"), m_history, &AdultHistory::historyHiv);
    addText(QStringLiteral("historyHivTreatment"), m_history, &AdultHistory::historyHivTreatment,
            100);
    addText(QStringLiteral("historyOtherDiseases"), m_history, &AdultHistory::historyOtherDiseases,
            200);
    addText(QStringLiteral("historyOccupation"), m_history, &AdultHistory::historyOccupation, 100);
    addInt(QStringLiteral("historyChildCount"), m_history, &AdultHistory::historyChildCount, 0, 99);
    addText(QStringLiteral("historyDisabilities"), m_history, &AdultHistory::historyDisabilities,
            200);
    addText(QStringLiteral("historyConflicts"), m_history, &AdultHistory::historyConflicts, 200);

    // Housing.
    addBool(QStringLiteral("housingPotableWater"), m_history, &AdultHistory::housingPotableWater);
    addBool(QStringLiteral("housingMosquitoNet"), m_history, &AdultHistory::housingMosquitoNet);
    addBool(QStringLiteral("housingMosquitoNetUse"), m_history,
            &AdultHistory::housingMosquitoNetUse);
    addBool(QStringLiteral("housingLatrine"), m_history, &AdultHistory::housingLatrine);
    addEnum(QStringLiteral("housingDomesticAnimals"), m_history,
            &AdultHistory::housingDomesticAnimals, 3);
    addBool(QStringLiteral("housingDomesticHygiene"), m_history,
            &AdultHistory::housingDomesticHygiene);
    addBool(QStringLiteral("housingSanitaryControl"), m_history,
            &AdultHistory::housingSanitaryControl);

    // Vital signs.
    addDecimal(QStringLiteral("physicalExamWeight"), m_history, &AdultHistory::physicalExamWeight, 1,
               999.9);
    addDecimal(QStringLiteral("physicalExamTemperature"), m_history,
               &AdultHistory::physicalExamTemperature, 1, 99.9);
    addInt(QStringLiteral("physicalExamHeight"), m_history, &AdultHistory::physicalExamHeight, 0, 999);
    addDecimal(QStringLiteral("physicalExamBmi"), m_history, &AdultHistory::physicalExamBmi, 2, 99.99);
    addInt(QStringLiteral("physicalExamAbdominalPerimeter"), m_history,
           &AdultHistory::physicalExamAbdominalPerimeter, 0, 999);
    addText(QStringLiteral("physicalExamConsciousness"), m_history,
            &AdultHistory::physicalExamConsciousness, 100);
    addText(QStringLiteral("physicalExamHeartRate"), m_history, &AdultHistory::physicalExamHeartRate,
            100);
    addText(QStringLiteral("physicalExamRespiratoryRate"), m_history,
            &AdultHistory::physicalExamRespiratoryRate, 100);
    addText(QStringLiteral("physicalExamCapillaryGlucose"), m_history,
            &AdultHistory::physicalExamCapillaryGlucose, 100);
}

} // namespace gambasse
