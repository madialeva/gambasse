#pragma once

#include <array>

#include <data/model/Consultation.h>

namespace gambasse {

// Pediatric consultation domain model (table b08_pediatric_consultation).
// It holds exactly the columns managed by the pediatric consultation window;
// the other columns of the table (legacy supplements, lesions...) keep their
// value. Combo values are the VB.NET domain codes (0 = the empty option "-"),
// see PediatricConsultationLabels. Weight keeps the VB.NET scaling in the
// table (x100) and temperature too (x10).
struct PediatricConsultation : Consultation {
    int nursingDiagnosis = 0;
    QString otherDiagnosis;

    // Clinical signs.
    double weight = 0.0;
    int length = 0;
    int weightRatio = 0;
    int heightRatio = 0;
    int consciousness = 0;
    double temperature = 0.0;
    int bloodGlucose = 0;
    int mentalStatus = 0;
    int armColor = 0;
    int appetiteTest = 0;
    int malariaTest = 0;
    bool convulsion = false;
    bool bradypnea = false;
    bool drinkingEating = false;

    // Respiratory.
    int retractionType = 0;
    int retractionSeverity = 0;
    int tachypnea = 0;
    bool burning = false;
    bool grunting = false;
    bool runnyNose = false;
    int wheeze = 0;
    int secretions = 0;
    int stridor = 0;
    bool congestion = false;
    bool tearing = false;
    bool soreThroat = false;
    bool cough = false;
    int coughDays = 0;
    int oxygenSaturation = 0;

    // Elimination.
    int stool = 0;
    QString bleeding;
    int urine = 0;
    int vomiting = 0;

    // Circulatory.
    int capillaryRefill = 0;
    bool absentPulse = false;
    int cyanosis = 0;
    QString edema;
    int heartRate = 0;
    QString arrhythmia;

    // Skin.
    bool skinFolds = false;
    bool palePalms = false;
    bool drySkin = false;
    QString bodyLesions;
    bool petechiae = false;
    bool pustules = false;
    bool jaundice = false;
    int exanthema = 0;
    int infection = 0;
    int skinEdema = 0;

    // Eye.
    int eyeDischarge = 0;
    bool eyelidEdema = false;
    bool stuckEyelids = false;
    bool redConjunctiva = false;
    bool paleConjunctiva = false;
    bool sunkenEyes = false;

    // Ear.
    bool swellingBehindEar = false;
    int earPain = 0;
    int earDischarge = 0;
    bool itchyEarCanal = false;
    int otoscopy = 0;

    // Mouth.
    int tonsils = 0;
    int mouthPain = 0;
    QString lesionType;
    bool koplikSpots = false;
    bool whitePatches = false;
    bool redTongue = false;
    bool palatalPetechiae = false;
    bool cervicalNodes = false;

    // Abdomen / pelvis.
    int abdominalPainSigns = 0;

    // Neurological.
    bool severeHeadache = false;
    bool photophobia = false;
    bool brudzinskiSign = false;
    bool kernigSign = false;
    bool neckStiffness = false;
    bool bulgingFontanelle = false;
    bool developmentalProblems = false;
    QString developmentalProblemsDescription;

    // Endocrine.
    bool excessiveHunger = false;
    bool excessiveThirst = false;
    bool asthenia = false;
    QString lymphadenopathy;
    QString rapidWeightLoss;
    QString breathOdor;

    // Indicated treatments (rows 1-10).
    struct Treatment {
        int medication = 0;
        QString description;
        int route = 0;
        int days = 0;
        QString dose;
        QString frequency;
        int perfusion = 0;
    };
    std::array<Treatment, 10> treatments{};

    // Child care recommendations.
    std::array<int, 5> recommendations{};
};

} // namespace gambasse
