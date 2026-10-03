#pragma once

#include <data/model/Consultation.h>

namespace gambasse {

// Pregnancy consultation domain model (table b07_pregnancy_consultation).
// It holds exactly the columns managed by the pregnancy consultation window.
// Weight and temperature keep the VB.NET scaling in the table (x10). The fixed
// rows of the window (iron, folic acid, deworming) carry no column.
struct PregnancyConsultation : Consultation {
    // Physical examination.
    double weight = 0.0;
    double temperature = 0.0;
    int height = 0;
    int abdominalPerimeter = 0;
    QString vomiting;
    QString fetalPosition;
    QString fetalAuscultation;
    int uterineHeightWeeks = 0;
    int uterineHeightCm = 0;
    bool leukocytosis = false;
    bool proteinuria = false;
    bool edemaLegs = false;
    bool edemaFace = false;
    bool edemaGeneral = false;

    // Treatments given now: dose, medication and next dose.
    struct Supplement {
        QString dose;
        QString medication;
        QString nextDose;
    };
    Supplement calcium;
    Supplement vitaminC;
    Supplement otherVitamins;
    Supplement malariaProphylaxis;
};

} // namespace gambasse
