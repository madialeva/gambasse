#include <ui/controller/FormController.h>

#include <QDebug>

#include <ui/DateFormat.h>

namespace gambasse {

FormController::FormController(QObject* parent) : QObject(parent) {}

FormController::~FormController() = default;

qlonglong FormController::patientId() const {
    return m_patientId;
}

QString FormController::patientName() const {
    return m_patientName;
}

QVariantMap FormController::values() const {
    return m_values;
}

QVariant FormController::value(const QString& field) const {
    return m_values.value(field);
}

void FormController::setValue(const QString& field, const QVariant& value) {
    // Values are only kept in the map while editing: the model is written on
    // save, so a form binding can call this on every keystroke.
    if (!m_values.contains(field)) {
        qWarning() << "FormController: unknown field" << field;
        return;
    }
    if (m_values.value(field) == value)
        return;
    m_values.insert(field, value);
    emit valuesChanged();
}

QStringList FormController::fieldNames() const {
    QStringList names;
    names.reserve(static_cast<int>(fields().size()));
    for (const Field& field : fields())
        names.append(field.name);
    return names;
}

void FormController::setPatient(qlonglong patientId, const QString& patientName) {
    m_patientId = patientId;
    m_patientName = patientName;
}

void FormController::writeValues() {
    for (const Field& field : fields()) {
        if (field.write)
            field.write(m_values.value(field.name));
    }
}

void FormController::refreshValues() {
    m_values.clear();
    for (const Field& field : fields()) {
        if (field.read)
            m_values.insert(field.name, field.read());
    }
    emit valuesChanged();
}

QVariant FormController::toVariant(const QDate& date) {
    if (!date.isValid())
        return QString();
    return date.toString(QString::fromLatin1(kDateFormat));
}

QDate FormController::toDate(const QVariant& value, const QDate& fallback) {
    const QString text = value.toString().trimmed();
    if (text.isEmpty())
        return fallback;
    const QDate date = QDate::fromString(text, QString::fromLatin1(kDateFormat));
    return date.isValid() ? date : fallback;
}

QVariant FormController::toVariant(const QString& text) {
    return text;
}

QString FormController::toText(const QVariant& value, int maxLength) {
    QString text = value.toString().trimmed();
    if (maxLength > 0)
        text = text.left(maxLength);
    return text;
}

QVariant FormController::toVariant(double value, int decimals) {
    return QString::number(value, 'f', decimals).replace(QLatin1Char('.'), QLatin1Char(','));
}

double FormController::toDouble(const QVariant& value) {
    QString normalized = value.toString().trimmed();
    normalized.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    const double parsed = normalized.toDouble(&ok);
    return ok ? parsed : 0.0;
}

int FormController::toInt(const QVariant& value, int low, int high) {
    return qBound(low, value.toString().trimmed().toInt(), high);
}

} // namespace gambasse
