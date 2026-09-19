#include <logic/PatientService.h>

#include <data/common/Database.h>
#include <data/model/Patient.h>
#include <logic/PhotoManager.h>

namespace gambasse {

qlonglong PatientService::nextId() const {
    return Database::instance().nextId();
}

PatientService::SaveResult PatientService::create(Patient& patient) const {
    if (patient.name.isEmpty())
        return SaveResult::NameRequired;
    if (Database::instance().hasDuplicate(patient, -1))
        return SaveResult::Duplicate;
    if (!Database::instance().insert(patient))
        return SaveResult::Error;
    return SaveResult::Saved;
}

PatientService::SaveResult PatientService::update(const Patient& previous,
                                                  Patient& patient) const {
    if (patient.name.isEmpty())
        return SaveResult::NameRequired;
    patient.id = previous.id;
    if (Database::instance().hasDuplicate(patient, previous.id))
        return SaveResult::Duplicate;
    if (!Database::instance().update(patient))
        return SaveResult::Error;
    PhotoManager().renameFor(previous, patient);
    return SaveResult::Saved;
}

bool PatientService::remove(const Patient& patient) const {
    if (!Database::instance().remove(patient.id))
        return false;
    PhotoManager().removeFor(patient);
    return true;
}

} // namespace gambasse
