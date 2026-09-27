#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

#include <data/model/Patient.h>
#include <data/model/PatientsModel.h>
#include <logic/AppSettings.h>
#include <logic/ClinicalContext.h>
#include <logic/PatientService.h>
#include <logic/PhotoManager.h>
#include <ui/controller/HistoryController.h>
#include <ui/filter/ColumnFilterProxy.h>

namespace gambasse {

// Bridge between the QML main screen and the patient domain: owns the
// in-memory model plus its filter proxy, the current selection, the edit
// session state and the detail values shown in the panel. All persistence,
// validation and availability rules reuse PatientService, PhotoManager and
// ClinicalContext; the QML side only renders values and shows messages.
// Registered as the QML type `PatientController`: Root.qml instantiates it and
// loads it once the shell is complete. It only needs QtCore and the models, so
// it is also testable without a GUI.
//
// SaveResult codes returned by saveDraft(): 0 Saved, 1 NameRequired,
// 2 Duplicate, 3 Error (mirrors PatientService::SaveResult).
class PatientController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel* gridModel READ gridModel CONSTANT)
    Q_PROPERTY(QStringList columnNames READ columnNames NOTIFY headersChanged)
    Q_PROPERTY(int columnCount READ columnCount CONSTANT)
    Q_PROPERTY(int sortColumn READ sortColumn NOTIFY sortChanged)
    Q_PROPERTY(int sortOrder READ sortOrder NOTIFY sortChanged)
    Q_PROPERTY(int currentRow READ currentRow NOTIFY currentChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY currentChanged)
    Q_PROPERTY(bool editing READ editing NOTIFY editingChanged)
    Q_PROPERTY(qlonglong patientId READ patientId NOTIFY detailChanged)
    Q_PROPERTY(QString patientName READ patientName NOTIFY detailChanged)
    Q_PROPERTY(int patientSex READ patientSex NOTIFY detailChanged)
    Q_PROPERTY(QString birthDateText READ birthDateText NOTIFY detailChanged)
    Q_PROPERTY(int ageRange READ ageRange NOTIFY detailChanged)
    Q_PROPERTY(QString address READ address NOTIFY detailChanged)
    Q_PROPERTY(QString cohabitants READ cohabitants NOTIFY detailChanged)
    Q_PROPERTY(QString contactPerson READ contactPerson NOTIFY detailChanged)
    Q_PROPERTY(int siblingCount READ siblingCount NOTIFY detailChanged)
    Q_PROPERTY(QString photoSource READ photoSource NOTIFY detailChanged)
    Q_PROPERTY(bool showPediatric READ showPediatric NOTIFY availabilityChanged)
    Q_PROPERTY(bool showAdult READ showAdult NOTIFY availabilityChanged)
    Q_PROPERTY(bool showPregnancy READ showPregnancy NOTIFY availabilityChanged)
    Q_PROPERTY(bool canCreatePediatric READ canCreatePediatric NOTIFY availabilityChanged)
    Q_PROPERTY(bool canCreateAdult READ canCreateAdult NOTIFY availabilityChanged)
    Q_PROPERTY(bool canCreatePregnancy READ canCreatePregnancy NOTIFY availabilityChanged)
    Q_PROPERTY(QString sexHomeLabel READ sexHomeLabel NOTIFY headersChanged)
    Q_PROPERTY(QString sexMullerLabel READ sexMullerLabel NOTIFY headersChanged)
    Q_PROPERTY(bool weekStartsMonday READ weekStartsMonday CONSTANT)
public:
    explicit PatientController(QObject* parent = nullptr);

    // Loads the model, sorts by name ascending and selects the first row.
    Q_INVOKABLE bool load();

    QAbstractItemModel* gridModel() const;
    QStringList columnNames() const;
    int columnCount() const;
    int sortColumn() const;
    int sortOrder() const; // Qt::SortOrder as int
    int currentRow() const; // proxy row, -1 when nothing applies
    bool hasSelection() const;
    bool editing() const;

    qlonglong patientId() const;
    QString patientName() const;
    int patientSex() const; // Patient::Sex as int
    QString birthDateText() const; // dd/MM/yyyy (1900-01-01 when null)
    int ageRange() const;
    QString address() const;
    QString cohabitants() const;
    QString contactPerson() const;
    int siblingCount() const;
    QString photoSource() const; // file:// URL or qrc default

    bool showPediatric() const;
    bool showAdult() const;
    bool showPregnancy() const;
    bool canCreatePediatric() const;
    bool canCreateAdult() const;
    bool canCreatePregnancy() const;
    QString sexHomeLabel() const;
    QString sexMullerLabel() const;
    bool weekStartsMonday() const; // first-day-of-week from the system locale

    // Reloads headers and language-dependent cells after a language change.
    Q_INVOKABLE void refreshLanguage();
    Q_INVOKABLE void setColumnFilter(int column, const QString& text);
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE void sortByColumn(int column); // toggles order on repeat
    Q_INVOKABLE void selectRow(int proxyRow);
    Q_INVOKABLE QString sexLabelText(int sex) const;
    Q_INVOKABLE void startAdd();
    Q_INVOKABLE void startEdit();
    Q_INVOKABLE void cancelEdit();
    // Gathers the draft like the Widgets window (trims text, clamps numbers,
    // coerces the birth date) and persists it. Returns a SaveResult code.
    Q_INVOKABLE int saveDraft(const QString& name, int sex, const QString& birthDate,
                              int ageRange, const QString& address, const QString& cohabitants,
                              const QString& contactPerson, int siblingCount);
    Q_INVOKABLE bool removeCurrent();
    Q_INVOKABLE void refreshAvailability();

    // History screens. Each call loads the history of the selected patient and
    // asks the view to open it: the controller only prepares the data, QML
    // shows the window (see openHistoryScreen).
    Q_INVOKABLE void openPediatricHistory();
    Q_INVOKABLE void openAdultHistory();
    Q_INVOKABLE void openPregnancyHistory();

signals:
    // The screen to open ("pediatric", "adult", "pregnancy") and the loaded
    // history controller, owned by this object.
    void openHistoryScreen(const QString& screen, gambasse::HistoryController* controller);
    void headersChanged();
    void currentChanged();
    void detailChanged();
    void availabilityChanged();
    void editingChanged();
    void sortChanged();

private:
    const Patient* selectedPatient() const; // proxy row -> source patient
    void refreshDetail(); // fields from selection, defaults or cleared
    void clampSelection(); // valid row after filtering, detail always refreshed
    void updateAvailability();
    void reloadAndReselect(qlonglong patientId);

    PatientsModel* m_model = nullptr;
    ColumnFilterProxy* m_proxy = nullptr;
    PatientService m_patientService;
    PhotoManager m_photoManager;
    ClinicalContext m_clinicalContext;

    bool m_editing = false;
    int m_editingSourceRow = -1; // source row, -1 = new patient
    Patient m_previous; // values before editing, for the photo rename
    int m_currentProxyRow = -1;

    Availability m_availability;
    int m_sortColumn = PatientsModel::ColumnName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    // Detail cache (view state).
    qlonglong m_patientId = 0;
    QString m_patientName;
    int m_patientSex = 0;
    QString m_birthDateText;
    int m_ageRange = 0;
    QString m_address;
    QString m_cohabitants;
    QString m_contactPerson;
    int m_siblingCount = 0;
    QString m_photoSource;
};

} // namespace gambasse
