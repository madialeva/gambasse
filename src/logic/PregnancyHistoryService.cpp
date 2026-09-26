#include <logic/PregnancyHistoryService.h>

#include <QDate>

#include <data/common/Database.h>

namespace gambasse {

bool PregnancyHistoryService::load(qlonglong patientId, PregnancyHistory& history,
                                   bool& isNew) const {
    if (Database::instance().hasPregnancyHistory(patientId)) {
        isNew = false;
        return Database::instance().loadPregnancyHistory(patientId, history);
    }
    history = PregnancyHistory();
    history.patientId = patientId;
    history.openingDate = QDate::currentDate();
    isNew = true;
    return true;
}

PregnancyHistoryService::SaveResult
PregnancyHistoryService::save(bool isNew, const PregnancyHistory& history) const {
    const bool ok = isNew ? Database::instance().insertPregnancyHistory(history)
                          : Database::instance().updatePregnancyHistory(history);
    return ok ? SaveResult::Saved : SaveResult::Error;
}

bool PregnancyHistoryService::remove(qlonglong patientId) const {
    return Database::instance().removePregnancyHistory(patientId);
}

} // namespace gambasse
