#include <ui/PatientController.h>

#include <QDate>
#include <QDialog>
#include <QFileInfo>
#include <QLocale>
#include <QUrl>

#include <data/common/Database.h>
#include <logic/SexLabel.h>
#include <ui/window/AdultHistoryWindow.h>
#include <ui/window/PediatricHistoryWindow.h>
#include <ui/window/PregnancyHistoryWindow.h>

#include <UxWidgets/UxDateField.h>

namespace gambasse {

namespace {

// PhotoManager answers with a Qt resource path (":/img/foto0.png"), which the
// Widgets loaders take as is; QML needs a real URL.
QString defaultPhotoUrl(const PhotoManager& photos) {
    return QStringLiteral("qrc") + photos.defaultPhotoResource();
}

QString photoUrl(const PhotoManager& photos, const Patient* p) {
    if (p == nullptr)
        return defaultPhotoUrl(photos);
    const QString path = photos.photoPath(*p);
    if (QFileInfo::exists(path))
        return QUrl::fromLocalFile(path).toString();
    const QString legacy = photos.legacyPhotoPath(*p);
    if (QFileInfo::exists(legacy))
        return QUrl::fromLocalFile(legacy).toString();
    return defaultPhotoUrl(photos);
}

} // namespace

PatientController::PatientController(QObject* parent) : QObject(parent) {
    m_model = new PatientsModel(this);
    m_proxy = new ColumnFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setSortLocaleAware(true);
}

bool PatientController::load() {
    if (!m_model->load())
        return false;
    m_proxy->sort(m_sortColumn, m_sortOrder);
    emit sortChanged();
    selectRow(m_proxy->rowCount() > 0 ? 0 : -1);
    return true;
}

QAbstractItemModel* PatientController::gridModel() const {
    return m_proxy;
}

QStringList PatientController::columnNames() const {
    QStringList names;
    for (int c = 0; c < columnCount(); ++c)
        names.append(m_model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString());
    return names;
}

int PatientController::columnCount() const {
    return PatientsModel::ColumnCount;
}

int PatientController::sortColumn() const {
    return m_sortColumn;
}

int PatientController::sortOrder() const {
    return static_cast<int>(m_sortOrder);
}

int PatientController::currentRow() const {
    return m_currentProxyRow;
}

bool PatientController::hasSelection() const {
    return selectedPatient() != nullptr;
}

bool PatientController::editing() const {
    return m_editing;
}

qlonglong PatientController::patientId() const {
    return m_patientId;
}

QString PatientController::patientName() const {
    return m_patientName;
}

int PatientController::patientSex() const {
    return m_patientSex;
}

QString PatientController::birthDateText() const {
    return m_birthDateText;
}

int PatientController::ageRange() const {
    return m_ageRange;
}

QString PatientController::address() const {
    return m_address;
}

QString PatientController::cohabitants() const {
    return m_cohabitants;
}

QString PatientController::contactPerson() const {
    return m_contactPerson;
}

int PatientController::siblingCount() const {
    return m_siblingCount;
}

QString PatientController::photoSource() const {
    return m_photoSource;
}

bool PatientController::showPediatric() const {
    return m_availability.showPediatric;
}

bool PatientController::showAdult() const {
    return m_availability.showAdult;
}

bool PatientController::showPregnancy() const {
    return m_availability.showPregnancy;
}

bool PatientController::canCreatePediatric() const {
    return m_availability.canCreatePediatric;
}

bool PatientController::canCreateAdult() const {
    return m_availability.canCreateAdult;
}

bool PatientController::canCreatePregnancy() const {
    return m_availability.canCreatePregnancy;
}

QString PatientController::sexHomeLabel() const {
    return sexLabel(Patient::Sex::Home);
}

QString PatientController::sexMullerLabel() const {
    return sexLabel(Patient::Sex::Muller);
}

bool PatientController::weekStartsMonday() const {
    return QLocale().firstDayOfWeek() == Qt::Monday;
}

void PatientController::refreshLanguage() {
    m_model->refreshLanguage();
    emit headersChanged();
}

void PatientController::setColumnFilter(int column, const QString& text) {
    m_proxy->setColumnFilter(column, text);
    clampSelection();
}

void PatientController::clearFilters() {
    m_proxy->clearFilters();
    clampSelection();
}

// Keeps the selection pointing at a valid row after the filtered set changed.
// The row number can survive the change while the patient behind it does not,
// so the detail is refreshed in every case (like the Widgets selection model
// emitting selectionChanged again).
void PatientController::clampSelection() {
    if (m_proxy->rowCount() == 0) {
        selectRow(-1);
        return;
    }
    if (m_currentProxyRow < 0 || m_currentProxyRow >= m_proxy->rowCount()) {
        selectRow(0);
        return;
    }
    emit currentChanged();
    refreshDetail();
    refreshAvailability();
}

void PatientController::sortByColumn(int column) {
    if (column == m_sortColumn)
        m_sortOrder = (m_sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder
                                                          : Qt::AscendingOrder;
    else {
        m_sortColumn = column;
        m_sortOrder = Qt::AscendingOrder;
    }
    m_proxy->sort(m_sortColumn, m_sortOrder);
    emit sortChanged();
}

void PatientController::selectRow(int proxyRow) {
    if (m_editing)
        return; // patient selection does not change while editing
    if (proxyRow < 0 || proxyRow >= m_proxy->rowCount())
        proxyRow = -1;
    if (proxyRow == m_currentProxyRow && proxyRow >= 0)
        return;
    m_currentProxyRow = proxyRow;
    emit currentChanged();
    refreshDetail();
    refreshAvailability();
}

QString PatientController::sexLabelText(int sex) const {
    return sexLabel(sex == static_cast<int>(Patient::Sex::Muller) ? Patient::Sex::Muller
                                                                  : Patient::Sex::Home);
}

void PatientController::startAdd() {
    m_editingSourceRow = -1;
    m_editing = true;
    emit editingChanged();
    refreshDetail(); // defaults for a new patient
    // The Widgets window disables the history entries with a null patient
    // (updateContextButtons(nullptr)) while the grid row stays selected.
    m_availability = Availability();
    emit availabilityChanged();
}

void PatientController::startEdit() {
    const Patient* p = selectedPatient();
    if (!p)
        return;
    m_editingSourceRow = m_proxy->mapToSource(m_proxy->index(m_currentProxyRow, 0)).row();
    m_previous = *p;
    m_editing = true;
    emit editingChanged();
    refreshDetail();
}

void PatientController::cancelEdit() {
    m_editing = false;
    m_editingSourceRow = -1;
    emit editingChanged();
    refreshDetail();
    refreshAvailability();
}

int PatientController::saveDraft(const QString& name, int sex, const QString& birthDate,
                                 int ageRange, const QString& address,
                                 const QString& cohabitants, const QString& contactPerson,
                                 int siblingCount) {
    Patient p;
    p.name = name.trimmed();
    p.sex = (sex == static_cast<int>(Patient::Sex::Muller)) ? Patient::Sex::Muller
                                                            : Patient::Sex::Home;
    QDate birth = QDate::fromString(birthDate.trimmed(),
                                    QString::fromLatin1(UxDateField::kDateFormat));
    if (!birth.isValid() || birth.year() < 1900 || birth.year() > 2100)
        birth = QDate(1900, 1, 1);
    p.birthDate = birth;
    p.ageRange = qBound(0, ageRange, 130);
    p.address = address.trimmed();
    p.cohabitants = cohabitants.trimmed();
    p.contactPerson = contactPerson.trimmed();
    p.siblingCount = qBound(0, siblingCount, 99);

    const bool isNew = (m_editingSourceRow < 0);
    const PatientService::SaveResult result =
        isNew ? m_patientService.create(p) : m_patientService.update(m_previous, p);
    if (result != PatientService::SaveResult::Saved)
        return static_cast<int>(result);

    const qlonglong savedId = p.id;
    m_editing = false;
    m_editingSourceRow = -1;
    emit editingChanged();
    reloadAndReselect(savedId);
    return static_cast<int>(PatientService::SaveResult::Saved);
}

bool PatientController::removeCurrent() {
    const Patient* p = selectedPatient();
    if (!p)
        return false;
    const Patient toDelete = *p;
    if (!m_patientService.remove(toDelete))
        return false;
    reloadAndReselect(-1);
    return true;
}

void PatientController::refreshAvailability() {
    updateAvailability();
}

void PatientController::openPediatricHistory() {
    const Patient* p = selectedPatient();
    if (!p)
        return;
    PediatricHistoryWindow dialog(p->id, p->name);
    if (!dialog.isValid())
        return;
    if (dialog.exec() == QDialog::Accepted)
        refreshAvailability();
}

void PatientController::openAdultHistory() {
    const Patient* p = selectedPatient();
    if (!p)
        return;
    AdultHistoryWindow dialog(p->id, p->name);
    if (!dialog.isValid())
        return;
    if (dialog.exec() == QDialog::Accepted)
        refreshAvailability();
}

void PatientController::openPregnancyHistory() {
    const Patient* p = selectedPatient();
    if (!p)
        return;
    PregnancyHistoryWindow dialog(p->id, p->name);
    if (!dialog.isValid())
        return;
    if (dialog.exec() == QDialog::Accepted)
        refreshAvailability();
}

const Patient* PatientController::selectedPatient() const {
    if (m_currentProxyRow < 0 || m_currentProxyRow >= m_proxy->rowCount())
        return nullptr;
    return m_model->patientAt(m_proxy->mapToSource(m_proxy->index(m_currentProxyRow, 0)).row());
}

void PatientController::refreshDetail() {
    // New patients get empty defaults; otherwise the fields reflect the
    // patient under edit or, in view mode, the selected patient.
    const Patient* p = nullptr;
    if (m_editing) {
        if (m_editingSourceRow >= 0)
            p = m_model->patientAt(m_editingSourceRow);
    } else {
        p = selectedPatient();
    }

    if (!p) {
        m_patientId = m_editing ? m_patientService.nextId() : 0;
        m_patientName.clear();
        m_patientSex = static_cast<int>(Patient::Sex::Muller);
        m_birthDateText =
            QDate(1900, 1, 1).toString(QString::fromLatin1(UxDateField::kDateFormat));
        m_ageRange = 0;
        m_address.clear();
        m_cohabitants.clear();
        m_contactPerson.clear();
        m_siblingCount = 0;
        m_photoSource = photoUrl(m_photoManager, nullptr);
    } else {
        m_patientId = p->id;
        m_patientName = p->name;
        m_patientSex = static_cast<int>(p->sex);
        const QDate birth = p->birthDate.isValid() ? p->birthDate : QDate(1900, 1, 1);
        m_birthDateText = birth.toString(QString::fromLatin1(UxDateField::kDateFormat));
        m_ageRange = p->ageRange;
        m_address = p->address;
        m_cohabitants = p->cohabitants;
        m_contactPerson = p->contactPerson;
        m_siblingCount = p->siblingCount;
        m_photoSource = photoUrl(m_photoManager, p);
    }
    emit detailChanged();
}

void PatientController::updateAvailability() {
    m_availability = m_clinicalContext.availability(selectedPatient());
    emit availabilityChanged();
}

void PatientController::reloadAndReselect(qlonglong patientId) {
    m_model->load();
    int sourceRow = -1;
    if (patientId > 0) {
        for (int row = 0; row < m_model->rowCount(); ++row) {
            const Patient* p = m_model->patientAt(row);
            if (p && p->id == patientId) {
                sourceRow = row;
                break;
            }
        }
    }
    m_currentProxyRow = -1; // force refresh even when the row number repeats
    if (sourceRow >= 0) {
        const QModelIndex proxyIdx = m_proxy->mapFromSource(m_model->index(sourceRow, 0));
        if (proxyIdx.isValid()) {
            m_currentProxyRow = proxyIdx.row();
            emit currentChanged();
            refreshDetail();
            refreshAvailability();
            return;
        }
    }
    if (m_proxy->rowCount() > 0) {
        m_currentProxyRow = 0;
        emit currentChanged();
        refreshDetail();
        refreshAvailability();
    } else {
        emit currentChanged();
        refreshDetail();
        refreshAvailability();
    }
}

} // namespace gambasse
