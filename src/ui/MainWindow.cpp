#include "MainWindow.h"

#include "../Paths.h"
#include "data/Database.h"
#include "data/PatientsModel.h"
#include "filter/ColumnFilterProxy.h"
#include "ui/TitleBar.h"

#include <QApplication>
#include <QAbstractButton>
#include <QAction>
#include <QActionGroup>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateEdit>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QMouseEvent>
#include <QSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSizePolicy>
#include <QSplitter>
#include <QStyle>
#include <QStyleFactory>
#include <QTableView>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QValidator>
#include <QVBoxLayout>

namespace gambasse {

namespace {

// Resize border width (pixels) for the frameless window.
constexpr int kResizeMargin = 6;

// Resize edge flags.
enum ResizeEdge { NoEdge = 0, EdgeLeft = 1, EdgeTop = 2, EdgeRight = 4, EdgeBottom = 8 };

// Detail panel field indexes.
enum Field { FieldName = 0, FieldSex, FieldDate, FieldAge, FieldAddress, FieldCohabitants, FieldContact, FieldSiblings, FieldCount };

// Converts input to upper case to prevent duplicates differing only in case.
class UppercaseValidator : public QValidator {
public:
    using QValidator::QValidator;
    State validate(QString& s, int& /*pos*/) const override {
        s = s.toUpper();
        return Acceptable;
    }
};
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();

    // Initial language and theme (config.ini; Portuguese and light by default).
    QSettings cfg(QDir(basePath()).filePath(QStringLiteral("config.ini")),
                  QSettings::IniFormat);
    const QString code = cfg.value(QStringLiteral("idioma/codigo"), QStringLiteral("pt")).toString();
    const QString theme = cfg.value(QStringLiteral("tema/modo"), QStringLiteral("claro")).toString();
    changeTheme(theme);
    changeLanguage(code);

