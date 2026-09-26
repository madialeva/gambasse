#include <logic/AdultHistoryService.h>

#include <QDate>

#include <data/common/Database.h>

namespace gambasse {

bool AdultHistoryService::load(qlonglong patientId, AdultHistory& history,
                               bool& isNew) const {
    if (Database::instance().hasAdultHistory(patientId)) {
        isNew = false;
        return Database::instance().loadAdultHistory(patientId, history);
    }
    history = AdultHistory();
    history.patientId = patientId;
    history.openingDate = QDate::currentDate();
    isNew = true;
    return true;
}

AdultHistoryService::SaveResult
AdultHistoryService::save(bool isNew, const AdultHistory& history) const {
    const bool ok = isNew ? Database::instance().insertAdultHistory(history)
                          : Database::instance().updateAdultHistory(history);
    return ok ? SaveResult::Saved : SaveResult::Error;
}

bool AdultHistoryService::remove(qlonglong patientId) const {
    return Database::instance().removeAdultHistory(patientId);
}

} // namespace gambasse
