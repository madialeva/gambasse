#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "Patient.h"

namespace gambasse {

// In-memory model containing the full patient list (low volume).
// Read-only at this stage.
class PatientsModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColumnId = 0,
        ColumnName,
        ColumnBirthDate,
        ColumnAgeRange,
        ColumnSex,
        ColumnAddress,
        ColumnCohabitants,
        ColumnContactPerson,
        ColumnCount
    };

    explicit PatientsModel(QObject* parent = nullptr);

    // Loads every patient from b01_paciente. Returns false on failure.
    bool load();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    const Patient* patientAt(int row) const;

    // CRUD operations synchronize the database and in-memory list.
    int  add(Patient p);                 // -1 on failure; otherwise, the new source row
    bool modify(int row, const Patient& p);
    bool remove(int row);

    // Refreshes header text after a language change.
    void refreshHeaders();

private:
    QVector<Patient> m_patients;
};

} // namespace gambasse
