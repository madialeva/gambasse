#include <ui/window/MainWindow.h>

#include <Paths.h>
#include <UxWidgets/UxComboInput.h>
#include <UxWidgets/UxDateField.h>
#include <UxWidgets/UxDateInput.h>
#include <UxWidgets/UxField.h>
#include <UxWidgets/UxLabel.h>
#include <UxWidgets/UxNumberField.h>
#include <UxWidgets/UxNumberInput.h>
#include <UxWidgets/UxTextField.h>
#include <UxWidgets/UxTextInput.h>
#include <data/model/PatientsModel.h>
#include <logic/SexLabel.h>
#include <ui/filter/ColumnFilterProxy.h>
#include <ui/window/AdultHistoryWindow.h>
#include <ui/window/PediatricHistoryWindow.h>
#include <ui/window/PregnancyHistoryWindow.h>
#include <ui/window/TitleBar.h>

#include <QApplication>
#include <QAbstractButton>
#include <QAction>
#include <QActionGroup>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFont>
#include <QMouseEvent>
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
#include <QSizePolicy>
#include <QSplitter>
#include <QStyle>
#include <QStyleFactory>
#include <QTableView>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace gambasse {

namespace {

// Resize border width (pixels) for the frameless window.
constexpr int kResizeMargin = 6;

// Resize edge flags.
enum ResizeEdge { NoEdge = 0, EdgeLeft = 1, EdgeTop = 2, EdgeRight = 4, EdgeBottom = 8 };
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();

    // Initial language and theme (config.ini; Portuguese and light by default).
    changeTheme(m_appSettings.theme());
    changeLanguage(m_appSettings.language());

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
    v->setContentsMargins(6, 0, 6, 6);

    v->addWidget(m_titleBar);
    v->addWidget(m_toolbar);

    // Manual edge resizing (lost with the native frame) is handled through
    // an event filter on the title bar and the central widget.
    m_titleBar->installEventFilter(this);
    central->installEventFilter(this);

    // History-creation buttons, located below the photo in the right column.
    // The pediatric one opens the pediatric history window; adult/pregnancy
    // stay inert until their windows are ported.
    m_createPediatricButton = new QPushButton(central);
    m_createAdultButton = new QPushButton(central);
    m_createPregnancyButton = new QPushButton(central);
    connect(m_createPediatricButton, &QPushButton::clicked,
            this, &MainWindow::openPediatricHistory);
    connect(m_createAdultButton, &QPushButton::clicked,
            this, &MainWindow::openAdultHistory);
    connect(m_createPregnancyButton, &QPushButton::clicked,
            this, &MainWindow::openPregnancyHistory);

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
    // Identifier: always read-only, shown with the custom non-editable label.
    m_codeCaption = new UxLabel(central);
    m_codeValue = new UxLabel(central);
    m_codeValue->setTextInteractionFlags(Qt::TextSelectableByMouse);

    // Field limits mirror b01_patient columns.
    constexpr int kMaxName        = 200;  // name
    constexpr int kMaxAddress     = 100;  // address
    constexpr int kMaxCohabitants = 100;  // cohabitants
    constexpr int kMaxContact     = 100;  // contact_person

    // Custom UxWidgets composite controls (label + field in one widget).
    m_nameInput = new UxTextInput(QString(), central);
    m_nameInput->textField()->setUppercase(true);  // prevent case-only duplicates
    m_nameInput->setRequired(true);                // visual feedback when empty
    m_nameInput->setMaxLength(kMaxName);
    m_sexInput = new UxComboInput(QString(), central);
    // The combo stores the persisted enum value; its labels follow the language.
    m_sexInput->comboBox()->addItem(sexLabel(Patient::Sex::Home), static_cast<int>(Patient::Sex::Home));
    m_sexInput->comboBox()->addItem(sexLabel(Patient::Sex::Muller), static_cast<int>(Patient::Sex::Muller));
    m_dateInput = new UxDateInput(QString(), central);
    m_ageInput = new UxNumberInput(QString(), central);
    m_ageInput->numberField()->setIntegerDigits(3);
    m_addressInput = new UxTextInput(QString(), central);
    m_addressInput->setMaxLength(kMaxAddress);
    m_cohabitantsInput = new UxTextInput(QString(), central);
    m_cohabitantsInput->setMaxLength(kMaxCohabitants);
    m_contactInput = new UxTextInput(QString(), central);
    m_contactInput->setMaxLength(kMaxContact);
    m_siblingsInput = new UxNumberInput(QString(), central);
    m_siblingsInput->numberField()->setIntegerDigits(2);

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
    auto* dataFields = new QVBoxLayout(m_dataGroup);
    // First row: read-only identifier (left) and name (remaining space). The
    // name control carries its own integrated label.
    m_codeValue->setMinimumWidth(45);
    auto* codeAndNameRow = new QHBoxLayout();
    codeAndNameRow->addWidget(m_codeCaption);
    codeAndNameRow->addWidget(m_codeValue);
    codeAndNameRow->addSpacing(12);
    codeAndNameRow->addWidget(m_nameInput, 1);
    dataFields->addLayout(codeAndNameRow);

