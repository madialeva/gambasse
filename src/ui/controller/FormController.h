#pragma once

#include <QDate>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <functional>

namespace gambasse {

// Base of the form screen controllers (histories and consultations).
//
// A clinical form has dozens of fields, so instead of one Q_PROPERTY per field
// each screen publishes a name -> value map plus a field table that knows how
// to read and write every field on its model. The QML forms then render a
// declarative schema bound to value()/setValue(), while the rules (coercion,
// bounds, date parsing, persistence) stay here in C++ through the existing
// services. No Widgets, no Quick: testable without a GUI.
class FormController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PatientController when a form screen opens.")
    Q_PROPERTY(qlonglong patientId READ patientId CONSTANT)
    Q_PROPERTY(QString patientName READ patientName CONSTANT)
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
public:
    // Read and write one field of the screen model. The lambdas are bound to
    // the concrete controller, so a field cannot be read but not written.
    struct Field {
        QString name;
        std::function<QVariant()> read;
        std::function<void(const QVariant&)> write;
    };

    explicit FormController(QObject* parent = nullptr);
    ~FormController() override;

    qlonglong patientId() const;
    QString patientName() const;
    QVariantMap values() const;

    Q_INVOKABLE QVariant value(const QString& field) const;
    Q_INVOKABLE void setValue(const QString& field, const QVariant& value);
    // Field names in form order, for tests and for schema validation.
    Q_INVOKABLE QStringList fieldNames() const;

    // Date helpers shared by the screens: the form works with the same
    // dd/MM/yyyy text as the Widgets fields (empty means no date).
    static QVariant toVariant(const QDate& date);
    static QDate toDate(const QVariant& value, const QDate& fallback = QDate(1900, 1, 1));
    static QVariant toVariant(const QString& text);
    static QString toText(const QVariant& value, int maxLength = 0);
    // Decimals keep the comma separator used by the Widgets fields and accept
    // both separators when reading, like the Widgets parseDecimal().
    static QVariant toVariant(double value, int decimals);
    static double toDouble(const QVariant& value);
    static int toInt(const QVariant& value, int low, int high);

signals:
    void valuesChanged();

protected:
    // Field factories shared by the screens: one line per field, with the same
    // trimming, bounds and date rules the Widgets gather step applied. The
    // model is passed by reference so a field can never be read but not
    // written, and both directions stay in one place.
    template <typename Model, typename T>
    void addText(const QString& name, Model& model, T Model::*member, int maxLength = 0) {
        m_fields.append(Field{name,
                              [&model, member] { return model.*member; },
                              [&model, member, maxLength](const QVariant& value) {
                                  model.*member = toText(value, maxLength);
                              }});
    }
    template <typename Model>
    void addInt(const QString& name, Model& model, int Model::*member, int low, int high) {
        m_fields.append(Field{name,
                              [&model, member] { return model.*member; },
                              [&model, member, low, high](const QVariant& value) {
                                  model.*member = toInt(value, low, high);
                              }});
    }
    template <typename Model>
    void addBool(const QString& name, Model& model, bool Model::*member) {
        m_fields.append(Field{name,
                              [&model, member] { return model.*member; },
                              [&model, member](const QVariant& value) {
                                  model.*member = value.toBool();
                              }});
    }
    template <typename Model>
    void addDecimal(const QString& name, Model& model, double Model::*member, int decimals,
                    double high) {
        m_fields.append(Field{name,
                              [&model, member, decimals] {
                                  return toVariant(model.*member, decimals);
                              },
                              [&model, member, high](const QVariant& value) {
                                  model.*member = qBound(0.0, toDouble(value), high);
                              }});
    }
    template <typename Model>
    void addDate(const QString& name, Model& model, QDate Model::*member) {
        m_fields.append(Field{name,
                              [&model, member] { return toVariant(model.*member); },
                              [&model, member](const QVariant& value) {
                                  model.*member = toDate(value);
                              }});
    }
    // Domain list stored as an enum member: the QML combo works with the int.
    template <typename Model, typename Enum>
    void addEnum(const QString& name, Model& model, Enum Model::*member, int count) {
        m_fields.append(Field{name,
                              [&model, member] { return static_cast<int>(model.*member); },
                              [&model, member, count](const QVariant& value) {
                                  model.*member = static_cast<Enum>(qBound(0, value.toInt(), count));
                              }});
    }

    // Same factories for screens whose repeated rows are listed as pointers
    // (the previous-treatment blocks of the pregnancy form, the treatment rows
    // of the pediatric consultation).
    void addText(const QString& name, QString* target, int maxLength = 0) {
        m_fields.append(Field{name,
                              [target] { return *target; },
                              [target, maxLength](const QVariant& value) {
                                  *target = toText(value, maxLength);
                              }});
    }
    void addDate(const QString& name, QDate* target) {
        m_fields.append(Field{name,
                              [target] { return toVariant(*target); },
                              [target](const QVariant& value) { *target = toDate(value); }});
    }
    void addInt(const QString& name, int* target, int low, int high) {
        m_fields.append(Field{name,
                              [target] { return *target; },
                              [target, low, high](const QVariant& value) {
                                  *target = toInt(value, low, high);
                              }});
    }
    void addBool(const QString& name, bool* target) {
        m_fields.append(Field{name,
                              [target] { return *target; },
                              [target](const QVariant& value) { *target = value.toBool(); }});
    }
    // Domain code stored as an int. The allowed codes are not always
    // contiguous (the pediatric active ingredients jump to 1000-1002), so an
    // unknown code falls back to 0, the empty option.
    void addCode(const QString& name, int* target, const QList<int>& codes) {
        m_fields.append(Field{name,
                              [target] { return *target; },
                              [target, codes](const QVariant& value) {
                                  const int code = value.toInt();
                                  *target = codes.contains(code) ? code : 0;
                              }});
    }

    // Field table of the concrete screen, built once in its constructor.
    virtual const QList<Field>& fields() const = 0;

    void setPatient(qlonglong patientId, const QString& patientName);
    // Writes every published value back to the model (before a save).
    void writeValues();
    // Publishes every field value to QML.
    void refreshValues();

protected:
    QList<Field> m_fields;

private:
    qlonglong m_patientId = 0;
    QString m_patientName;
    QVariantMap m_values;
};

} // namespace gambasse
