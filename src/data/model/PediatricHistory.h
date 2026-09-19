#pragma once

#include <QDate>
#include <QString>

namespace gambasse {

// Pediatric history domain model (table b05_pediatric_history).
// It holds exactly the columns managed by the pediatric history window.
// Columns the form never shows (diabetes, hepatitis, hiv, parental
// occupations, Apgar and the supplement medication/dose texts) keep their
// schema defaults on insert and are left untouched on update.
struct PediatricHistory {

    // Domestic animals (b05 domestic_animals).
    enum class DomesticAnimals { None = 0, Stable = 1, Dwelling = 2, StableAndDwelling = 3 };

    // Treatment types (b05 supplement_treatment1..4).
    enum class TreatmentType {
        None = 0,
        VitaminA = 1,
        Albendazole = 2,
        Iron = 3,
        VitaminC = 4,
        Micronutrients = 5,
        Zinc = 6,
        AscorbicAcid = 7
    };

    qlonglong patientId = 0;
    QString   allergies;
    QDate     openingDate = QDate(1900, 1, 1);

    // Birth history.
    int     neonatalWeight = 0;
    int     neonatalHeight = 0;
    QString neonatalDeliveryIncidents;
    QString neonatalMalformations;

    // Child history.
    QString clinicalDiseaseType;
    QString clinicalDiseaseTreatment;
    QString otherDiseases;
    QString conflicts;

    // Feeding.
    bool    feedingBreastfeeding = false;
    QString feedingAdditionalFeeding;
    QString feedingFeedingProblems;
    bool    feedingTreatedWater = false;
    QString feedingTreatedWaterDetails;

    // Vaccines.
    bool vaccineBcg = false;
    bool vaccineOpv0 = false;
    bool vaccineOpv1 = false;
    bool vaccineOpv2 = false;
    bool vaccineOpv3 = false;
    bool vaccineDptHib1 = false;
    bool vaccineDptHib2 = false;
    bool vaccineDptHib3 = false;
    bool vaccineHepB1 = false;
    bool vaccineHepB2 = false;
    bool vaccineHepB3 = false;
    bool vaccineMeasles9 = false;
    bool vaccineMeasles10 = false;

    // Housing.
    bool            latrine = false;
    bool            domesticHygiene = false;
    bool            mosquitoNet = false;
    bool            netUse = false;
    DomesticAnimals domesticAnimals = DomesticAnimals::None;
    QString         economicSituation;

    // Previous treatments (dates are null when invalid).
    QDate         supplementDate1;
    QDate         supplementDate2;
    QDate         supplementDate3;
    QDate         supplementDate4;
    TreatmentType supplementTreatment1 = TreatmentType::None;
    TreatmentType supplementTreatment2 = TreatmentType::None;
    TreatmentType supplementTreatment3 = TreatmentType::None;
    TreatmentType supplementTreatment4 = TreatmentType::None;
};

} // namespace gambasse
