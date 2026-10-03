#include <ui/controller/HistoryController.h>

#include <logic/PediatricHistoryLabels.h>

namespace gambasse {

HistoryController::HistoryController(QObject* parent) : FormController(parent) {}

HistoryController::~HistoryController() = default;

bool HistoryController::loaded() const {
    return m_loaded;
}

bool HistoryController::isNew() const {
    return m_isNew;
}

bool HistoryController::canDelete() const {
    return m_loaded && !m_isNew;
}

bool HistoryController::load(qlonglong patientId, const QString& patientName) {
    setPatient(patientId, patientName);
    const bool ok = serviceLoad();
    setLoaded(ok, m_isNew);
    if (ok)
        refreshValues();
    return ok;
}

int HistoryController::save() {
    if (!m_loaded)
        return 1;
    writeValues();
    const int result = serviceSave();
    if (result == 0)
        refreshValues(); // the stored row is now the model in memory
    return result;
}

bool HistoryController::remove() {
    if (!canDelete())
        return false;
    if (!serviceRemove())
        return false;
    m_isNew = true; // the history is gone: the screen reloads as a new one
    emit loadedChanged();
    return true;
}

void HistoryController::revert() {
    if (!m_loaded)
        return;
    if (serviceLoad())
        refreshValues();
}

bool& HistoryController::m_isNewFlag() {
    return m_isNew;
}

void HistoryController::setLoaded(bool loaded, bool isNew) {
    m_loaded = loaded;
    m_isNew = isNew;
    emit loadedChanged();
}

QVariantList HistoryController::domesticAnimalsOptions() const {
    QVariantList options;
    for (int v = 0; v <= 3; ++v) {
        options.append(QVariantMap{
            {QStringLiteral("value"), v},
            {QStringLiteral("label"),
             domesticAnimalsLabel(static_cast<PediatricHistory::DomesticAnimals>(v))}});
    }
    return options;
}

QVariantList HistoryController::treatmentTypeOptions() const {
    QVariantList options;
    for (int v = 0; v <= 7; ++v) {
        options.append(QVariantMap{
            {QStringLiteral("value"), v},
            {QStringLiteral("label"),
             treatmentTypeLabel(static_cast<PediatricHistory::TreatmentType>(v))}});
    }
    return options;
}

} // namespace gambasse
