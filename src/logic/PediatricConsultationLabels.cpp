#include <logic/PediatricConsultationLabels.h>

#include <QCoreApplication>
#include <QtGlobal>
#include <iterator>

namespace gambasse {

namespace {

// One option of a domain: the persisted VB.NET code and the English source
// text, translated at run time in the context of its domain.
struct Entry {
    int code;
    const char* source;
};

struct Domain {
    const char* key;
    const char* context;
    const Entry* entries;
    int count;
};

// Codes and meanings from IConsultaPediatrica.vb / ConsultaPediatrica.vb
// (LISTA_TIPOS_*). Code 0 is always the empty option "-".

const Entry kNursingDiagnosis[] = {
    {0, QT_TRANSLATE_NOOP("NursingDiagnosis", "-")},
    {1, QT_TRANSLATE_NOOP("NursingDiagnosis", "SEVERE DEHYDRATION")},
    {2, QT_TRANSLATE_NOOP("NursingDiagnosis", "MODERATE DEHYDRATION")},
    {3, QT_TRANSLATE_NOOP("NursingDiagnosis", "MILD DEHYDRATION")},
    {4, QT_TRANSLATE_NOOP("NursingDiagnosis", "SEVERE MALNUTRITION")},
    {5, QT_TRANSLATE_NOOP("NursingDiagnosis", "MODERATE MALNUTRITION")},
    {6, QT_TRANSLATE_NOOP("NursingDiagnosis", "MILD MALNUTRITION")},
    {7, QT_TRANSLATE_NOOP("NursingDiagnosis", "CHRONIC MALNUTRITION")},
};

const Entry kConsciousness[] = {
    {0, QT_TRANSLATE_NOOP("Consciousness", "-")},
    {1, QT_TRANSLATE_NOOP("Consciousness", "A")},
    {2, QT_TRANSLATE_NOOP("Consciousness", "V")},
    {3, QT_TRANSLATE_NOOP("Consciousness", "P")},
    {4, QT_TRANSLATE_NOOP("Consciousness", "U")},
};

const Entry kMentalStatus[] = {
    {0, QT_TRANSLATE_NOOP("MentalStatus", "-")},
    {1, QT_TRANSLATE_NOOP("MentalStatus", "LETHARGIC")},
    {2, QT_TRANSLATE_NOOP("MentalStatus", "CONFUSED")},
    {3, QT_TRANSLATE_NOOP("MentalStatus", "DROWSY")},
    {4, QT_TRANSLATE_NOOP("MentalStatus", "STUPOROUS")},
    {5, QT_TRANSLATE_NOOP("MentalStatus", "IRRITABLE")},
    {6, QT_TRANSLATE_NOOP("MentalStatus", "RESTLESS")},
    {7, QT_TRANSLATE_NOOP("MentalStatus", "AGITATED")},
    {8, QT_TRANSLATE_NOOP("MentalStatus", "LOW ACTIVITY")},
};

const Entry kArmColor[] = {
    {0, QT_TRANSLATE_NOOP("ArmColor", "-")},
    {1, QT_TRANSLATE_NOOP("ArmColor", "RED")},
    {2, QT_TRANSLATE_NOOP("ArmColor", "ORANGE")},
    {3, QT_TRANSLATE_NOOP("ArmColor", "YELLOW")},
    {4, QT_TRANSLATE_NOOP("ArmColor", "GREEN")},
};

const Entry kTestResult[] = {
    {0, QT_TRANSLATE_NOOP("TestResult", "-")},
    {1, QT_TRANSLATE_NOOP("TestResult", "POSITIVE")},
    {2, QT_TRANSLATE_NOOP("TestResult", "NEGATIVE")},
};

const Entry kZScore[] = {
    {0, QT_TRANSLATE_NOOP("ZScore", "-")},
    {1, QT_TRANSLATE_NOOP("ZScore", "< -3")},
    {2, QT_TRANSLATE_NOOP("ZScore", "= -3")},
    {3, QT_TRANSLATE_NOOP("ZScore", "< -2")},
    {4, QT_TRANSLATE_NOOP("ZScore", "= -2")},
    {5, QT_TRANSLATE_NOOP("ZScore", "< -1")},
    {6, QT_TRANSLATE_NOOP("ZScore", "= -1")},
    {7, QT_TRANSLATE_NOOP("ZScore", "> -1")},
    {8, QT_TRANSLATE_NOOP("ZScore", "< +1")},
    {9, QT_TRANSLATE_NOOP("ZScore", ">= +1")},
};

const Entry kRetractionType[] = {
    {0, QT_TRANSLATE_NOOP("RetractionType", "-")},
    {1, QT_TRANSLATE_NOOP("RetractionType", "SUBCOSTAL")},
    {2, QT_TRANSLATE_NOOP("RetractionType", "INTERCOSTAL")},
    {3, QT_TRANSLATE_NOOP("RetractionType", "SUPRACLAVICULAR")},
};

const Entry kSeverity[] = {
    {0, QT_TRANSLATE_NOOP("Severity", "-")},
    {1, QT_TRANSLATE_NOOP("Severity", "SEVERE")},
    {2, QT_TRANSLATE_NOOP("Severity", "MODERATE")},
    {3, QT_TRANSLATE_NOOP("Severity", "MILD")},
};

const Entry kSecretions[] = {
    {0, QT_TRANSLATE_NOOP("Secretions", "-")},
    {1, QT_TRANSLATE_NOOP("Secretions", "GREEN")},
    {2, QT_TRANSLATE_NOOP("Secretions", "YELLOW")},
    {3, QT_TRANSLATE_NOOP("Secretions", "WHITE")},
};

const Entry kStool[] = {
    {0, QT_TRANSLATE_NOOP("Stool", "-")},
    {1, QT_TRANSLATE_NOOP("Stool", "WATERY DIARRHEA")},
    {2, QT_TRANSLATE_NOOP("Stool", "PERSISTENT DIARRHEA")},
    {3, QT_TRANSLATE_NOOP("Stool", "BLOODY DIARRHEA")},
    {4, QT_TRANSLATE_NOOP("Stool", "CONSTIPATION")},
};

const Entry kVomiting[] = {
    {0, QT_TRANSLATE_NOOP("Vomiting", "-")},
    {1, QT_TRANSLATE_NOOP("Vomiting", "PERSISTENT")},
    {2, QT_TRANSLATE_NOOP("Vomiting", "OCCASIONAL")},
    {3, QT_TRANSLATE_NOOP("Vomiting", "WITH BLOOD")},
    {4, QT_TRANSLATE_NOOP("Vomiting", "FOOD")},
    {5, QT_TRANSLATE_NOOP("Vomiting", "OTHER")},
};

const Entry kUrine[] = {
    {0, QT_TRANSLATE_NOOP("Urine", "-")},
    {1, QT_TRANSLATE_NOOP("Urine", "NOCTURIA")},
    {2, QT_TRANSLATE_NOOP("Urine", "HEMATURIA")},
    {3, QT_TRANSLATE_NOOP("Urine", "DYSURIA")},
    {4, QT_TRANSLATE_NOOP("Urine", "PYURIA")},
    {5, QT_TRANSLATE_NOOP("Urine", "GLYCOSURIA")},
    {6, QT_TRANSLATE_NOOP("Urine", "KETONURIA")},
};

const Entry kCapillaryRefill[] = {
    {0, QT_TRANSLATE_NOOP("CapillaryRefill", "-")},
    {1, QT_TRANSLATE_NOOP("CapillaryRefill", "> 2''")},
    {2, QT_TRANSLATE_NOOP("CapillaryRefill", "< 2''")},
};

const Entry kCyanosis[] = {
    {0, QT_TRANSLATE_NOOP("Cyanosis", "-")},
    {1, QT_TRANSLATE_NOOP("Cyanosis", "CENTRAL")},
    {2, QT_TRANSLATE_NOOP("Cyanosis", "PERIPHERAL")},
};

const Entry kExanthema[] = {
    {0, QT_TRANSLATE_NOOP("Exanthema", "-")},
    {1, QT_TRANSLATE_NOOP("Exanthema", "ITCHY")},
    {2, QT_TRANSLATE_NOOP("Exanthema", "FADES WITH PRESSURE")},
    {3, QT_TRANSLATE_NOOP("Exanthema", "DOES NOT FADE WITH PRESSURE")},
};

const Entry kSkinInfection[] = {
    {0, QT_TRANSLATE_NOOP("SkinInfection", "-")},
    {1, QT_TRANSLATE_NOOP("SkinInfection", "LOCAL")},
    {2, QT_TRANSLATE_NOOP("SkinInfection", "EXTENSIVE")},
};

const Entry kSkinEdema[] = {
    {0, QT_TRANSLATE_NOOP("SkinEdema", "-")},
    {1, QT_TRANSLATE_NOOP("SkinEdema", "LOCAL")},
    {2, QT_TRANSLATE_NOOP("SkinEdema", "EXTENSIVE")},
};

const Entry kEyeDischarge[] = {
    {0, QT_TRANSLATE_NOOP("EyeDischarge", "-")},
    {1, QT_TRANSLATE_NOOP("EyeDischarge", "SEVERE PURULENT")},
    {2, QT_TRANSLATE_NOOP("EyeDischarge", "MODERATE PURULENT")},
    {3, QT_TRANSLATE_NOOP("EyeDischarge", "WATERY")},
};

const Entry kEarPain[] = {
    {0, QT_TRANSLATE_NOOP("EarPain", "-")},
    {1, QT_TRANSLATE_NOOP("EarPain", "VERY SEVERE")},
    {2, QT_TRANSLATE_NOOP("EarPain", "SEVERE")},
    {3, QT_TRANSLATE_NOOP("EarPain", "MODERATE")},
    {4, QT_TRANSLATE_NOOP("EarPain", "MILD")},
};

const Entry kEarDischarge[] = {
    {0, QT_TRANSLATE_NOOP("EarDischarge", "-")},
    {1, QT_TRANSLATE_NOOP("EarDischarge", "SEROUS")},
    {2, QT_TRANSLATE_NOOP("EarDischarge", "PURULENT +14 DAYS")},
    {3, QT_TRANSLATE_NOOP("EarDischarge", "PURULENT -14 DAYS")},
    {4, QT_TRANSLATE_NOOP("EarDischarge", "MILD")},
};

const Entry kOtoscopy[] = {
    {0, QT_TRANSLATE_NOOP("Otoscopy", "-")},
    {1, QT_TRANSLATE_NOOP("Otoscopy", "RED")},
    {2, QT_TRANSLATE_NOOP("Otoscopy", "YELLOW")},
    {3, QT_TRANSLATE_NOOP("Otoscopy", "OPAQUE")},
    {4, QT_TRANSLATE_NOOP("Otoscopy", "BULGING")},
    {5, QT_TRANSLATE_NOOP("Otoscopy", "EDEMA")},
};

const Entry kTonsils[] = {
    {0, QT_TRANSLATE_NOOP("Tonsils", "-")},
    {1, QT_TRANSLATE_NOOP("Tonsils", "EDEMA")},
    {2, QT_TRANSLATE_NOOP("Tonsils", "WHITE-YELLOW EXUDATE")},
    {3, QT_TRANSLATE_NOOP("Tonsils", "THICK GRAY MEMBRANE")},
    {4, QT_TRANSLATE_NOOP("Tonsils", "VESICLES")},
    {5, QT_TRANSLATE_NOOP("Tonsils", "ULCERS")},
};

const Entry kMouthPain[] = {
    {0, QT_TRANSLATE_NOOP("MouthPain", "-")},
    {1, QT_TRANSLATE_NOOP("MouthPain", "SEVERE")},
    {2, QT_TRANSLATE_NOOP("MouthPain", "MODERATE")},
};

const Entry kAbdominalSigns[] = {
    {0, QT_TRANSLATE_NOOP("AbdominalSigns", "-")},
    {1, QT_TRANSLATE_NOOP("AbdominalSigns", "ACUTE PAIN")},
    {2, QT_TRANSLATE_NOOP("AbdominalSigns", "ABDOMINAL DYSTROPHY")},
    {3, QT_TRANSLATE_NOOP("AbdominalSigns", "ABDOMINAL GUARDING")},
    {4, QT_TRANSLATE_NOOP("AbdominalSigns", "BLUMBERG SIGN")},
    {5, QT_TRANSLATE_NOOP("AbdominalSigns", "RENAL FOSSA PAIN")},
    {6, QT_TRANSLATE_NOOP("AbdominalSigns", "FIST PERCUSSION SIGN")},
};

const Entry kCareRecommendation[] = {
    {0, QT_TRANSLATE_NOOP("CareRecommendation", "-")},
    {1, QT_TRANSLATE_NOOP("CareRecommendation", "Danger signs for the mother to return to a health centre at once")},
    {2, QT_TRANSLATE_NOOP("CareRecommendation", "Advice for the mother on giving oral medicines at home")},
    {3, QT_TRANSLATE_NOOP("CareRecommendation", "Home advice for patients with cough and breathing difficulty")},
    {4, QT_TRANSLATE_NOOP("CareRecommendation", "Advice for treating diarrhea at home")},
    {5, QT_TRANSLATE_NOOP("CareRecommendation", "Malaria prevention measures")},
    {6, QT_TRANSLATE_NOOP("CareRecommendation", "Home advice in case of fever")},
    {7, QT_TRANSLATE_NOOP("CareRecommendation", "Home advice on feeding")},
    {8, QT_TRANSLATE_NOOP("CareRecommendation", "Food hygiene prevention measures")},
    {9, QT_TRANSLATE_NOOP("CareRecommendation", "Household hygiene prevention measures")},
    {10, QT_TRANSLATE_NOOP("CareRecommendation", "Advice for the child with diabetes mellitus")},
    {11, QT_TRANSLATE_NOOP("CareRecommendation", "Early stimulation guide for developmental problems in the child")},
    {12, QT_TRANSLATE_NOOP("CareRecommendation", "Instructions on the child's vaccination with the country's vaccination card")},
    {13, QT_TRANSLATE_NOOP("CareRecommendation", "Plan A: home treatment advice for dehydration")},
};

const Entry kActiveIngredient[] = {
    {0, QT_TRANSLATE_NOOP("ActiveIngredient", "-")},
    {1, QT_TRANSLATE_NOOP("ActiveIngredient", "Acetylcysteine")},
    {2, QT_TRANSLATE_NOOP("ActiveIngredient", "Aciclovir")},
    {3, QT_TRANSLATE_NOOP("ActiveIngredient", "Folic acid")},
    {4, QT_TRANSLATE_NOOP("ActiveIngredient", "Fatty acids")},
    {5, QT_TRANSLATE_NOOP("ActiveIngredient", "Tranexamic acid")},
    {6, QT_TRANSLATE_NOOP("ActiveIngredient", "Adenosine")},
    {7, QT_TRANSLATE_NOOP("ActiveIngredient", "Epinephrine")},
    {8, QT_TRANSLATE_NOOP("ActiveIngredient", "Albendazole")},
    {9, QT_TRANSLATE_NOOP("ActiveIngredient", "Almagate (Almax)")},
    {10, QT_TRANSLATE_NOOP("ActiveIngredient", "Alprazolam")},
    {11, QT_TRANSLATE_NOOP("ActiveIngredient", "Amiodarone")},
    {12, QT_TRANSLATE_NOOP("ActiveIngredient", "Amitriptyline (Tryptizol)")},
    {13, QT_TRANSLATE_NOOP("ActiveIngredient", "Amlodipine")},
    {14, QT_TRANSLATE_NOOP("ActiveIngredient", "Amoxicillin")},
    {15, QT_TRANSLATE_NOOP("ActiveIngredient", "Amoxicillin/clavulanic acid")},
    {16, QT_TRANSLATE_NOOP("ActiveIngredient", "Ampicillin")},
    {17, QT_TRANSLATE_NOOP("ActiveIngredient", "ASA")},
    {18, QT_TRANSLATE_NOOP("ActiveIngredient", "Artemether")},
    {19, QT_TRANSLATE_NOOP("ActiveIngredient", "Artemether + lumefantrine")},
    {20, QT_TRANSLATE_NOOP("ActiveIngredient", "Atropine")},
    {21, QT_TRANSLATE_NOOP("ActiveIngredient", "Azithromycin")},
    {22, QT_TRANSLATE_NOOP("ActiveIngredient", "Beclometasone")},
    {23, QT_TRANSLATE_NOOP("ActiveIngredient", "Benzathine benzylpenicillin")},
    {24, QT_TRANSLATE_NOOP("ActiveIngredient", "Betahistine (Serc)")},
    {25, QT_TRANSLATE_NOOP("ActiveIngredient", "Bisoprolol")},
    {26, QT_TRANSLATE_NOOP("ActiveIngredient", "Bromazepam")},
    {27, QT_TRANSLATE_NOOP("ActiveIngredient", "Budesonide")},
    {28, QT_TRANSLATE_NOOP("ActiveIngredient", "Hyoscine butylbromide (Buscopan)")},
    {29, QT_TRANSLATE_NOOP("ActiveIngredient", "Calcium")},
    {30, QT_TRANSLATE_NOOP("ActiveIngredient", "Captopril")},
    {31, QT_TRANSLATE_NOOP("ActiveIngredient", "Carbamazepine")},
    {32, QT_TRANSLATE_NOOP("ActiveIngredient", "Cefixime")},
    {33, QT_TRANSLATE_NOOP("ActiveIngredient", "Cefalexin")},
    {34, QT_TRANSLATE_NOOP("ActiveIngredient", "Cefotaxime")},
    {35, QT_TRANSLATE_NOOP("ActiveIngredient", "Cefuroxime")},
    {36, QT_TRANSLATE_NOOP("ActiveIngredient", "Ceftriaxone")},
    {37, QT_TRANSLATE_NOOP("ActiveIngredient", "Cimetidine")},
    {38, QT_TRANSLATE_NOOP("ActiveIngredient", "Cinitapride (Cidine)")},
    {39, QT_TRANSLATE_NOOP("ActiveIngredient", "Ciprofloxacin")},
    {40, QT_TRANSLATE_NOOP("ActiveIngredient", "Cinitapride")},
    {41, QT_TRANSLATE_NOOP("ActiveIngredient", "Clarithromycin")},
    {42, QT_TRANSLATE_NOOP("ActiveIngredient", "Clebopride (Flatoril)")},
    {43, QT_TRANSLATE_NOOP("ActiveIngredient", "Clindamycin")},
    {44, QT_TRANSLATE_NOOP("ActiveIngredient", "Clonazepam (Rivotril)")},
    {45, QT_TRANSLATE_NOOP("ActiveIngredient", "Clotrimazole")},
    {46, QT_TRANSLATE_NOOP("ActiveIngredient", "Cloxacillin")},
    {47, QT_TRANSLATE_NOOP("ActiveIngredient", "Vitamin B complex")},
    {48, QT_TRANSLATE_NOOP("ActiveIngredient", "Co-trimoxazole: sulfamethoxazole + trimethoprim (Bactrim)")},
    {49, QT_TRANSLATE_NOOP("ActiveIngredient", "Diazepam")},
    {50, QT_TRANSLATE_NOOP("ActiveIngredient", "Diclofenac")},
    {51, QT_TRANSLATE_NOOP("ActiveIngredient", "Diethylcarbamazine")},
    {52, QT_TRANSLATE_NOOP("ActiveIngredient", "Digoxin")},
    {53, QT_TRANSLATE_NOOP("ActiveIngredient", "Dexamethasone")},
    {54, QT_TRANSLATE_NOOP("ActiveIngredient", "Domperidone (Motilium)")},
    {55, QT_TRANSLATE_NOOP("ActiveIngredient", "Enalapril")},
    {56, QT_TRANSLATE_NOOP("ActiveIngredient", "Dexketoprofen (Enantyum)")},
    {57, QT_TRANSLATE_NOOP("ActiveIngredient", "Erythromycin")},
    {58, QT_TRANSLATE_NOOP("ActiveIngredient", "Spironolactone (Aldactone)")},
    {59, QT_TRANSLATE_NOOP("ActiveIngredient", "Streptomycin")},
    {60, QT_TRANSLATE_NOOP("ActiveIngredient", "Etoricoxib (Arcoxia)")},
    {61, QT_TRANSLATE_NOOP("ActiveIngredient", "Phenytoin")},
    {62, QT_TRANSLATE_NOOP("ActiveIngredient", "Fluconazole")},
    {63, QT_TRANSLATE_NOOP("ActiveIngredient", "Flumazenil")},
    {64, QT_TRANSLATE_NOOP("ActiveIngredient", "Fluorescein, eye drops")},
    {65, QT_TRANSLATE_NOOP("ActiveIngredient", "Fluticasone")},
    {66, QT_TRANSLATE_NOOP("ActiveIngredient", "Formoterol")},
    {67, QT_TRANSLATE_NOOP("ActiveIngredient", "Fosfomycin")},
    {68, QT_TRANSLATE_NOOP("ActiveIngredient", "Ferrous fumarate")},
    {69, QT_TRANSLATE_NOOP("ActiveIngredient", "Furosemide")},
    {70, QT_TRANSLATE_NOOP("ActiveIngredient", "Gentamicin")},
    {71, QT_TRANSLATE_NOOP("ActiveIngredient", "Gliclazide (Diamicron)")},
    {72, QT_TRANSLATE_NOOP("ActiveIngredient", "Griseofulvin")},
    {73, QT_TRANSLATE_NOOP("ActiveIngredient", "Glucose 50%")},
    {74, QT_TRANSLATE_NOOP("ActiveIngredient", "Haloperidol")},
    {75, QT_TRANSLATE_NOOP("ActiveIngredient", "Hydralazine")},
    {76, QT_TRANSLATE_NOOP("ActiveIngredient", "Hydrochlorothiazide")},
    {77, QT_TRANSLATE_NOOP("ActiveIngredient", "Hydrocortisone")},
    {78, QT_TRANSLATE_NOOP("ActiveIngredient", "Ibuprofen")},
    {79, QT_TRANSLATE_NOOP("ActiveIngredient", "Ipratropium")},
    {80, QT_TRANSLATE_NOOP("ActiveIngredient", "Irbesartan")},
    {81, QT_TRANSLATE_NOOP("ActiveIngredient", "Iruxol")},
    {82, QT_TRANSLATE_NOOP("ActiveIngredient", "Itraconazole")},
    {83, QT_TRANSLATE_NOOP("ActiveIngredient", "Ivermectin")},
    {84, QT_TRANSLATE_NOOP("ActiveIngredient", "Ketoconazole")},
    {85, QT_TRANSLATE_NOOP("ActiveIngredient", "Labetalol (Trandate)")},
    {86, QT_TRANSLATE_NOOP("ActiveIngredient", "Lactulose (Duphalac)")},
    {87, QT_TRANSLATE_NOOP("ActiveIngredient", "Levetiracetam (Keppra)")},
    {88, QT_TRANSLATE_NOOP("ActiveIngredient", "Levofloxacin")},
    {89, QT_TRANSLATE_NOOP("ActiveIngredient", "Levothyroxine (Eutirox)")},
    {90, QT_TRANSLATE_NOOP("ActiveIngredient", "Loperamide (Fortasec)")},
    {91, QT_TRANSLATE_NOOP("ActiveIngredient", "Loratadine (Clarityne)")},
    {92, QT_TRANSLATE_NOOP("ActiveIngredient", "Lorazepam")},
    {93, QT_TRANSLATE_NOOP("ActiveIngredient", "Lormetazepam (Noctamid)")},
    {94, QT_TRANSLATE_NOOP("ActiveIngredient", "Macrogol (Movicol)")},
    {95, QT_TRANSLATE_NOOP("ActiveIngredient", "Mebendazole")},
    {96, QT_TRANSLATE_NOOP("ActiveIngredient", "Metformin")},
    {97, QT_TRANSLATE_NOOP("ActiveIngredient", "Metamizole")},
    {98, QT_TRANSLATE_NOOP("ActiveIngredient", "Methylprednisolone")},
    {99, QT_TRANSLATE_NOOP("ActiveIngredient", "Metoclopramide")},
    {100, QT_TRANSLATE_NOOP("ActiveIngredient", "Metronidazole (Flagyl)")},
    {101, QT_TRANSLATE_NOOP("ActiveIngredient", "Metronidazole + spiramycin (Rhodogil)")},
    {102, QT_TRANSLATE_NOOP("ActiveIngredient", "Miconazole")},
    {103, QT_TRANSLATE_NOOP("ActiveIngredient", "Micronutrients")},
    {104, QT_TRANSLATE_NOOP("ActiveIngredient", "Midazolam")},
    {105, QT_TRANSLATE_NOOP("ActiveIngredient", "Monurol")},
    {106, QT_TRANSLATE_NOOP("ActiveIngredient", "Morphine")},
    {107, QT_TRANSLATE_NOOP("ActiveIngredient", "Multivitamins")},
    {108, QT_TRANSLATE_NOOP("ActiveIngredient", "Mupirocin, cream")},
    {109, QT_TRANSLATE_NOOP("ActiveIngredient", "Naloxone")},
    {110, QT_TRANSLATE_NOOP("ActiveIngredient", "Neomycin + bacitracin")},
    {111, QT_TRANSLATE_NOOP("ActiveIngredient", "Nifedipine (Adalat)")},
    {112, QT_TRANSLATE_NOOP("ActiveIngredient", "Nystatin")},
    {113, QT_TRANSLATE_NOOP("ActiveIngredient", "Nitrofural (Furacin)")},
    {114, QT_TRANSLATE_NOOP("ActiveIngredient", "Nitroglycerin")},
    {115, QT_TRANSLATE_NOOP("ActiveIngredient", "Olmesartan")},
    {116, QT_TRANSLATE_NOOP("ActiveIngredient", "Omeprazole")},
    {117, QT_TRANSLATE_NOOP("ActiveIngredient", "Ondansetron (Zofran)")},
    {118, QT_TRANSLATE_NOOP("ActiveIngredient", "Paracetamol")},
    {119, QT_TRANSLATE_NOOP("ActiveIngredient", "Permethrin 5%")},
    {120, QT_TRANSLATE_NOOP("ActiveIngredient", "Risperidone (Risperdal)")},
    {121, QT_TRANSLATE_NOOP("ActiveIngredient", "Praziquantel")},
    {122, QT_TRANSLATE_NOOP("ActiveIngredient", "Prednisolone")},
    {123, QT_TRANSLATE_NOOP("ActiveIngredient", "Prednisone")},
    {124, QT_TRANSLATE_NOOP("ActiveIngredient", "Pregabalin")},
    {125, QT_TRANSLATE_NOOP("ActiveIngredient", "Primaquine")},
    {126, QT_TRANSLATE_NOOP("ActiveIngredient", "Propranolol")},
    {127, QT_TRANSLATE_NOOP("ActiveIngredient", "Ranitidine")},
    {128, QT_TRANSLATE_NOOP("ActiveIngredient", "Salbutamol")},
    {129, QT_TRANSLATE_NOOP("ActiveIngredient", "Simeticone (Aero Red)")},
    {130, QT_TRANSLATE_NOOP("ActiveIngredient", "Oral rehydration salts")},
    {131, QT_TRANSLATE_NOOP("ActiveIngredient", "Lactated Ringer's solution")},
    {132, QT_TRANSLATE_NOOP("ActiveIngredient", "Normal saline 0.9%")},
    {133, QT_TRANSLATE_NOOP("ActiveIngredient", "Glucose 5% solution")},
    {134, QT_TRANSLATE_NOOP("ActiveIngredient", "Silver sulfadiazine ointment")},
    {135, QT_TRANSLATE_NOOP("ActiveIngredient", "Sulfadoxine/pyrimethamine")},
    {136, QT_TRANSLATE_NOOP("ActiveIngredient", "Ferrous sulfate")},
    {137, QT_TRANSLATE_NOOP("ActiveIngredient", "Ferrous sulfate + folic acid")},
    {138, QT_TRANSLATE_NOOP("ActiveIngredient", "Tetracycline eye ointment")},
    {139, QT_TRANSLATE_NOOP("ActiveIngredient", "Tramadol")},
    {140, QT_TRANSLATE_NOOP("ActiveIngredient", "Tramadol + paracetamol")},
    {141, QT_TRANSLATE_NOOP("ActiveIngredient", "Clorazepate (Tranxilium)")},
    {142, QT_TRANSLATE_NOOP("ActiveIngredient", "Sodium valproate, valproic acid (Depakine)")},
    {143, QT_TRANSLATE_NOOP("ActiveIngredient", "Vilanterol")},
    {144, QT_TRANSLATE_NOOP("ActiveIngredient", "Vitamin A")},
    {145, QT_TRANSLATE_NOOP("ActiveIngredient", "Vitamin C")},
    {146, QT_TRANSLATE_NOOP("ActiveIngredient", "Vitamin D")},
    {147, QT_TRANSLATE_NOOP("ActiveIngredient", "Vitamin K")},
    {148, QT_TRANSLATE_NOOP("ActiveIngredient", "Voluven")},
    {149, QT_TRANSLATE_NOOP("ActiveIngredient", "Zinc")},
    {1000, QT_TRANSLATE_NOOP("ActiveIngredient", "OTHERS:")},
    {1001, QT_TRANSLATE_NOOP("ActiveIngredient", "Disinfectant:")},
    {1002, QT_TRANSLATE_NOOP("ActiveIngredient", "Local treatment:")},
};

const Entry kAdministrationRoute[] = {
    {0, QT_TRANSLATE_NOOP("AdministrationRoute", "-")},
    {1, QT_TRANSLATE_NOOP("AdministrationRoute", "RECTAL")},
    {2, QT_TRANSLATE_NOOP("AdministrationRoute", "IV")},
    {3, QT_TRANSLATE_NOOP("AdministrationRoute", "IM")},
    {4, QT_TRANSLATE_NOOP("AdministrationRoute", "SUBCUT")},
    {5, QT_TRANSLATE_NOOP("AdministrationRoute", "DERMAL")},
    {6, QT_TRANSLATE_NOOP("AdministrationRoute", "IO")},
    {7, QT_TRANSLATE_NOOP("AdministrationRoute", "SUBLINGUAL")},
    {8, QT_TRANSLATE_NOOP("AdministrationRoute", "ET")},
    {9, QT_TRANSLATE_NOOP("AdministrationRoute", "INFUSION")},
    {10, QT_TRANSLATE_NOOP("AdministrationRoute", "INHALED")},
    {11, QT_TRANSLATE_NOOP("AdministrationRoute", "NEBULIZED")},
    {12, QT_TRANSLATE_NOOP("AdministrationRoute", "NG TUBE")},
    {13, QT_TRANSLATE_NOOP("AdministrationRoute", "DROPS")},
    {14, QT_TRANSLATE_NOOP("AdministrationRoute", "TABLETS")},
    {15, QT_TRANSLATE_NOOP("AdministrationRoute", "SYRUP")},
};

const Entry kAdministrationDays[] = {
    {0, QT_TRANSLATE_NOOP("AdministrationDays", "-")},
    {1, QT_TRANSLATE_NOOP("AdministrationDays", "1 DAY")},
    {2, QT_TRANSLATE_NOOP("AdministrationDays", "2 DAYS")},
    {3, QT_TRANSLATE_NOOP("AdministrationDays", "3 DAYS")},
    {4, QT_TRANSLATE_NOOP("AdministrationDays", "4 DAYS")},
    {5, QT_TRANSLATE_NOOP("AdministrationDays", "5 DAYS")},
    {6, QT_TRANSLATE_NOOP("AdministrationDays", "6 DAYS")},
    {7, QT_TRANSLATE_NOOP("AdministrationDays", "7 DAYS")},
    {8, QT_TRANSLATE_NOOP("AdministrationDays", "8 DAYS")},
    {9, QT_TRANSLATE_NOOP("AdministrationDays", "9 DAYS")},
    {10, QT_TRANSLATE_NOOP("AdministrationDays", "10 DAYS")},
    {11, QT_TRANSLATE_NOOP("AdministrationDays", "11 DAYS")},
    {12, QT_TRANSLATE_NOOP("AdministrationDays", "12 DAYS")},
    {13, QT_TRANSLATE_NOOP("AdministrationDays", "13 DAYS")},
    {14, QT_TRANSLATE_NOOP("AdministrationDays", "14 DAYS")},
    {15, QT_TRANSLATE_NOOP("AdministrationDays", "15 DAYS")},
    {16, QT_TRANSLATE_NOOP("AdministrationDays", "1 MONTH")},
    {17, QT_TRANSLATE_NOOP("AdministrationDays", "2 MONTHS")},
};

const Domain kDomains[] = {
    {"nursingDiagnosis", "NursingDiagnosis", kNursingDiagnosis, int(std::size(kNursingDiagnosis))},
    {"consciousness", "Consciousness", kConsciousness, int(std::size(kConsciousness))},
    {"mentalStatus", "MentalStatus", kMentalStatus, int(std::size(kMentalStatus))},
    {"armColor", "ArmColor", kArmColor, int(std::size(kArmColor))},
    {"test", "TestResult", kTestResult, int(std::size(kTestResult))},
    {"ratio", "ZScore", kZScore, int(std::size(kZScore))},
    {"retractionType", "RetractionType", kRetractionType, int(std::size(kRetractionType))},
    {"severity", "Severity", kSeverity, int(std::size(kSeverity))},
    {"secretions", "Secretions", kSecretions, int(std::size(kSecretions))},
    {"stool", "Stool", kStool, int(std::size(kStool))},
    {"vomiting", "Vomiting", kVomiting, int(std::size(kVomiting))},
    {"urine", "Urine", kUrine, int(std::size(kUrine))},
    {"capillaryRefill", "CapillaryRefill", kCapillaryRefill, int(std::size(kCapillaryRefill))},
    {"cyanosis", "Cyanosis", kCyanosis, int(std::size(kCyanosis))},
    {"exanthema", "Exanthema", kExanthema, int(std::size(kExanthema))},
    {"infection", "SkinInfection", kSkinInfection, int(std::size(kSkinInfection))},
    {"edema", "SkinEdema", kSkinEdema, int(std::size(kSkinEdema))},
    {"eyeDischarge", "EyeDischarge", kEyeDischarge, int(std::size(kEyeDischarge))},
    {"earPain", "EarPain", kEarPain, int(std::size(kEarPain))},
    {"earDischarge", "EarDischarge", kEarDischarge, int(std::size(kEarDischarge))},
    {"otoscopy", "Otoscopy", kOtoscopy, int(std::size(kOtoscopy))},
    {"tonsils", "Tonsils", kTonsils, int(std::size(kTonsils))},
    {"mouthPain", "MouthPain", kMouthPain, int(std::size(kMouthPain))},
    {"abdominalPainSigns", "AbdominalSigns", kAbdominalSigns, int(std::size(kAbdominalSigns))},
    {"careRecommendation", "CareRecommendation", kCareRecommendation, int(std::size(kCareRecommendation))},
    {"activeIngredient", "ActiveIngredient", kActiveIngredient, int(std::size(kActiveIngredient))},
    {"administrationRoute", "AdministrationRoute", kAdministrationRoute, int(std::size(kAdministrationRoute))},
    {"administrationDays", "AdministrationDays", kAdministrationDays, int(std::size(kAdministrationDays))},
};

const Domain* findDomain(const QString& key) {
    for (const Domain& domain : kDomains) {
        if (key == QLatin1String(domain.key))
            return &domain;
    }
    return nullptr;
}

} // namespace

QStringList pediatricDomains() {
    QStringList keys;
    for (const Domain& domain : kDomains)
        keys.append(QLatin1String(domain.key));
    return keys;
}

QList<DomainOption> pediatricDomainOptions(const QString& domain) {
    QList<DomainOption> options;
    const Domain* d = findDomain(domain);
    if (!d)
        return options;
    for (int i = 0; i < d->count; ++i)
        options.append(DomainOption{d->entries[i].code,
                                    QCoreApplication::translate(d->context, d->entries[i].source)});
    return options;
}

QList<int> pediatricDomainCodes(const QString& domain) {
    QList<int> codes;
    const Domain* d = findDomain(domain);
    if (!d)
        return codes;
    for (int i = 0; i < d->count; ++i)
        codes.append(d->entries[i].code);
    return codes;
}

} // namespace gambasse