    // Once visible, position the filter row and select the first patient so the
    // detail and contextual buttons are populated.
    QTimer::singleShot(0, this, [this]() {
        positionFilters();
        if (m_proxy && m_proxy->rowCount() > 0)
            m_table->selectRow(0);
        m_table->setFocus();
    });
}

void MainWindow::buildUi() {
    // Frameless window: the OS title bar is replaced by the custom TitleBar.
    // Keep the Qt::Window type so the taskbar entry is preserved.
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setMinimumSize(960, 540);
    resize(1008, 561); // base size for the oldest supported laptops

    // Custom title bar first: logo, name, language, theme, window controls.
    // It must exist before buildToolbar(), which moves the language and
    // theme buttons into it.
    m_titleBar = new TitleBar();

    // Create the top toolbar, including history/consultation entry buttons.
    buildToolbar();

    auto* central = new QWidget(this);
    auto* v = new QVBoxLayout(central);
    v->setContentsMargins(6, 6, 6, 6);

    v->addWidget(m_titleBar);

    // Manual edge resizing (lost with the native frame) is handled through
    // an event filter on the title bar and the central widget.
    m_titleBar->installEventFilter(this);
    central->installEventFilter(this);

    // History-creation buttons, located below the photo in the right column.
    m_createPediatricButton = new QPushButton(central);
    m_createAdultButton = new QPushButton(central);
    m_createPregnancyButton = new QPushButton(central);
    for (QPushButton* b : {m_createPediatricButton, m_createAdultButton, m_createPregnancyButton})
        connect(b, &QPushButton::clicked, this, &MainWindow::onActionNotImplemented);

    // --- Top row: grid (left) and photo/create-buttons column (right). ---
    m_photo = new QLabel(central);
    m_photo->setFixedSize(200, 170);
    m_photo->setFrameShape(QFrame::Box);
    m_photo->setAlignment(Qt::AlignCenter);
    m_photo->setScaledContents(false);

    auto* rightColumn = new QVBoxLayout();
    rightColumn->addWidget(m_photo, 0, Qt::AlignHCenter | Qt::AlignTop);
    rightColumn->addWidget(m_createPediatricButton);
    rightColumn->addWidget(m_createAdultButton);
    rightColumn->addWidget(m_createPregnancyButton);
    rightColumn->addStretch(1);

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(buildListPanel(), 1);
    topRow->addLayout(rightColumn, 0);
    v->addLayout(topRow, 1);

    // --- Campos editables del detalle ---
    m_valueLabels.resize(FieldCount);
    for (int i = 0; i < FieldCount; ++i)
        m_valueLabels[i] = new QLabel(central);

    // Identifier: always read-only (a QLabel, excluded from edit toggling).
    m_codeCaption = new QLabel(central);
    m_codeValue = new QLabel(central);
    m_codeValue->setTextInteractionFlags(Qt::TextSelectableByMouse);

    // Field limits mirror b01_paciente columns.
    constexpr int kMaxName        = 200;  // b01_nome
    constexpr int kMaxAddress     = 100;  // b01_e_enderezo
    constexpr int kMaxCohabitants = 100;  // b01_e_coabitantes
    constexpr int kMaxContact     = 100;  // b01_e_pessoacontacto

    m_nameEdit = new QLineEdit(central);
    m_nameEdit->setValidator(new UppercaseValidator(m_nameEdit));  // enforce upper case
    m_nameEdit->setMaxLength(kMaxName);
    m_sexCombo = new QComboBox(central);
    // Persisted domain terms (Galician/Portuguese), matching the Sex column.
    m_sexCombo->addItem(QStringLiteral("Home"), static_cast<int>(Patient::Sex::Home));
    m_sexCombo->addItem(QStringLiteral("Muller"), static_cast<int>(Patient::Sex::Muller));
    m_dateEdit = new QDateEdit(central);
    m_dateEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDateRange(QDate(1900, 1, 1), QDate(2100, 12, 31));
    m_ageSpinBox = new QSpinBox(central);
    m_ageSpinBox->setRange(0, 130);
    m_addressEdit = new QLineEdit(central);
    m_addressEdit->setMaxLength(kMaxAddress);
    m_cohabitantsEdit = new QLineEdit(central);
    m_cohabitantsEdit->setMaxLength(kMaxCohabitants);
    m_contactEdit = new QLineEdit(central);
    m_contactEdit->setMaxLength(kMaxContact);
    m_siblingsSpinBox = new QSpinBox(central);
    m_siblingsSpinBox->setRange(0, 99);

    // --- Patient CRUD bar above "Basic data". ---
    m_addButton    = new QPushButton(central);
    m_editButton = new QPushButton(central);
    m_deleteButton    = new QPushButton(central);
    m_saveButton    = new QPushButton(central);
    m_cancelButton  = new QPushButton(central);
    connect(m_addButton,    &QPushButton::clicked, this, &MainWindow::onAdd);
    connect(m_editButton, &QPushButton::clicked, this, &MainWindow::onEdit);
    connect(m_deleteButton,    &QPushButton::clicked, this, &MainWindow::onDelete);
    connect(m_saveButton,    &QPushButton::clicked, this, &MainWindow::onSave);
    connect(m_cancelButton,  &QPushButton::clicked, this, &MainWindow::onCancel);
    auto* patientBar = new QHBoxLayout();
    for (QPushButton* b : {m_addButton, m_editButton, m_deleteButton, m_saveButton, m_cancelButton})
        patientBar->addWidget(b);
    patientBar->addStretch(1);
    v->addLayout(patientBar);

    // --- Detail row: "Basic data" (left) and "Address" (right). ---
    m_dataGroup = new QGroupBox(central);
    auto* dataForm = new QFormLayout(m_dataGroup);
    // First row: read-only identifier (left) and name (remaining space).
    m_codeValue->setMinimumWidth(45);
    auto* codeAndNameRow = new QHBoxLayout();
    codeAndNameRow->addWidget(m_codeValue);
    codeAndNameRow->addSpacing(12);
    codeAndNameRow->addWidget(m_valueLabels[FieldName]);
    codeAndNameRow->addWidget(m_nameEdit, 1);
    dataForm->addRow(m_codeCaption, codeAndNameRow);
    dataForm->addRow(m_valueLabels[FieldSex],   m_sexCombo);
    dataForm->addRow(m_valueLabels[FieldDate],  m_dateEdit);
    dataForm->addRow(m_valueLabels[FieldAge],   m_ageSpinBox);

    m_addressGroup = new QGroupBox(central);
    auto* addressForm = new QFormLayout(m_addressGroup);
    addressForm->addRow(m_valueLabels[FieldAddress], m_addressEdit);
    addressForm->addRow(m_valueLabels[FieldCohabitants], m_cohabitantsEdit);
    addressForm->addRow(m_valueLabels[FieldContact], m_contactEdit);
    addressForm->addRow(m_valueLabels[FieldSiblings], m_siblingsSpinBox);

    auto* dataLayout = new QHBoxLayout();
    dataLayout->addWidget(m_dataGroup, 1);
    dataLayout->addWidget(m_addressGroup, 1);
    v->addLayout(dataLayout);

    setCentralWidget(central);
    setEditMode(false);   // starts in view mode
    clearDetail();
}

void MainWindow::buildToolbar() {
    auto* toolbar = addToolBar(QStringLiteral("toolbar"));
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(28, 18));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // Text-only history/consultation entry buttons at the left. Retain the
    // toolbar QAction so their visibility can be changed reliably.
    auto createActionButton = [this, toolbar](QAction*& action) {
        auto* b = new QToolButton(toolbar);
        b->setToolButtonStyle(Qt::ToolButtonTextOnly);
        connect(b, &QToolButton::clicked, this, &MainWindow::onActionNotImplemented);
        action = toolbar->addWidget(b);
        return b;
    };
    m_pediatricHistoryButton = createActionButton(m_pediatricHistoryAction);
    m_pediatricConsultationButton = createActionButton(m_pediatricConsultationAction);
    m_adultHistoryButton = createActionButton(m_adultHistoryAction);
    m_adultConsultationButton = createActionButton(m_adultConsultationAction);
    m_pregnancyHistoryButton = createActionButton(m_pregnancyHistoryAction);
    m_pregnancyConsultationButton = createActionButton(m_pregnancyConsultationAction);

