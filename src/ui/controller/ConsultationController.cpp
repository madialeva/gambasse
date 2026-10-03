#include <ui/controller/ConsultationController.h>

#include <ui/DateFormat.h>

namespace gambasse {

ConsultationController::ConsultationController(std::unique_ptr<ConsultationStoreBase> store,
                                               QObject* parent)
    : FormController(parent), m_store(std::move(store)) {}

ConsultationController::~ConsultationController() = default;

const QList<FormController::Field>& ConsultationController::fields() const {
    return m_fields;
}

bool ConsultationController::loaded() const {
    return m_loaded;
}

QVariantList ConsultationController::consultations() const {
    return m_consultations;
}

int ConsultationController::currentIndex() const {
    return m_currentIndex;
}

bool ConsultationController::isNew() const {
    return !m_store->edited().isStored();
}

bool ConsultationController::canDelete() const {
    return m_loaded && !isNew();
}

bool ConsultationController::canStartNew() const {
    return m_loaded && !isNew();
}

QString ConsultationController::dateText() const {
    return m_store->edited().date.date().toString(QString::fromLatin1(kDateFormat));
}

bool ConsultationController::load(qlonglong patientId, const QString& patientName) {
    setPatient(patientId, patientName);
    m_loaded = m_store->list(patientId);
    if (!m_loaded) {
        emit stateChanged();
        return false;
    }
    publishList();
    startNew();
    return true;
}

void ConsultationController::select(int index) {
    if (!m_loaded || index < 0 || index >= m_store->size())
        return;
    m_store->select(index);
    m_currentIndex = index;
    refreshValues();
    emit stateChanged();
}

void ConsultationController::startNew() {
    if (!m_loaded)
        return;
    m_store->startNew(patientId());
    m_currentIndex = -1;
    refreshValues();
    emit stateChanged();
}

int ConsultationController::save() {
    if (!m_loaded)
        return 1;
    writeValues();
    const int result = m_store->save();
    if (result != 0)
        return result;
    // The saved consultation stays in the form, now selected in the list.
    const QString key = m_store->edited().originalKey;
    if (m_store->list(patientId()))
        publishList();
    m_currentIndex = -1;
    for (int i = 0; i < m_store->size(); ++i) {
        if (m_store->at(i).originalKey == key) {
            m_currentIndex = i;
            break;
        }
    }
    refreshValues(); // the stored row is now the model in memory
    emit stateChanged();
    return 0;
}

bool ConsultationController::remove() {
    if (!canDelete())
        return false;
    if (!m_store->remove())
        return false;
    if (m_store->list(patientId()))
        publishList();
    startNew();
    return true;
}

void ConsultationController::publishList() {
    m_consultations.clear();
    for (int i = 0; i < m_store->size(); ++i) {
        const Consultation& c = m_store->at(i);
        QString reason = c.reason;
        reason.replace(QLatin1Char('\n'), QLatin1Char(' '));
        m_consultations.append(QVariantMap{
            {QStringLiteral("date"),
             c.date.toString(QString::fromLatin1(kDateFormat) + QStringLiteral(" HH:mm"))},
            {QStringLiteral("reason"), reason.simplified()}});
    }
    emit consultationsChanged();
}

} // namespace gambasse
