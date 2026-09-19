#pragma once

#include <QtGlobal>

#include <data/model/PediatricHistory.h>

namespace gambasse {

// Pediatric history load/save coordination. The window only gathers/renders
// values and shows messages; all database access goes through here.
class PediatricHistoryService {
public:
    enum class SaveResult { Saved, Error };

    // Loads the history of the patient into history. When none exists, fills
    // history with the new-history defaults (opening date = today) and sets
    // isNew. Returns false on database error.
    bool load(qlonglong patientId, PediatricHistory& history, bool& isNew) const;
    SaveResult save(bool isNew, const PediatricHistory& history) const;
    bool remove(qlonglong patientId) const;
};

} // namespace gambasse
