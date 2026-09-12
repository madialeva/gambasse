#include "PatientsModel.h"

#include "Database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QtGlobal>

namespace gambasse {

PatientsModel::PatientsModel(QObject* parent)
    : QAbstractTableModel(parent) {}

bool PatientsModel::load() {
    beginResetModel();
    m_patients.clear();

    QSqlQuery q(Database::instance().connection());
    const QString sql = QStringLiteral(
        "SELECT b01_id, b01_nome, b01_datanascimento, b01_sexo, b01_anosaproximados, "
        "       b01_e_enderezo, b01_e_coabitantes, b01_e_pessoacontacto, b01_e_numero_irmaos "
        "FROM b01_paciente ORDER BY b01_nome");

    if (!q.exec(sql)) {
        endResetModel();
        return false;
    }

    while (q.next()) {
        Patient p;
        p.id              = q.value(0).toLongLong();
        p.name            = q.value(1).toString();
        p.birthDate      = q.value(2).toDate();
        p.sex            = q.value(3).toInt() == 1 ? Patient::Sex::Muller : Patient::Sex::Home;
        p.ageRange      = q.value(4).toInt();
        p.address        = q.value(5).toString();
        p.cohabitants     = q.value(6).toString();
        p.contactPerson  = q.value(7).toString();
        p.siblingCount    = q.value(8).toInt();
        m_patients.append(p);
    }

    endResetModel();
    qInfo("PatientsModel: %lld patients loaded", static_cast<long long>(m_patients.size()));
    return true;
}

int PatientsModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_patients.size();
}

int PatientsModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant PatientsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_patients.size())
        return {};
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
        return {};

    const Patient& p = m_patients.at(index.row());
    switch (index.column()) {
    case ColumnId:         return p.id;
    case ColumnName:       return p.name;
    case ColumnBirthDate: return p.isValidDate()
                                     ? p.birthDate.toString(QStringLiteral("dd/MM/yyyy"))
                                     : QString();
    case ColumnAgeRange:   return p.ageRange;
    case ColumnSex:        return p.sexToString();
    case ColumnAddress:    return p.address;
    case ColumnCohabitants:return p.cohabitants;
    case ColumnContactPerson: return p.contactPerson;
    default:                return {};
    }
}

QVariant PatientsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole)
        return {};
    if (orientation == Qt::Vertical)
        return section + 1;

    switch (section) {
    case ColumnId: return tr("Code");
    case ColumnName: return tr("Name");
    case ColumnBirthDate: return tr("Birth date");
    case ColumnAgeRange: return tr("Age");
    case ColumnSex: return tr("Sex");
    case ColumnAddress: return tr("Address");
    case ColumnCohabitants: return tr("Cohabitants");
    case ColumnContactPerson: return tr("Contact");
    default:                return {};
    }
}

const Patient* PatientsModel::patientAt(int row) const {
    if (row < 0 || row >= m_patients.size())
        return nullptr;
    return &m_patients.at(row);
}

int PatientsModel::add(Patient p) {
    if (!Database::instance().insert(p))
        return -1;
    const int row = m_patients.size();
    beginInsertRows(QModelIndex(), row, row);
    m_patients.append(p);
    endInsertRows();
    return row;
}

bool PatientsModel::modify(int row, const Patient& p) {
    if (row < 0 || row >= m_patients.size())
        return false;
    if (!Database::instance().update(p))
        return false;
    m_patients[row] = p;
    emit dataChanged(index(row, 0), index(row, ColumnCount - 1));
    return true;
}

bool PatientsModel::remove(int row) {
    if (row < 0 || row >= m_patients.size())
        return false;
    if (!Database::instance().remove(m_patients.at(row).id))
        return false;
    beginRemoveRows(QModelIndex(), row, row);
    m_patients.remove(row);
    endRemoveRows();
    return true;
}

void PatientsModel::refreshHeaders() {
    emit headerDataChanged(Qt::Horizontal, 0, ColumnCount - 1);
}

} // namespace gambasse
