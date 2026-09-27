#include <ui/controller/PregnancyHistoryController.h>


namespace gambasse {

PregnancyHistoryController::PregnancyHistoryController(QObject* parent) : HistoryController(parent) {
    buildFields();
}

const QList<HistoryController::Field>& PregnancyHistoryController::fields() const {
    return m_fields;
}

bool PregnancyHistoryController::serviceLoad() {
    return m_service.load(patientId(), m_history, m_isNewFlag());
}

int PregnancyHistoryController::serviceSave() {
    m_history.patientId = patientId();
    return static_cast<int>(m_service.save(isNew(), m_history));
}

bool PregnancyHistoryController::serviceRemove() {
    return m_service.remove(patientId());
}

void PregnancyHistoryController::buildFields() {
    // Personal data.
    addDate(QStringLiteral("openingDate"), m_history, &PregnancyHistory::openingDate);
    addText(QStringLiteral("allergies"), m_history, &PregnancyHistory::allergies, 100);

    // Pregnancy history.
    addDate(QStringLiteral("obstetricLastMenstruation"), m_history,
            &PregnancyHistory::obstetricLastMenstruation);
    addInt(QStringLiteral("obstetricDeceasedSiblingCount"), m_history,
           &PregnancyHistory::obstetricDeceasedSiblingCount, 0, 99);
    addDate(QStringLiteral("obstetricExpectedDelivery"), m_history,
            &PregnancyHistory::obstetricExpectedDelivery);
    addInt(QStringLiteral("obstetricDeliveryCount"), m_history,
           &PregnancyHistory::obstetricDeliveryCount, 0, 99);
    addInt(QStringLiteral("obstetricAbortionCount"), m_history,
           &PregnancyHistory::obstetricAbortionCount, 0, 999);
    addText(QStringLiteral("obstetricDeliveryProblems"), m_history,
            &PregnancyHistory::obstetricDeliveryProblems, 200);
    addText(QStringLiteral("obstetricGynecologicalDiseases"), m_history,
            &PregnancyHistory::obstetricGynecologicalDiseases, 200);
    addText(QStringLiteral("obstetricBarrierMethods"), m_history,
            &PregnancyHistory::obstetricBarrierMethods, 200);
    addBool(QStringLiteral("obstetricHivWoman"), m_history, &PregnancyHistory::obstetricHivWoman);
    addBool(QStringLiteral("obstetricHivMan"), m_history, &PregnancyHistory::obstetricHivMan);
    addText(QStringLiteral("obstetricSerologies"), m_history, &PregnancyHistory::obstetricSerologies,
            100);

    // Medical history.
    addBool(QStringLiteral("medicalDiabetes"), m_history, &PregnancyHistory::medicalDiabetes);
    addText(QStringLiteral("medicalDiabetesTreatment"), m_history,
            &PregnancyHistory::medicalDiabetesTreatment, 100);
    addBool(QStringLiteral("medicalHepatitis"), m_history, &PregnancyHistory::medicalHepatitis);
    addText(QStringLiteral("medicalHepatitisType"), m_history,
            &PregnancyHistory::medicalHepatitisType, 3);
    addText(QStringLiteral("medicalHepatitisTreatment"), m_history,
            &PregnancyHistory::medicalHepatitisTreatment, 100);
    addBool(QStringLiteral("medicalHypertension"), m_history, &PregnancyHistory::medicalHypertension);
    addText(QStringLiteral("medicalHypertensionTreatment"), m_history,
            &PregnancyHistory::medicalHypertensionTreatment, 100);
    addText(QStringLiteral("medicalOtherDiseases"), m_history,
            &PregnancyHistory::medicalOtherDiseases, 200);
    addText(QStringLiteral("medicalDisabilities"), m_history, &PregnancyHistory::medicalDisabilities,
            200);
    addText(QStringLiteral("medicalConflicts"), m_history, &PregnancyHistory::medicalConflicts, 200);

    // Housing.
    addBool(QStringLiteral("housingPotableWater"), m_history, &PregnancyHistory::housingPotableWater);
    addBool(QStringLiteral("housingLatrine"), m_history, &PregnancyHistory::housingLatrine);
    addBool(QStringLiteral("housingMosquitoNetParents"), m_history,
            &PregnancyHistory::housingMosquitoNetParents);
    addBool(QStringLiteral("housingMosquitoNetUseParents"), m_history,
            &PregnancyHistory::housingMosquitoNetUseParents);
    addBool(QStringLiteral("housingTreatedWater"), m_history, &PregnancyHistory::housingTreatedWater);
    addBool(QStringLiteral("housingDomesticHygiene"), m_history,
            &PregnancyHistory::housingDomesticHygiene);
    addBool(QStringLiteral("housingMosquitoNetChildren"), m_history,
            &PregnancyHistory::housingMosquitoNetChildren);
    addBool(QStringLiteral("housingMosquitoNetUseChildren"), m_history,
            &PregnancyHistory::housingMosquitoNetUseChildren);
    addEnum(QStringLiteral("housingDomesticAnimals"), m_history,
            &PregnancyHistory::housingDomesticAnimals, 3);

    // Physical examination.
    addDecimal(QStringLiteral("physicalExamWeight"), m_history, &PregnancyHistory::physicalExamWeight,
               1, 999.9);
    addDecimal(QStringLiteral("physicalExamTemperature"), m_history,
               &PregnancyHistory::physicalExamTemperature, 1, 99.9);
    addInt(QStringLiteral("physicalExamHeight"), m_history, &PregnancyHistory::physicalExamHeight, 0,
           999);
    addInt(QStringLiteral("physicalExamAbdominalPerimeter"), m_history,
           &PregnancyHistory::physicalExamAbdominalPerimeter, 0, 999);
    addText(QStringLiteral("physicalExamVomiting"), m_history, &PregnancyHistory::physicalExamVomiting,
            100);
    addText(QStringLiteral("physicalExamFetalAuscultation"), m_history,
            &PregnancyHistory::physicalExamFetalAuscultation, 100);
    addText(QStringLiteral("physicalExamFetalPosition"), m_history,
            &PregnancyHistory::physicalExamFetalPosition, 100);
    addInt(QStringLiteral("physicalExamUterineHeightWeeks"), m_history,
           &PregnancyHistory::physicalExamUterineHeightWeeks, 0, 99);
    addInt(QStringLiteral("physicalExamUterineHeightCm"), m_history,
           &PregnancyHistory::physicalExamUterineHeightCm, 0, 99);
    addBool(QStringLiteral("physicalExamLeukocytosis"), m_history,
            &PregnancyHistory::physicalExamLeukocytosis);
    addBool(QStringLiteral("physicalExamProteinuria"), m_history,
            &PregnancyHistory::physicalExamProteinuria);
    addBool(QStringLiteral("physicalExamMmi"), m_history, &PregnancyHistory::physicalExamMmi);
    addBool(QStringLiteral("physicalExamFace"), m_history, &PregnancyHistory::physicalExamFace);
    addBool(QStringLiteral("physicalExamGeneral"), m_history, &PregnancyHistory::physicalExamGeneral);

    // Previous treatments: five rows of date, medication and dose.
    QDate* const dates[] = {&m_history.supplementDate1, &m_history.supplementDate2,
                            &m_history.supplementDate3, &m_history.supplementDate4,
                            &m_history.supplementDate5};
    QString* const medications[] = {&m_history.supplementMedication1,
                                    &m_history.supplementMedication2,
                                    &m_history.supplementMedication3,
                                    &m_history.supplementMedication4,
                                    &m_history.supplementMedication5};
    QString* const doses[] = {&m_history.supplementDose1, &m_history.supplementDose2,
                              &m_history.supplementDose3, &m_history.supplementDose4,
                              &m_history.supplementDose5};
    for (int row = 0; row < 5; ++row) {
        addDate(QStringLiteral("supplementDate%1").arg(row + 1), dates[row]);
        addText(QStringLiteral("supplementMedication%1").arg(row + 1), medications[row], 100);
        addText(QStringLiteral("supplementDose%1").arg(row + 1), doses[row], 100);
    }

    // Current treatments: dose, medication and next dose of each row.
    const struct {
        const char* name;
        QString PregnancyHistory::*dose;
        QString PregnancyHistory::*medication;
        QString PregnancyHistory::*nextDose;
    } currentRows[] = {
        {"Calcium", &PregnancyHistory::supplementCalciumD, &PregnancyHistory::supplementCalciumM,
         &PregnancyHistory::supplementCalciumPd},
        {"VitaminC", &PregnancyHistory::supplementVitaminCD, &PregnancyHistory::supplementVitaminCM,
         &PregnancyHistory::supplementVitaminCPd},
        {"OtherVitamins", &PregnancyHistory::supplementOtherVitaminsD,
         &PregnancyHistory::supplementOtherVitaminsM, &PregnancyHistory::supplementOtherVitaminsPd},
        {"MalariaProphylaxis", &PregnancyHistory::supplementMalariaProphylaxisD,
         &PregnancyHistory::supplementMalariaProphylaxisM,
         &PregnancyHistory::supplementMalariaProphylaxisPd},
    };
    for (const auto& row : currentRows) {
        addText(QStringLiteral("supplement%1D").arg(QLatin1String(row.name)), m_history, row.dose, 50);
        addText(QStringLiteral("supplement%1M").arg(QLatin1String(row.name)), m_history,
                row.medication, 50);
        addText(QStringLiteral("supplement%1Pd").arg(QLatin1String(row.name)), m_history,
                row.nextDose, 50);
    }
}

} // namespace gambasse
