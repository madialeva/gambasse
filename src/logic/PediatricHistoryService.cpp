#include <logic/PediatricHistoryService.h>

#include <QDate>

#include <data/common/Database.h>

namespace gambasse {

bool PediatricHistoryService::load(qlonglong patientId, PediatricHistory& history,
                                   bool& isNew) const {
    if (Database::instance().hasPediatricHistory(patientId)) {
        isNew = false;
        return Database::instance().loadPediatricHistory(patientId, history);
    }
    history = PediatricHistory();
    history.patientId = patientId;
    history.openingDate = QDate::currentDate();
    isNew = true;
    return true;
}

PediatricHistoryService::SaveResult
PediatricHistoryService::save(bool isNew, const PediatricHistory& history) const {
    const bool ok = isNew ? Database::instance().insertPediatricHistory(history)
                          : Database::instance().updatePediatricHistory(history);
    return ok ? SaveResult::Saved : SaveResult::Error;
}

bool PediatricHistoryService::remove(qlonglong patientId) const {
    return Database::instance().removePediatricHistory(patientId);
}

} // namespace gambasse