    // Language and theme live in the custom title bar now (same buttons,
    // menus and persistence as before).
    // Language button: flag and name, with a language menu.
    m_languageButton = new QToolButton(m_titleBar);
    m_languageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_languageButton->setPopupMode(QToolButton::InstantPopup);
    auto* menuIdioma = new QMenu(m_languageButton);
    auto* grpIdioma = new QActionGroup(this);
    grpIdioma->setExclusive(true);
    m_actEs = menuIdioma->addAction(QIcon(QStringLiteral(":/img/flag-es.png")), QString());
    m_actPt = menuIdioma->addAction(QIcon(QStringLiteral(":/img/flag-pt.png")), QString());
    for (QAction* a : {m_actEs, m_actPt}) {
        a->setCheckable(true);
        grpIdioma->addAction(a);
    }
    connect(m_actEs, &QAction::triggered, this, [this]() { changeLanguage(QStringLiteral("es")); });
    connect(m_actPt, &QAction::triggered, this, [this]() { changeLanguage(QStringLiteral("pt")); });
    m_languageButton->setMenu(menuIdioma);
    m_titleBar->insertControl(m_languageButton);

    // Theme button: sun/moon and name, with a light/dark menu.
    m_themeButton = new QToolButton(m_titleBar);
    m_themeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_themeButton->setPopupMode(QToolButton::InstantPopup);
    auto* menuTema = new QMenu(m_themeButton);
    auto* grpTema = new QActionGroup(this);
    grpTema->setExclusive(true);
    m_lightAction = menuTema->addAction(QIcon(QStringLiteral(":/img/sun.svg")), QString());
    m_darkAction = menuTema->addAction(QIcon(QStringLiteral(":/img/moon.svg")), QString());
    for (QAction* a : {m_lightAction, m_darkAction}) {
        a->setCheckable(true);
        grpTema->addAction(a);
    }
    connect(m_lightAction, &QAction::triggered, this, [this]() { changeTheme(QStringLiteral("claro")); });
    connect(m_darkAction, &QAction::triggered, this, [this]() { changeTheme(QStringLiteral("oscuro")); });
    m_themeButton->setMenu(menuTema);
    m_titleBar->insertControl(m_themeButton);
}

QWidget* MainWindow::buildListPanel() {
    auto* panel = new QWidget(this);
    auto* lay = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->setSpacing(2);

    // Model and filter proxy.
    m_model = new PatientsModel(this);
    m_model->load();
    m_proxy = new ColumnFilterProxy(this);
    m_proxy->setSourceModel(m_model);

    m_table = new QTableView(panel);
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSortingEnabled(true);

    // Ascending alphabetical order by name at startup, case-insensitive and
    // locale-aware for accents.
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setSortLocaleAware(true);
    m_table->sortByColumn(PatientsModel::ColumnName, Qt::AscendingOrder);

    // Reasonable initial widths; the identifier column is narrow.
    const int anchos[PatientsModel::ColumnCount] = {60, 170, 95, 55, 75, 170, 120, 120};
    for (int c = 0; c < PatientsModel::ColumnCount; ++c)
        m_table->setColumnWidth(c, anchos[c]);

    // Filter row: one QLineEdit per column, placed over the header.
    m_filterRow = new QWidget(panel);
    m_filterRow->setFixedHeight(26);
    for (int c = 0; c < PatientsModel::ColumnCount; ++c) {
        auto* edit = new QLineEdit(m_filterRow);
        edit->setClearButtonEnabled(true);
        connect(edit, &QLineEdit::textChanged, this, [this, c](const QString& t) {
            m_proxy->setColumnFilter(c, t);
        });
        m_filters.append(edit);
    }

    lay->addWidget(m_filterRow);
    lay->addWidget(m_table, 1);

    // Reposition the filter row when columns or scrolling change.
    QHeaderView* h = m_table->horizontalHeader();
    connect(h, &QHeaderView::sectionResized, this, [this]() { positionFilters(); });
    connect(h, &QHeaderView::geometriesChanged, this, [this]() { positionFilters(); });
    connect(m_table->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this]() { positionFilters(); });
    m_table->viewport()->installEventFilter(this);

    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &MainWindow::onSelectionChanged);

    return panel;
}

void MainWindow::positionFilters() {
    if (!m_table || m_filters.isEmpty())
        return;
    QHeaderView* h = m_table->horizontalHeader();
    const int left = m_table->frameWidth();
    const int alto = m_filterRow->height();
    const int anchoRow = m_filterRow->width();

    for (int c = 0; c < m_filters.size(); ++c) {
        QLineEdit* e = m_filters[c];
        if (m_table->isColumnHidden(c) || h->sectionSize(c) == 0) {
            e->hide();
            continue;
        }
        const int x = left + h->sectionViewportPosition(c);
        const int w = h->sectionSize(c);
        e->setGeometry(x, 0, w, alto);
        // Visible only when at least partially within the row.
        e->setVisible(x + w > left && x < anchoRow);
    }
}

int MainWindow::selectedSourceRow() const {
    const QModelIndex idx = m_table->selectionModel()->currentIndex();
    return idx.isValid() ? m_proxy->mapToSource(idx).row() : -1;
}

