#pragma once

#include <ui/controller/FormController.h>

namespace gambasse {

// Base of the history screen controllers (pediatric, adult, pregnancy).
//
// A patient has at most one history of each kind: the screen loads it (or the
// new-history defaults), saves it and deletes it. The field table and the
// value map come from FormController.
class HistoryController : public FormController {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PatientController when a history screen opens.")
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    // A history that does not exist yet cannot be deleted.
    Q_PROPERTY(bool isNew READ isNew NOTIFY loadedChanged)
    Q_PROPERTY(bool canDelete READ canDelete NOTIFY loadedChanged)
public:
    explicit HistoryController(QObject* parent = nullptr);
    ~HistoryController() override;

    bool loaded() const;
    bool isNew() const;
    bool canDelete() const; // loaded and not a new history

    // Loads the history of the patient (or the new-history defaults) and
    // publishes its values. Returns false on database error.
    Q_INVOKABLE bool load(qlonglong patientId, const QString& patientName);

    // Domain lists the forms show in combo boxes: [{ "value": int,
    // "label": string }] with the label already translated. The persisted
    // value is the integer, so the language never changes the data.
    Q_INVOKABLE QVariantList domesticAnimalsOptions() const;
    Q_INVOKABLE QVariantList treatmentTypeOptions() const;

    // Persists the current values: insert for a new history, update otherwise.
    // Returns 0 Saved, 1 Error (mirrors the services' SaveResult).
    Q_INVOKABLE int save();
    // Deletes the history of the patient. New histories cannot be deleted.
    Q_INVOKABLE bool remove();
    // Drops the pending edits and republishes the stored values.
    Q_INVOKABLE void revert();

signals:
    void loadedChanged();

protected:
    // Service calls of the concrete screen.
    virtual bool serviceLoad() = 0;
    virtual int serviceSave() = 0;
    virtual bool serviceRemove() = 0;

    // Writable reference to the isNew flag, for the service load() calls.
    bool& m_isNewFlag();
    // Called by the service implementations when the load outcome is known.
    void setLoaded(bool loaded, bool isNew);

private:
    bool m_loaded = false;
    bool m_isNew = true;
};

} // namespace gambasse
