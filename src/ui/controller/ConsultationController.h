#pragma once

#include <ui/controller/FormController.h>

#include <data/model/Consultation.h>
#include <logic/ConsultationService.h>

#include <memory>

namespace gambasse {

// List, edited model and service of one consultation model, behind an
// interface so that ConsultationController can own it without being a
// template (a template base would hide the QObject chain from qmllint).
class ConsultationStoreBase {
public:
    virtual ~ConsultationStoreBase() = default;
    virtual bool list(qlonglong patientId) = 0;
    virtual int size() const = 0;
    virtual const Consultation& at(int index) const = 0;
    virtual const Consultation& edited() const = 0;
    virtual void select(int index) = 0;
    virtual void startNew(qlonglong patientId) = 0;
    virtual int save() = 0;
    virtual bool remove() = 0;
};

template <typename M>
class ConsultationStore : public ConsultationStoreBase {
public:
    bool list(qlonglong patientId) override { return service.list(patientId, consultations); }
    int size() const override { return static_cast<int>(consultations.size()); }
    const Consultation& at(int index) const override { return consultations.at(index); }
    const Consultation& edited() const override { return current; }
    void select(int index) override { current = consultations.at(index); }
    void startNew(qlonglong patientId) override { current = service.newConsultation(patientId); }
    int save() override { return static_cast<int>(service.save(current)); }
    bool remove() override { return service.remove(current); }

    QList<M> consultations;
    // The consultation in the form: the field table is bound to it.
    M current;
    ConsultationService<M> service;
};

// Base of the consultation screen controllers (pediatric, adult, pregnancy).
//
// A patient has any number of consultations: the screen lists them (most
// recent first) and edits one at a time, either a new one dated now or one
// selected from the list. Saving keeps the screen open and refreshes the list;
// deleting leaves a new consultation in the form. The field table and the
// value map come from FormController; each screen only declares its fields
// over the edited model of its store.
class ConsultationController : public FormController {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PatientController when a consultation screen opens.")
    Q_PROPERTY(bool loaded READ loaded NOTIFY stateChanged)
    // [{ "date": "dd/MM/yyyy HH:mm", "reason": string }], most recent first.
    Q_PROPERTY(QVariantList consultations READ consultations NOTIFY consultationsChanged)
    // Row of the edited consultation in the list, -1 for a new one.
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY stateChanged)
    Q_PROPERTY(bool isNew READ isNew NOTIFY stateChanged)
    // A stored consultation can be deleted and left for a new one.
    Q_PROPERTY(bool canDelete READ canDelete NOTIFY stateChanged)
    Q_PROPERTY(bool canStartNew READ canStartNew NOTIFY stateChanged)
    // Date of the edited consultation (dd/MM/yyyy), read-only in the form.
    Q_PROPERTY(QString dateText READ dateText NOTIFY stateChanged)
public:
    ~ConsultationController() override;

    bool loaded() const;
    QVariantList consultations() const;
    int currentIndex() const;
    bool isNew() const;
    bool canDelete() const;
    bool canStartNew() const;
    QString dateText() const;

    // Lists the consultations of the patient and prepares a new one. Returns
    // false on database error.
    Q_INVOKABLE bool load(qlonglong patientId, const QString& patientName);
    // Edits the consultation of that list row.
    Q_INVOKABLE void select(int index);
    // Clears the form for a new consultation dated now.
    Q_INVOKABLE void startNew();
    // Persists the current values: insert for a new consultation, update
    // otherwise. The screen stays on the saved consultation. Returns 0 Saved,
    // 1 Error (mirrors the service's SaveResult).
    Q_INVOKABLE int save();
    // Deletes the edited consultation and prepares a new one. New
    // consultations cannot be deleted.
    Q_INVOKABLE bool remove();

signals:
    void stateChanged();
    void consultationsChanged();

protected:
    ConsultationController(std::unique_ptr<ConsultationStoreBase> store, QObject* parent);

    const QList<Field>& fields() const override;
    // The typed store of the concrete screen, to bind its fields.
    template <typename M>
    ConsultationStore<M>& store() {
        return static_cast<ConsultationStore<M>&>(*m_store);
    }

private:
    // Rebuilds the published list from the store.
    void publishList();

    std::unique_ptr<ConsultationStoreBase> m_store;
    QVariantList m_consultations;
    int m_currentIndex = -1;
    bool m_loaded = false;
};

} // namespace gambasse
