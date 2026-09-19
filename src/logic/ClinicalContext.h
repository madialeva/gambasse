#pragma once

namespace gambasse {

class Patient;

// Which history and consultation entry points apply to a patient.
struct Availability {
    bool showPediatric = false;
    bool showAdult = false;
    bool showPregnancy = false;
    bool canCreatePediatric = false;
    bool canCreateAdult = false;
    bool canCreatePregnancy = false;
};

// Decides which clinical histories and consultations are available for a
// patient from their age, sex and the histories that already exist.
class ClinicalContext {
public:
    Availability availability(const Patient* patient) const;
};

} // namespace gambasse
