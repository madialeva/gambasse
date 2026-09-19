#include <logic/PediatricHistoryLabels.h>

#include <QCoreApplication>

namespace gambasse {

QString domesticAnimalsLabel(PediatricHistory::DomesticAnimals value) {
    switch (value) {
    case PediatricHistory::DomesticAnimals::Stable:
        return QCoreApplication::translate("DomesticAnimals", "In stable");
    case PediatricHistory::DomesticAnimals::Dwelling:
        return QCoreApplication::translate("DomesticAnimals", "In dwelling");
    case PediatricHistory::DomesticAnimals::StableAndDwelling:
        return QCoreApplication::translate("DomesticAnimals", "In stable and dwelling");
    case PediatricHistory::DomesticAnimals::None:
    default:
        return QCoreApplication::translate("DomesticAnimals", "None");
    }
}

QString treatmentTypeLabel(PediatricHistory::TreatmentType value) {
    switch (value) {
    case PediatricHistory::TreatmentType::VitaminA:
        return QCoreApplication::translate("TreatmentType", "Vitamin A");
    case PediatricHistory::TreatmentType::Albendazole:
        return QCoreApplication::translate("TreatmentType", "Albendazole");
    case PediatricHistory::TreatmentType::Iron:
        return QCoreApplication::translate("TreatmentType", "Iron");
    case PediatricHistory::TreatmentType::VitaminC:
        return QCoreApplication::translate("TreatmentType", "Vitamin C");
    case PediatricHistory::TreatmentType::Micronutrients:
        return QCoreApplication::translate("TreatmentType", "Micronutrients");
    case PediatricHistory::TreatmentType::Zinc:
        return QCoreApplication::translate("TreatmentType", "Zinc");
    case PediatricHistory::TreatmentType::AscorbicAcid:
        return QCoreApplication::translate("TreatmentType", "Ascorbic acid");
    case PediatricHistory::TreatmentType::None:
    default:
        return QCoreApplication::translate("TreatmentType", "None");
    }
}

} // namespace gambasse
