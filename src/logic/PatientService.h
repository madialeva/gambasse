#pragma once

#include <QtGlobal>

namespace gambasse {

class Patient;

// Patient write operations: validation, duplicate detection and coordination
// with the photo files. The UI only gathers/renders values and shows messages.
class PatientService {
public:
    enum class SaveResult { Saved, NameRequired, Duplicate, Error };

    qlonglong nextId() const;
    SaveResult create(Patient& patient) const;
    SaveResult update(const Patient& previous, Patient& patient) const;
    bool remove(const Patient& patient) const;
};

} // namespace gambasse