    // Sex uses the custom label + combo composite like the rest of the fields.
    dataFields->addWidget(m_sexInput);

    dataFields->addWidget(m_dateInput);
    dataFields->addWidget(m_ageInput);
    dataFields->addStretch(1);

    m_addressGroup = new QGroupBox(central);
    auto* addressLayout = new QVBoxLayout(m_addressGroup);
    addressLayout->addWidget(m_addressInput);
    addressLayout->addWidget(m_cohabitantsInput);
    addressLayout->addWidget(m_contactInput);
    addressLayout->addWidget(m_siblingsInput);
    addressLayout->addStretch(1);

    auto* dataLayout = new QHBoxLayout();
    dataLayout->addWidget(m_dataGroup, 1);
    dataLayout->addWidget(m_addressGroup, 1);
    v->addLayout(dataLayout);

    setCentralWidget(central);
    setEditMode(false);   // starts in view mode
    clearDetail();
}

void MainWindow::buildToolbar() {
    // Plain widget in the central layout (below the title bar), not a docked
    // toolbar: docked toolbars always render above the central area.
    m_toolbar = new QToolBar(this);
    QToolBar* toolbar = m_toolbar;
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

    // The history entries open their windows (existing or new); only the
    // consultation entries stay inert until their forms are ported.
    disconnect(m_pediatricHistoryButton, &QToolButton::clicked,
               this, &MainWindow::onActionNotImplemented);
    connect(m_pediatricHistoryButton, &QToolButton::clicked,
            this, &MainWindow::openPediatricHistory);
    disconnect(m_adultHistoryButton, &QToolButton::clicked,
               this, &MainWindow::onActionNotImplemented);
    connect(m_adultHistoryButton, &QToolButton::clicked,
            this, &MainWindow::openAdultHistory);
    disconnect(m_pregnancyHistoryButton, &QToolButton::clicked,
               this, &MainWindow::onActionNotImplemented);
    connect(m_pregnancyHistoryButton, &QToolButton::clicked,
            this, &MainWindow::openPregnancyHistory);

    // Language and theme live in the custom title bar now (same buttons,
    // menus and persistence as before).
    // Language button: flag and name, with a language menu.
    m_languageButton = new QToolButton(m_titleBar);
    m_languageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_languageButton->setPopupMode(QToolButton::InstantPopup);
    auto* menuIdioma = new QMenu(m_languageButton);
    auto* grpIdioma = new QActionGroup(this);
    grpIdioma->setExclusive(true);
    struct LanguageEntry { const char* code; const char* flag; };
    const LanguageEntry languages[] = {
        {"es", ":/img/flag-es.png"},
        {"pt", ":/img/flag-pt.png"},
        {"en", ":/img/flag-en.png"},
    };
    for (const LanguageEntry& language : languages) {
        const QString code = QString::fromLatin1(language.code);
        QAction* action = menuIdioma->addAction(QIcon(QString::fromLatin1(language.flag)), QString());
        action->setCheckable(true);
        grpIdioma->addAction(action);
        m_languageActions.insert(code, action);
        connect(action, &QAction::triggered, this, [this, code]() { changeLanguage(code); });
    }
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
        m_nameInput->setText(QString());
        m_sexInput->comboBox()->setCurrentIndex(
            m_sexInput->comboBox()->findData(static_cast<int>(Patient::Sex::Muller)));
        m_dateInput->setText(QDate(1900, 1, 1).toString(QString::fromLatin1(UxDateField::kDateFormat)));
        m_ageInput->setText(QStringLiteral("0"));
        m_addressInput->setText(QString());
        m_cohabitantsInput->setText(QString());
        m_contactInput->setText(QString());
        m_siblingsInput->setText(QStringLiteral("0"));
        return;
    }
    m_codeValue->setText(QString::number(p->id));
    m_nameInput->setText(p->name);
    m_sexInput->comboBox()->setCurrentIndex(
        m_sexInput->comboBox()->findData(static_cast<int>(p->sex)));
    const QDate birth = p->birthDate.isValid() ? p->birthDate : QDate(1900, 1, 1);
    m_dateInput->setText(birth.toString(QString::fromLatin1(UxDateField::kDateFormat)));
    m_ageInput->setText(QString::number(p->ageRange));
    m_addressInput->setText(p->address);
    m_cohabitantsInput->setText(p->cohabitants);
    m_contactInput->setText(p->contactPerson);
    m_siblingsInput->setText(QString::number(p->siblingCount));
}