const Patient* MainWindow::selectedPatient() const {
    const int f = selectedSourceRow();
    return f >= 0 ? m_model->patientAt(f) : nullptr;
}

void MainWindow::onSelectionChanged() {
    if (m_isEditing)
        return; // patient selection does not change while editing
    const Patient* p = selectedPatient();
    loadIntoFields(p);
    updateContextButtons(p);
    loadPhoto(p);
}

void MainWindow::loadIntoFields(const Patient* p) {
    if (!p) {
        m_codeValue->clear();
        m_nameEdit->clear();
        m_sexCombo->setCurrentIndex(m_sexCombo->findData(static_cast<int>(Patient::Sex::Muller)));
        m_dateEdit->setDate(QDate(1900, 1, 1));
        m_ageSpinBox->setValue(0);
        m_addressEdit->clear();
        m_cohabitantsEdit->clear();
        m_contactEdit->clear();
        m_siblingsSpinBox->setValue(0);
        return;
    }
    m_codeValue->setText(QString::number(p->id));
    m_nameEdit->setText(p->name);
    m_sexCombo->setCurrentIndex(m_sexCombo->findData(static_cast<int>(p->sex)));
    m_dateEdit->setDate(p->birthDate.isValid() ? p->birthDate : QDate(1900, 1, 1));
    m_ageSpinBox->setValue(p->ageRange);
    m_addressEdit->setText(p->address);
    m_cohabitantsEdit->setText(p->cohabitants);
    m_contactEdit->setText(p->contactPerson);
    m_siblingsSpinBox->setValue(p->siblingCount);
}

void MainWindow::gatherFromFields(Patient& p) const {
    p.name = m_nameEdit->text().trimmed();
    p.sex = static_cast<Patient::Sex>(m_sexCombo->currentData().toInt());
    p.birthDate = m_dateEdit->date();
    p.ageRange = m_ageSpinBox->value();
    p.address = m_addressEdit->text().trimmed();
    p.cohabitants = m_cohabitantsEdit->text().trimmed();
    p.contactPerson = m_contactEdit->text().trimmed();
    p.siblingCount = m_siblingsSpinBox->value();
}

void MainWindow::setEditMode(bool editing) {
    m_isEditing = editing;
    for (QWidget* w : {static_cast<QWidget*>(m_nameEdit), static_cast<QWidget*>(m_sexCombo),
                       static_cast<QWidget*>(m_dateEdit), static_cast<QWidget*>(m_ageSpinBox),
                       static_cast<QWidget*>(m_addressEdit), static_cast<QWidget*>(m_cohabitantsEdit),
                       static_cast<QWidget*>(m_contactEdit), static_cast<QWidget*>(m_siblingsSpinBox)})
        if (w) w->setEnabled(editing);

    if (m_addButton)    m_addButton->setEnabled(!editing);
    if (m_editButton) m_editButton->setEnabled(!editing);
    if (m_deleteButton)    m_deleteButton->setEnabled(!editing);
    if (m_saveButton)    m_saveButton->setEnabled(editing);
    if (m_cancelButton)  m_cancelButton->setEnabled(editing);

    // Editing locks the grid and filter row.
    if (m_table)     m_table->setEnabled(!editing);
    if (m_filterRow) m_filterRow->setEnabled(!editing);
}

