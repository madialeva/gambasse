#pragma once

#include <QDate>
#include <QString>

#include <data/model/PediatricHistory.h>

namespace gambasse {

// Adult history domain model (table b03_adult_history).
// It holds exactly the columns managed by the adult history window.
// Decimal vital signs keep the VB.NET scaling used by the ORM: weight and
// temperature x10, BMI x100.
struct AdultHistory {

    qlonglong patientId = 0;
    QString   allergies;
    QDate     openingDate = QDate(1900, 1, 1);

    // Adult history.
    bool    historyDiabetes = false;
    QString historyDiabetesTreatment;
    bool    historyHepatitis = false;
    QString historyHepatitisType;
    QString historyHepatitisTreatment;
    bool    historyHiv = false;
    QString historyHivTreatment;
    QString historyOtherDiseases;
    QString historyDisabilities;
    QString historyConflicts;
    QString historyOccupation;
    int     historyChildCount = 0;

    // Housing.
    bool                                housingPotableWater = false;
    bool                                housingLatrine = false;
    bool                                housingDomesticHygiene = false;
    PediatricHistory::DomesticAnimals   housingDomesticAnimals =
        PediatricHistory::DomesticAnimals::None;
    bool housingMosquitoNet = false;
    bool housingMosquitoNetUse = false;
    bool housingSanitaryControl = false;

    // Vital signs.
    double  physicalExamWeight = 0.0;
    int     physicalExamHeight = 0;
    double  physicalExamTemperature = 0.0;
    int     physicalExamAbdominalPerimeter = 0;
    double  physicalExamBmi = 0.0;
    QString physicalExamConsciousness;
    QString physicalExamCapillaryGlucose;
    QString physicalExamHeartRate;
    QString physicalExamRespiratoryRate;
};

} // namespace gambasse