void MainWindow::gatherFromFields(Patient& p) const {
    p.name = m_nameInput->text().trimmed();
    p.sex = static_cast<Patient::Sex>(m_sexInput->comboBox()->currentData().toInt());
    QDate birth = QDate::fromString(m_dateInput->text().trimmed(),
                                    QString::fromLatin1(UxDateField::kDateFormat));
    if (!birth.isValid() || birth.year() < 1900 || birth.year() > 2100)
        birth = QDate(1900, 1, 1);
    p.birthDate = birth;
    p.ageRange = qBound(0, m_ageInput->text().toInt(), 130);
    p.address = m_addressInput->text().trimmed();
    p.cohabitants = m_cohabitantsInput->text().trimmed();
    p.contactPerson = m_contactInput->text().trimmed();
    p.siblingCount = qBound(0, m_siblingsInput->text().toInt(), 99);
}

void MainWindow::setEditMode(bool editing) {
    m_isEditing = editing;
    // Disable the inner fields, not the composites, so their labels keep the
    // normal (non-disabled) colour in view mode.
    const QVector<UxInput*> inputs = {m_nameInput, m_dateInput, m_ageInput, m_addressInput,
                                      m_cohabitantsInput, m_contactInput, m_siblingsInput};
    for (UxInput* input : inputs)
        if (input) input->field()->setEnabled(editing);
    if (m_sexInput) m_sexInput->comboBox()->setEnabled(editing);

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
        m_photo->setPixmap(QPixmap(m_photoManager.defaultPhotoResource())
                              .scaled(m_photo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    updateContextButtons(nullptr);
}

// Selects the row of the patient with the given identifier after a model reload.
void MainWindow::selectPatientById(qlonglong patientId) {
    int sourceRow = -1;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const Patient* p = m_model->patientAt(row);
        if (p && p->id == patientId) {
            sourceRow = row;
            break;
        }
    }
    if (sourceRow < 0) {
        if (m_proxy->rowCount() > 0)
            m_table->selectRow(0);
        else
            clearDetail();
        return;
    }
    const QModelIndex proxyIdx = m_proxy->mapFromSource(m_model->index(sourceRow, 0));
    if (proxyIdx.isValid()) {
        m_table->selectRow(proxyIdx.row());
        m_table->scrollTo(proxyIdx);
    }
}

// --- CRUD ---

void MainWindow::onAdd() {
    m_editingRow = -1;            // new patient
    loadIntoFields(nullptr);      // empty fields, 1900 date, Muller sex
    // Display the calculated identifier (read-only).
    m_codeValue->setText(QString::number(m_patientService.nextId()));
    updateContextButtons(nullptr);
    loadPhoto(nullptr);
    setEditMode(true);
    m_nameInput->textField()->setFocus();
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
    m_nameInput->textField()->setFocus();
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

    const bool isNew = (m_editingRow < 0);
    const PatientService::SaveResult result =
        isNew ? m_patientService.create(p)
              : m_patientService.update(m_previousPatient, p);

    switch (result) {
    case PatientService::SaveResult::NameRequired:
        QMessageBox::warning(this, tr("Incomplete data"), tr("Name is required."));
        m_nameInput->textField()->setFocus();
        return;
    case PatientService::SaveResult::Duplicate:
        QMessageBox::warning(this, tr("Duplicate patient"),
            tr("A patient with the same name, birth date, sex, and age already exists."));
        return;
    case PatientService::SaveResult::Error:
        QMessageBox::critical(this, tr("Error"), tr("Could not save the patient."));
        return;
    case PatientService::SaveResult::Saved:
        break;
    }

    const qlonglong savedId = p.id;
    m_editingRow = -1;
    setEditMode(false);

    // Reload so ordering/filtering reflect the change, then re-select the patient.
    m_model->load();
    selectPatientById(savedId);
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

    const int previousProxyRow = m_table->selectionModel()->currentIndex().row();
    const Patient toDelete = *p;   // copy before the model is reloaded

    if (!m_patientService.remove(toDelete)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not delete the patient."));
        return;
    }

    m_model->load();
    if (m_proxy->rowCount() > 0)
        m_table->selectRow(qBound(0, previousProxyRow, m_proxy->rowCount() - 1));
    else
        clearDetail();
}

void MainWindow::updateContextButtons(const Patient* p) {
    // The three Create buttons are always visible; only their enabled state
    // changes. Entry buttons in the toolbar use their QAction for visibility.
    const Availability availability = m_clinicalContext.availability(p);

    m_pediatricHistoryAction->setVisible(availability.showPediatric);
    m_pediatricConsultationAction->setVisible(availability.showPediatric);
    m_adultHistoryAction->setVisible(availability.showAdult);
    m_adultConsultationAction->setVisible(availability.showAdult);
    m_pregnancyHistoryAction->setVisible(availability.showPregnancy);
    m_pregnancyConsultationAction->setVisible(availability.showPregnancy);

    // History creation below the photo is always visible, but only enabled when
    // the history does not exist and the patient meets the age/sex criteria.
    m_createPediatricButton->setVisible(true);
    m_createPediatricButton->setEnabled(availability.canCreatePediatric);
    m_createAdultButton->setVisible(true);
    m_createAdultButton->setEnabled(availability.canCreateAdult);
    m_createPregnancyButton->setVisible(true);
    m_createPregnancyButton->setEnabled(availability.canCreatePregnancy);
}

void MainWindow::loadPhoto(const Patient* p) {
    if (!m_photo)
        return;
    QPixmap pm;
    if (p) {
        const QString photoPath = m_photoManager.photoPath(*p);
        const QString legacyPath = m_photoManager.legacyPhotoPath(*p);
        if (QFileInfo::exists(photoPath))
            pm.load(photoPath);
        else if (QFileInfo::exists(legacyPath))
            pm.load(legacyPath);
    }
    if (pm.isNull())
        pm = QPixmap(m_photoManager.defaultPhotoResource());

    m_photo->setPixmap(pm.scaled(m_photo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onActionNotImplemented() {
    QMessageBox::information(
        this, tr("Unavailable"),
        tr("This feature is not implemented in this phase yet."));
}

void MainWindow::openPediatricHistory() {
    const Patient* p = selectedPatient();
    if (!p)
        return;
    PediatricHistoryWindow dialog(p->id, p->name, this);
    if (!dialog.isValid())
        return;
    // After saving or deleting, the entry/create buttons change state.
    if (dialog.exec() == QDialog::Accepted)
        updateContextButtons(selectedPatient());
}

void MainWindow::openAdultHistory() {
    const Patient* p = selectedPatient();
    if (!p)
        return;
    AdultHistoryWindow dialog(p->id, p->name, this);
    if (!dialog.isValid())
        return;
    // After saving or deleting, the entry/create buttons change state.
    if (dialog.exec() == QDialog::Accepted)
        updateContextButtons(selectedPatient());
}

void MainWindow::openPregnancyHistory() {
    const Patient* p = selectedPatient();
    if (!p)
        return;
    PregnancyHistoryWindow dialog(p->id, p->name, this);
    if (!dialog.isValid())
        return;
    // After saving or deleting, the entry/create buttons change state.
    if (dialog.exec() == QDialog::Accepted)
        updateContextButtons(selectedPatient());
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

    m_appSettings.setLanguage(code);

    for (auto it = m_languageActions.constBegin(); it != m_languageActions.constEnd(); ++it)
        it.value()->setChecked(it.key() == code);

    retranslate(); // refuerzo por si installTranslator no dispara LanguageChange
}

void MainWindow::updateLanguageButton() {
    if (!m_languageButton)
        return;
    QAction* active = m_languageActions.value(m_languageCode);
    if (!active)
        active = m_languageActions.value(QStringLiteral("pt"));
    if (active) {
        m_languageButton->setIcon(active->icon());
        m_languageButton->setText(active->text());
    }
}

void MainWindow::changeTheme(const QString& mode) {
    m_theme = (mode == QLatin1String("oscuro")) ? QStringLiteral("oscuro") : QStringLiteral("claro");
    applyTheme(m_theme);

    m_appSettings.setTheme(m_theme);

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

    if (QAction* es = m_languageActions.value(QStringLiteral("es"))) es->setText(tr("Spanish"));
    if (QAction* pt = m_languageActions.value(QStringLiteral("pt"))) pt->setText(tr("Portuguese"));
    if (QAction* en = m_languageActions.value(QStringLiteral("en"))) en->setText(tr("English"));
    if (m_lightAction) m_lightAction->setText(tr("Light"));
    if (m_darkAction) m_darkAction->setText(tr("Dark"));
    if (m_languageButton) m_languageButton->setToolTip(tr("Language"));
    if (m_themeButton) m_themeButton->setToolTip(tr("Theme"));
    updateLanguageButton();
    updateThemeButton();

    if (m_dataGroup)    m_dataGroup->setTitle(tr("Basic data"));
    if (m_addressGroup) m_addressGroup->setTitle(tr("Address"));

    if (m_codeCaption) m_codeCaption->setText(tr("Code:"));
    if (m_sexInput) m_sexInput->setLabelText(tr("Sex:"));
    if (m_nameInput) m_nameInput->setLabelText(tr("Name:"));
    if (m_dateInput) m_dateInput->setLabelText(tr("Birth date:"));
    if (m_ageInput) m_ageInput->setLabelText(tr("Age:"));
    if (m_addressInput) m_addressInput->setLabelText(tr("Address:"));
    if (m_cohabitantsInput) m_cohabitantsInput->setLabelText(tr("Cohabitants:"));
    if (m_contactInput) m_contactInput->setLabelText(tr("Contact:"));
    if (m_siblingsInput) m_siblingsInput->setLabelText(tr("Siblings:"));
    alignDetailLabels();

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

    if (m_model) m_model->refreshLanguage();

    // The sex combo labels follow the language; the selected value is preserved
    // because only the item text changes.
    if (m_sexInput) {
        QComboBox* sexCombo = m_sexInput->comboBox();
        const int homeIndex = sexCombo->findData(static_cast<int>(Patient::Sex::Home));
        const int mullerIndex = sexCombo->findData(static_cast<int>(Patient::Sex::Muller));
        if (homeIndex >= 0) sexCombo->setItemText(homeIndex, sexLabel(Patient::Sex::Home));
        if (mullerIndex >= 0) sexCombo->setItemText(mullerIndex, sexLabel(Patient::Sex::Muller));
    }

    // Each filter placeholder is its column name.
    for (int c = 0; c < m_filters.size(); ++c) {
        const QString cab = m_model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString();
        m_filters[c]->setPlaceholderText(cab);
    }
}

void MainWindow::alignDetailLabels() {
    // Integrated labels take their natural width, so the fields of a detail
    // group would not line up in a column. Give the labels of each group a
    // common width (recomputed after every retranslation).
    auto labelOf = [](QWidget* widget) -> QLabel* {
        if (auto* caption = qobject_cast<QLabel*>(widget))
            return caption;
        return widget->findChild<QLabel*>();
    };
    auto align = [&labelOf](const QVector<QWidget*>& widgets) {
        int width = 0;
        for (QWidget* w : widgets)
            if (QLabel* label = labelOf(w))
                width = qMax(width, label->sizeHint().width());
        for (QWidget* w : widgets)
            if (QLabel* label = labelOf(w))
                label->setFixedWidth(width);
    };

    align({m_sexInput, m_dateInput, m_ageInput});
    align({m_addressInput, m_cohabitantsInput, m_contactInput, m_siblingsInput});
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