void MainWindow::clearDetail() {
    loadIntoFields(nullptr);
    if (m_photo)
        m_photo->setPixmap(QPixmap(QStringLiteral(":/img/foto0.png"))
                              .scaled(m_photo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    updateContextButtons(nullptr);
}

// --- CRUD ---

void MainWindow::onAdd() {
    m_editingRow = -1;            // new patient
    loadIntoFields(nullptr);      // empty fields, 1900 date, Muller sex
    // Display the calculated identifier (read-only).
    m_codeValue->setText(QString::number(Database::instance().nextId()));
    updateContextButtons(nullptr);
    loadPhoto(nullptr);
    setEditMode(true);
    m_nameEdit->setFocus();
}

void MainWindow::onEdit() {
    const int f = selectedSourceRow();
    const Patient* p = (f >= 0) ? m_model->patientAt(f) : nullptr;
    if (!p)
        return;
    m_editingRow = f;
    m_previousPatient = *p;       // used to rename the photo if its name changes
    loadIntoFields(p);
    setEditMode(true);
    m_nameEdit->setFocus();
}

void MainWindow::onCancel() {
    setEditMode(false);
    const Patient* p = selectedPatient();
    loadIntoFields(p);
    loadPhoto(p);
    updateContextButtons(p);
    m_editingRow = -1;
}

void MainWindow::onSave() {
    Patient p;
    gatherFromFields(p);
    if (p.name.isEmpty()) {
        QMessageBox::warning(this, tr("Incomplete data"), tr("Name is required."));
        m_nameEdit->setFocus();
        return;
    }

    Database& db = Database::instance();
    const bool isNew = (m_editingRow < 0);
    const qlonglong exceptId = isNew ? -1 : m_previousPatient.id;
    if (db.hasDuplicate(p, exceptId)) {
        QMessageBox::warning(this, tr("Duplicate patient"),
            tr("A patient with the same name, birth date, sex, and age already exists."));
        return;
    }

    int sourceRow = -1;
    if (isNew) {
        sourceRow = m_model->add(p);   // assigns p.id internally
        if (sourceRow < 0) {
            QMessageBox::critical(this, tr("Error"), tr("Could not save the patient."));
            return;
        }
    } else {
        p.id = m_previousPatient.id;
        if (!m_model->modify(m_editingRow, p)) {
            QMessageBox::critical(this, tr("Error"), tr("Could not save the patient."));
            return;
        }
        sourceRow = m_editingRow;
        // Rename the photo if any filename component has changed.
        const QString oldFilename = m_previousPatient.photoFilename();
        const QString newFilename = p.photoFilename();
        if (oldFilename != newFilename) {
            const QString oldPath = QDir(basePath()).filePath(oldFilename);
            const QString newPath = QDir(basePath()).filePath(newFilename);
            if (QFileInfo::exists(oldPath))
                QFile::rename(oldPath, newPath);
        }
    }

    m_editingRow = -1;
    setEditMode(false);

    // Select the affected row after mapping source to proxy following reordering.
    const QModelIndex proxyIdx = m_proxy->mapFromSource(m_model->index(sourceRow, 0));
    if (proxyIdx.isValid()) {
        m_table->selectRow(proxyIdx.row());
        m_table->scrollTo(proxyIdx);
    }
    const Patient* sel = selectedPatient();
    loadIntoFields(sel);
    loadPhoto(sel);
    updateContextButtons(sel);
}

void MainWindow::onDelete() {
    const int f = selectedSourceRow();
    const Patient* p = (f >= 0) ? m_model->patientAt(f) : nullptr;
    if (!p)
        return;

        if (QMessageBox::question(this, tr("Delete patient"),
            tr("Delete patient \"%1\" and all their histories and consultations?").arg(p->name))
        != QMessageBox::Yes)
        return;

    const QString photoPath = QDir(basePath()).filePath(p->photoFilename());
    const int previousProxyRow = m_table->selectionModel()->currentIndex().row();

    if (!m_model->remove(f)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not delete the patient."));
        return;
    }
    if (QFileInfo::exists(photoPath))
        QFile::remove(photoPath);

    if (m_proxy->rowCount() > 0) {
        m_table->selectRow(qBound(0, previousProxyRow, m_proxy->rowCount() - 1));
    } else {
        clearDetail();
    }
}

void MainWindow::updateContextButtons(const Patient* p) {
    // The three Create buttons are always visible; only their enabled state
    // changes. Entry buttons in the toolbar use their QAction for visibility.
    if (!p) {
        for (QAction* a : {m_pediatricHistoryAction, m_pediatricConsultationAction, m_adultHistoryAction, m_adultConsultationAction,
                           m_pregnancyHistoryAction, m_pregnancyConsultationAction})
            if (a) a->setVisible(false);
        for (QPushButton* b : {m_createPediatricButton, m_createAdultButton, m_createPregnancyButton}) {
            b->setVisible(true);
            b->setEnabled(false);
        }
        return;
    }

    Database& db = Database::instance();
    const bool hasPediatric = db.hasPediatricHistory(p->id);
    const bool hasAdult = db.hasAdultHistory(p->id);
    const bool hasPregnancy = db.hasPregnancyHistory(p->id);

    const bool isChild = (p->ageRange <= 16) || (p->isValidDate() && p->ageInYears() <= 16);
    const bool isAdult = (p->ageRange > 16)  || (p->isValidDate() && p->ageInYears() > 16);
    const bool isPregnant = (p->sex == Patient::Sex::Muller)
                           && ((p->ageRange >= 9) || (p->isValidDate() && p->ageInYears() >= 9));

    // Toolbar entry controls are visible only when the history already exists.
    m_pediatricHistoryAction->setVisible(hasPediatric);  m_pediatricConsultationAction->setVisible(hasPediatric);
    m_adultHistoryAction->setVisible(hasAdult);      m_adultConsultationAction->setVisible(hasAdult);
    m_pregnancyHistoryAction->setVisible(hasPregnancy);  m_pregnancyConsultationAction->setVisible(hasPregnancy);

    // History creation below the photo is always visible, but only enabled when
    // the history does not exist and the patient meets the age/sex criteria.
    m_createPediatricButton->setVisible(true); m_createPediatricButton->setEnabled(!hasPediatric && isChild);
    m_createAdultButton->setVisible(true); m_createAdultButton->setEnabled(!hasAdult && isAdult);
    m_createPregnancyButton->setVisible(true); m_createPregnancyButton->setEnabled(!hasPregnancy && isPregnant);
}

void MainWindow::loadPhoto(const Patient* p) {
    if (!m_photo)
        return;
    if (!p) {
        m_photo->setPixmap(QPixmap(QStringLiteral(":/img/foto0.png"))
                              .scaled(m_photo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        return;
    }
    const QString applicationDirectory = basePath();
    QPixmap pm;

    const QString photoFilenamePath = QDir(applicationDirectory).filePath(p->photoFilename());
    const QString legacyPhotoPath = QDir(applicationDirectory).filePath(QStringLiteral("fotos/%1.jpg").arg(p->id));

    if (QFileInfo::exists(photoFilenamePath))
        pm.load(photoFilenamePath);
    else if (QFileInfo::exists(legacyPhotoPath))
        pm.load(legacyPhotoPath);

    if (pm.isNull())
        pm = QPixmap(QStringLiteral(":/img/foto0.png"));

    m_photo->setPixmap(pm.scaled(m_photo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onActionNotImplemented() {
    QMessageBox::information(
        this, tr("Unavailable"),
        tr("This feature is not implemented in this phase yet."));
}

// --------- i18n ---------

void MainWindow::changeLanguage(const QString& code) {
    m_languageCode = code;
    qApp->removeTranslator(&m_translator);
    if (code != QLatin1String("en")) { // English is the code's source language.
        bool ok = m_translator.load(QStringLiteral("gambasse_") + code, QStringLiteral(":/i18n"));
        if (!ok)
            ok = m_translator.load(QStringLiteral("gambasse_") + code,
                                  QDir(basePath())
                                      .filePath(QStringLiteral("translations")));
        if (ok)
            qApp->installTranslator(&m_translator);
    }

    QSettings cfg(QDir(basePath()).filePath(QStringLiteral("config.ini")),
                  QSettings::IniFormat);
    cfg.setValue(QStringLiteral("idioma/codigo"), code);

    if (m_actEs) m_actEs->setChecked(code == QLatin1String("es"));
    if (m_actPt) m_actPt->setChecked(code == QLatin1String("pt"));

    retranslate(); // refuerzo por si installTranslator no dispara LanguageChange
}

void MainWindow::updateLanguageButton() {
    if (!m_languageButton)
        return;
    const bool es = (m_languageCode == QLatin1String("es"));
    m_languageButton->setIcon(QIcon(es ? QStringLiteral(":/img/flag-es.png")
                                  : QStringLiteral(":/img/flag-pt.png")));
    m_languageButton->setText(es ? tr("Spanish") : tr("Portuguese"));
}

void MainWindow::changeTheme(const QString& mode) {
    m_theme = (mode == QLatin1String("oscuro")) ? QStringLiteral("oscuro") : QStringLiteral("claro");
    applyTheme(m_theme);

    QSettings cfg(QDir(basePath()).filePath(QStringLiteral("config.ini")),
                  QSettings::IniFormat);
    cfg.setValue(QStringLiteral("tema/modo"), m_theme);

    if (m_lightAction) m_lightAction->setChecked(m_theme == QLatin1String("claro"));
    if (m_darkAction) m_darkAction->setChecked(m_theme == QLatin1String("oscuro"));
    updateThemeButton();
    styleToolbarButtons();
}

void MainWindow::styleToolbarButtons() {
    const bool dark = (m_theme == QLatin1String("oscuro"));
    const QString base  = dark ? QStringLiteral("#27496D") : QStringLiteral("#CFE2F3");
    const QString hover = dark ? QStringLiteral("#2F5A85") : QStringLiteral("#BBD6EE");
    const QString press = dark ? QStringLiteral("#1B3550") : QStringLiteral("#A9C9E8");
    const QString border = dark ? QStringLiteral("#1B3550") : QStringLiteral("#9FC3E8");
    const QString text = dark ? QStringLiteral("white")   : QStringLiteral("#1A1A1A");
    const QString disabledBackground = dark ? QStringLiteral("#3A3A3A") : QStringLiteral("#E4E3DF");
    const QString disabledText = dark ? QStringLiteral("#7A7A7A") : QStringLiteral("#A6A6A6");
    const QString disabledBorder = dark ? QStringLiteral("#333333") : QStringLiteral("#CFCFCF");

    // Build a style sheet for QToolButton or QPushButton, with bold text and a
    // muted disabled state.
    auto rule = [&](const QString& selector) {
        return QStringLiteral("%1 { background:%2; color:%3; border:1px solid %4;"
                              " border-radius:4px; padding:4px 10px; font-weight:bold; }"
                              " %1:hover:enabled { background:%5; }"
                              " %1:pressed:enabled { background:%6; }"
                              " %1:disabled { background:%7; color:%8; border-color:%9; }")
            .arg(selector, base, text, border, hover, press, disabledBackground, disabledText, disabledBorder);
    };

    const QString toolButtonStyle = rule(QStringLiteral("QToolButton"));
    const QString pushButtonStyle = rule(QStringLiteral("QPushButton"));

    for (QToolButton* b : {m_pediatricHistoryButton, m_pediatricConsultationButton, m_adultHistoryButton, m_adultConsultationButton,
                           m_pregnancyHistoryButton, m_pregnancyConsultationButton})
        if (b) b->setStyleSheet(toolButtonStyle);
    for (QPushButton* b : {m_createPediatricButton, m_createAdultButton, m_createPregnancyButton,
                           m_addButton, m_editButton, m_deleteButton, m_saveButton, m_cancelButton})
        if (b) b->setStyleSheet(pushButtonStyle);

    // Custom title-bar window controls: flat buttons with a subtle hover and
    // a red close on hover, following the active theme.
    if (m_titleBar) {
        const QString windowBase =
            QStringLiteral("QToolButton { background:transparent; border:none;"
                           " padding:4px 8px; font-weight:bold; }"
                           " QToolButton:hover:enabled { background:%1; }"
                           " QToolButton:pressed:enabled { background:%2; }")
                .arg(hover, press);
        const QString closeStyle =
            QStringLiteral("QToolButton { background:transparent; border:none;"
                           " padding:4px 8px; font-weight:bold; }"
                           " QToolButton:hover:enabled { background:#E81123; color:white; }"
                           " QToolButton:pressed:enabled { background:#A00A18; color:white; }");
        for (QToolButton* b : m_titleBar->windowButtons()) {
            if (b)
                b->setStyleSheet(b == m_titleBar->closeButton() ? closeStyle : windowBase);
        }
    }
}

void MainWindow::applyTheme(const QString& mode) {
    if (mode == QLatin1String("oscuro")) {
        QPalette p;
        p.setColor(QPalette::Window, QColor(53, 53, 53));
        p.setColor(QPalette::WindowText, Qt::white);
        p.setColor(QPalette::Base, QColor(35, 35, 35));
        p.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        p.setColor(QPalette::ToolTipBase, QColor(25, 25, 25));
        p.setColor(QPalette::ToolTipText, Qt::white);
        p.setColor(QPalette::Text, Qt::white);
        p.setColor(QPalette::Button, QColor(53, 53, 53));
        p.setColor(QPalette::ButtonText, Qt::white);
        p.setColor(QPalette::BrightText, Qt::red);
        p.setColor(QPalette::Link, QColor(42, 130, 218));
        p.setColor(QPalette::Highlight, QColor(42, 130, 218));
        p.setColor(QPalette::HighlightedText, Qt::black);
        p.setColor(QPalette::PlaceholderText, QColor(150, 150, 150));
        p.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
        p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
        qApp->setPalette(p);
    } else {
        // Light theme: warm off-white background and black text.
        const QColor offWhite(243, 242, 238);      // general background
        const QColor nearWhite(251, 250, 246);     // editable/grid areas
        const QColor alternate(236, 234, 227);     // alternate rows/buttons
        const QColor textBlack(26, 26, 26);
        QPalette p;
        p.setColor(QPalette::Window, offWhite);
        p.setColor(QPalette::WindowText, textBlack);
        p.setColor(QPalette::Base, nearWhite);
        p.setColor(QPalette::AlternateBase, alternate);
        p.setColor(QPalette::ToolTipBase, nearWhite);
        p.setColor(QPalette::ToolTipText, textBlack);
        p.setColor(QPalette::Text, textBlack);
        p.setColor(QPalette::Button, alternate);
        p.setColor(QPalette::ButtonText, textBlack);
        p.setColor(QPalette::BrightText, Qt::red);
        p.setColor(QPalette::Link, QColor(26, 95, 180));
        p.setColor(QPalette::Highlight, QColor(61, 126, 190));
        p.setColor(QPalette::HighlightedText, Qt::white);
        p.setColor(QPalette::PlaceholderText, QColor(138, 138, 138));
        p.setColor(QPalette::Disabled, QPalette::Text, QColor(160, 160, 160));
        p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(160, 160, 160));
        qApp->setPalette(p);
    }
}

void MainWindow::updateThemeButton() {
    if (!m_themeButton)
        return;
    const bool dark = (m_theme == QLatin1String("oscuro"));
    m_themeButton->setIcon(QIcon(dark ? QStringLiteral(":/img/moon.svg")
                                    : QStringLiteral(":/img/sun.svg")));
    m_themeButton->setText(dark ? tr("Dark") : tr("Light"));
}

void MainWindow::changeEvent(QEvent* event) {
    if (event && event->type() == QEvent::LanguageChange)
        retranslate();
    if (event && event->type() == QEvent::WindowStateChange && m_titleBar)
        m_titleBar->refreshMaximizeGlyph();
    QMainWindow::changeEvent(event);
}

void MainWindow::retranslate() {
    setWindowTitle(tr("Gambasse"));

    if (m_actEs) m_actEs->setText(tr("Spanish"));
    if (m_actPt) m_actPt->setText(tr("Portuguese"));
    if (m_lightAction) m_lightAction->setText(tr("Light"));
    if (m_darkAction) m_darkAction->setText(tr("Dark"));
    if (m_languageButton) m_languageButton->setToolTip(tr("Language"));
    if (m_themeButton) m_themeButton->setToolTip(tr("Theme"));
    updateLanguageButton();
    updateThemeButton();

    if (m_dataGroup)    m_dataGroup->setTitle(tr("Basic data"));
    if (m_addressGroup) m_addressGroup->setTitle(tr("Address"));

    if (m_codeCaption) m_codeCaption->setText(tr("Code:"));
    if (!m_valueLabels.isEmpty()) {
        m_valueLabels[FieldName]->setText(tr("Name:"));
        m_valueLabels[FieldSex]->setText(tr("Sex:"));
        m_valueLabels[FieldDate]->setText(tr("Birth date:"));
        m_valueLabels[FieldAge]->setText(tr("Age:"));
        m_valueLabels[FieldAddress]->setText(tr("Address:"));
        m_valueLabels[FieldCohabitants]->setText(tr("Cohabitants:"));
        m_valueLabels[FieldContact]->setText(tr("Contact:"));
        m_valueLabels[FieldSiblings]->setText(tr("Siblings:"));
    }

    if (m_pediatricHistoryButton) {
        m_createPediatricButton->setText(tr("Create pediatric history"));
        m_pediatricHistoryButton->setText(tr("Pediatric history"));
        m_pediatricConsultationButton->setText(tr("Pediatric consultation"));
        m_createAdultButton->setText(tr("Create adult history"));
        m_adultHistoryButton->setText(tr("Adult history"));
        m_adultConsultationButton->setText(tr("Adult consultation"));
        m_createPregnancyButton->setText(tr("Create pregnancy history"));
        m_pregnancyHistoryButton->setText(tr("Pregnancy history"));
        m_pregnancyConsultationButton->setText(tr("Pregnancy consultation"));
    }

    if (m_addButton) {
        m_addButton->setText(tr("Add"));
        m_editButton->setText(tr("Edit"));
        m_deleteButton->setText(tr("Delete"));
        m_saveButton->setText(tr("Save"));
        m_cancelButton->setText(tr("Cancel"));
    }

    if (m_model) m_model->refreshHeaders();

    // Each filter placeholder is its column name.
    for (int c = 0; c < m_filters.size(); ++c) {
        const QString cab = m_model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
        m_filters[c]->setPlaceholderText(cab);
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (m_table && obj == m_table->viewport() && event->type() == QEvent::Resize)
        positionFilters();

    // Manual edge resizing for the frameless window. The filter runs before
    // the title bar's own drag handling, so border presses resize while the
    // rest of the bar still moves the window.
    if (m_titleBar && (obj == m_titleBar || obj == centralWidget())) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* press = static_cast<QMouseEvent*>(event);
            if (press->button() == Qt::LeftButton) {
                const int edges = resizeEdgesAt(press->globalPosition().toPoint());
                if (edges != NoEdge) {
                    m_resizeEdges = edges;
                    m_resizeStartPos = press->globalPosition().toPoint();
                    m_resizeStartGeometry = frameGeometry();
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto* move = static_cast<QMouseEvent*>(event);
            const QPoint globalPos = move->globalPosition().toPoint();
            if (m_resizeEdges != NoEdge) {
                QRect geometry = m_resizeStartGeometry;
                const QPoint delta = globalPos - m_resizeStartPos;
                if (m_resizeEdges & EdgeLeft)
                    geometry.setLeft(geometry.left() + delta.x());
                if (m_resizeEdges & EdgeRight)
                    geometry.setRight(geometry.right() + delta.x());
                if (m_resizeEdges & EdgeTop)
                    geometry.setTop(geometry.top() + delta.y());
                if (m_resizeEdges & EdgeBottom)
                    geometry.setBottom(geometry.bottom() + delta.y());
                if (geometry.width() < minimumWidth())
                    geometry.setWidth(minimumWidth());
                if (geometry.height() < minimumHeight())
                    geometry.setHeight(minimumHeight());
                setGeometry(geometry.normalized());
                return true;
            }
            if (QWidget* watched = qobject_cast<QWidget*>(obj))
                watched->setCursor(cursorForEdges(resizeEdgesAt(globalPos)));
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto* release = static_cast<QMouseEvent*>(event);
            if (release->button() == Qt::LeftButton && m_resizeEdges != NoEdge) {
                m_resizeEdges = NoEdge;
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

int MainWindow::resizeEdgesAt(const QPoint& globalPos) const {
    if (isMaximized())
        return NoEdge;
    const QRect frame = frameGeometry();
    int edges = NoEdge;
    if (globalPos.x() <= frame.left() + kResizeMargin)
        edges |= EdgeLeft;
    if (globalPos.x() >= frame.right() - kResizeMargin)
        edges |= EdgeRight;
    if (globalPos.y() <= frame.top() + kResizeMargin)
        edges |= EdgeTop;
    if (globalPos.y() >= frame.bottom() - kResizeMargin)
        edges |= EdgeBottom;
    return edges;
}

Qt::CursorShape MainWindow::cursorForEdges(int edges) {
    switch (edges) {
    case EdgeLeft:
    case EdgeRight:
        return Qt::SizeHorCursor;
    case EdgeTop:
    case EdgeBottom:
        return Qt::SizeVerCursor;
    case EdgeLeft | EdgeTop:
    case EdgeRight | EdgeBottom:
        return Qt::SizeFDiagCursor;
    case EdgeRight | EdgeTop:
    case EdgeLeft | EdgeBottom:
        return Qt::SizeBDiagCursor;
    default:
        return Qt::ArrowCursor;
    }
}

} // namespace gambasse
