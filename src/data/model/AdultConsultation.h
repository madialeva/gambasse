#pragma once

#include <array>

#include <data/model/Consultation.h>

namespace gambasse {

// Adult consultation domain model (table b06_adult_consultation).
// It holds exactly the columns managed by the adult consultation window.
struct AdultConsultation : Consultation {
    // Lesions: body region number (see the body diagram) and impairment.
    std::array<int, 8> lesionRegion{};
    std::array<QString, 8> lesionImpairment;

    // Physical examination: respiratory.
    bool nasalFlaring = false;
    bool stridor = false;
    bool wheezing = false;
    bool retraction = false;
    // Circulatory.
    bool arrhythmia = false;
    bool capillaryRefill = false;
    bool absentPulse = false;
    bool edema = false;
    // Skin and mucosa.
    bool skinFolds = false;
    bool paleMucosa = false;
    bool sunkenEyes = false;
    bool palePalms = false;
    // Endocrine-metabolic.
    bool organomegaly = false;
    QString organomegalyDescription;
    bool lymphadenopathy = false;
    QString lymphadenopathyDescription;
    // Elimination.
    bool secretion = false;
    bool stoolChanges = false;
    bool cough = false;
    QString coughDescription;
    bool urineChanges = false;
    QString urineChangesDescription;
    bool vomiting = false;
    // Health disorders.
    QString dehydrationDegree;
    QString malnutritionDegree;
    QString other;
    // Conduct.
    bool irritable = false;
    QString otherCode;
};

} // namespace gambasse
