#include <ui/controller/HistoryController.h>

#include <QDebug>

#include <logic/PediatricHistoryLabels.h>
#include <ui/DateFormat.h>

namespace gambasse {

HistoryController::HistoryController(QObject* parent) : QObject(parent) {}

HistoryController::~HistoryController() = default;

qlonglong HistoryController::patientId() const {
    return m_patientId;
}

QString HistoryController::patientName() const {
    return m_patientName;
}

bool HistoryController::loaded() const {
    return m_loaded;
}

bool HistoryController::isNew() const {
    return m_isNew;
}

bool HistoryController::canDelete() const {
    return m_loaded && !m_isNew;
}

QVariantMap HistoryController::values() const {
    return m_values;
}

bool HistoryController::load(qlonglong patientId, const QString& patientName) {
    m_patientId = patientId;
    m_patientName = patientName;
    const bool ok = serviceLoad();
    setLoaded(ok, m_isNew);
    if (ok)
        refreshValues();
    return ok;
}

QVariant HistoryController::value(const QString& field) const {
    return m_values.value(field);
}

void HistoryController::setValue(const QString& field, const QVariant& value) {
    // Values are only kept in the map while editing: the model is written on
    // save(), so a form binding can call this on every keystroke.
    if (!m_values.contains(field)) {
        qWarning() << "HistoryController: unknown field" << field;
        return;
    }
    if (m_values.value(field) == value)
        return;
    m_values.insert(field, value);
    emit valuesChanged();
}

QStringList HistoryController::fieldNames() const {
    QStringList names;
    names.reserve(static_cast<int>(fields().size()));
    for (const Field& field : fields())
        names.append(field.name);
    return names;
}

int HistoryController::save() {
    if (!m_loaded)
        return 1;
    for (const Field& field : fields()) {
        if (field.write)
            field.write(m_values.value(field.name));
    }
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

void HistoryController::refreshValues() {
    m_values.clear();
    for (const Field& field : fields()) {
        if (field.read)
            m_values.insert(field.name, field.read());
    }
    emit valuesChanged();
}

const HistoryController::Field* HistoryController::findField(const QString& name) const {
    for (const Field& field : fields()) {
        if (field.name == name)
            return &field;
    }
    return nullptr;
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

QVariant HistoryController::toVariant(const QDate& date) {
    if (!date.isValid())
        return QString();
    return date.toString(QString::fromLatin1(kDateFormat));
}

QDate HistoryController::toDate(const QVariant& value, const QDate& fallback) {
    const QString text = value.toString().trimmed();
    if (text.isEmpty())
        return fallback;
    const QDate date = QDate::fromString(text, QString::fromLatin1(kDateFormat));
    return date.isValid() ? date : fallback;
}

QVariant HistoryController::toVariant(const QString& text) {
    return text;
}

QString HistoryController::toText(const QVariant& value, int maxLength) {
    QString text = value.toString().trimmed();
    if (maxLength > 0)
        text = text.left(maxLength);
    return text;
}

QVariant HistoryController::toVariant(double value, int decimals) {
    return QString::number(value, 'f', decimals).replace(QLatin1Char('.'), QLatin1Char(','));
}

double HistoryController::toDouble(const QVariant& value) {
    QString normalized = value.toString().trimmed();
    normalized.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    const double parsed = normalized.toDouble(&ok);
    return ok ? parsed : 0.0;
}

int HistoryController::toInt(const QVariant& value, int low, int high) {
    return qBound(low, value.toString().trimmed().toInt(), high);
}

} // namespace gambasse
