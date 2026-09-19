#include <logic/ClinicalContext.h>

#include <data/common/Database.h>
#include <data/model/Patient.h>

namespace gambasse {

Availability ClinicalContext::availability(const Patient* patient) const {
    Availability availability;
    if (!patient)
        return availability;

    Database& db = Database::instance();
    const bool hasPediatric = db.hasPediatricHistory(patient->id);
    const bool hasAdult = db.hasAdultHistory(patient->id);
    const bool hasPregnancy = db.hasPregnancyHistory(patient->id);

    const bool isChild = (patient->ageRange <= 16) ||
                         (patient->isValidDate() && patient->ageInYears() <= 16);
    const bool isAdult = (patient->ageRange > 16) ||
                         (patient->isValidDate() && patient->ageInYears() > 16);
    const bool isPregnant = (patient->sex == Patient::Sex::Muller) &&
                            ((patient->ageRange >= 9) ||
                             (patient->isValidDate() && patient->ageInYears() >= 9));

    availability.showPediatric = hasPediatric;
    availability.showAdult = hasAdult;
    availability.showPregnancy = hasPregnancy;
    availability.canCreatePediatric = !hasPediatric && isChild;
    availability.canCreateAdult = !hasAdult && isAdult;
    availability.canCreatePregnancy = !hasPregnancy && isPregnant;
    return availability;
}

} // namespace gambasse
