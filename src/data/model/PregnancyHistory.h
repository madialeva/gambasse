#pragma once

#include <QDate>
#include <QString>

#include <data/model/PediatricHistory.h>

namespace gambasse {

// Pregnancy history domain model (table b04_pregnancy_history).
// It holds exactly the columns managed by the pregnancy history window.
// Weight and temperature keep the VB.NET x10 scaling used by the ORM.
// Nullable dates (expected delivery, last menstruation and the 5 previous
// treatment dates) use an invalid QDate for NULL.
struct PregnancyHistory {

    qlonglong patientId = 0;
    QString   allergies;
    QDate     openingDate = QDate(1900, 1, 1);

    // Pregnancy history (dates are null when invalid).
    QDate   obstetricLastMenstruation;
    QDate   obstetricExpectedDelivery;
    int     obstetricDeceasedSiblingCount = 0;
    int     obstetricDeliveryCount = 0;
    int     obstetricAbortionCount = 0;
    QString obstetricDeliveryProblems;
    QString obstetricGynecologicalDiseases;
    QString obstetricBarrierMethods;
    bool    obstetricHivWoman = false;
    bool    obstetricHivMan = false;
    QString obstetricSerologies;

    // Medical history.
    bool    medicalDiabetes = false;
    QString medicalDiabetesTreatment;
    bool    medicalHepatitis = false;
    QString medicalHepatitisType;
    QString medicalHepatitisTreatment;
    bool    medicalHypertension = false;
    QString medicalHypertensionTreatment;
    QString medicalOtherDiseases;
    QString medicalDisabilities;
    QString medicalConflicts;

    // Housing.
    bool                              housingPotableWater = false;
    bool                              housingTreatedWater = false;
    bool                              housingLatrine = false;
    bool                              housingDomesticHygiene = false;
    PediatricHistory::DomesticAnimals housingDomesticAnimals =
        PediatricHistory::DomesticAnimals::None;
    bool housingMosquitoNetParents = false;
    bool housingMosquitoNetUseParents = false;
    bool housingMosquitoNetChildren = false;
    bool housingMosquitoNetUseChildren = false;

    // Physical examination.
    double  physicalExamWeight = 0.0;
    int     physicalExamHeight = 0;
    double  physicalExamTemperature = 0.0;
    int     physicalExamAbdominalPerimeter = 0;
    QString physicalExamVomiting;
    QString physicalExamFetalPosition;
    QString physicalExamFetalAuscultation;
    int     physicalExamUterineHeightWeeks = 0;
    int     physicalExamUterineHeightCm = 0;
    bool    physicalExamLeukocytosis = false;
    bool    physicalExamProteinuria = false;
    bool    physicalExamMmi = false;
    bool    physicalExamFace = false;
    bool    physicalExamGeneral = false;

    // Previous treatments (dates are null when invalid).
    QDate   supplementDate1;
    QDate   supplementDate2;
    QDate   supplementDate3;
    QDate   supplementDate4;
    QDate   supplementDate5;
    QString supplementMedication1;
    QString supplementMedication2;
    QString supplementMedication3;
    QString supplementMedication4;
    QString supplementMedication5;
    QString supplementDose1;
    QString supplementDose2;
    QString supplementDose3;
    QString supplementDose4;
    QString supplementDose5;

    // Current treatments (dose / medication / next dose rows).
    QString supplementCalciumD;
    QString supplementCalciumM;
    QString supplementCalciumPd;
    QString supplementVitaminCD;
    QString supplementVitaminCM;
    QString supplementVitaminCPd;
    QString supplementOtherVitaminsD;
    QString supplementOtherVitaminsM;
    QString supplementOtherVitaminsPd;
    QString supplementMalariaProphylaxisD;
    QString supplementMalariaProphylaxisM;
    QString supplementMalariaProphylaxisPd;
};

} // namespace gambasse
