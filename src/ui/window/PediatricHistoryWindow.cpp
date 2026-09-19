#include <ui/window/PediatricHistoryWindow.h>

#include <UxWidgets/UxCheck.h>
#include <UxWidgets/UxComboInput.h>
#include <UxWidgets/UxDateField.h>
#include <UxWidgets/UxDateInput.h>
#include <UxWidgets/UxLabel.h>
#include <UxWidgets/UxNumberField.h>
#include <UxWidgets/UxNumberInput.h>
#include <UxWidgets/UxTextField.h>
#include <UxWidgets/UxTextInput.h>
#include <logic/PediatricHistoryLabels.h>
#include <ui/window/TitleBar.h>

#include <QComboBox>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace gambasse {

namespace {

// Client area replicating FrmHistoriaPediatrica (usable on small monitors).
constexpr int kContentWidth = 1008;
constexpr int kContentHeight = 561;

UxTextInput* makeText(QWidget* parent, int x, int y, int w, int h, int maxLength) {
    auto* input = new UxTextInput(QString(), parent);
    input->setGeometry(x, y, w, h);
    if (maxLength > 0)
        input->setMaxLength(maxLength);
    return input;
}

UxDateInput* makeDate(QWidget* parent, int x, int y, int w, int h) {
    auto* input = new UxDateInput(QString(), parent);
    input->setGeometry(x, y, w, h);
    return input;
}

UxNumberInput* makeNumber(QWidget* parent, int x, int y, int w, int h, int integerDigits) {
    auto* input = new UxNumberInput(QString(), parent);
    input->setGeometry(x, y, w, h);
    input->numberField()->setIntegerDigits(integerDigits);
    return input;
}

// Checks auto-size to their text: keep the specified position and grow to
// fit the (translated) caption.
UxCheck* makeCheck(QWidget* parent, int x, int y) {
    auto* check = new UxCheck(parent);
    check->move(x, y);
    return check;
}

UxLabel* makeLabel(QWidget* parent, int x, int y, int w, int h) {
    auto* label = new UxLabel(parent);
    label->setGeometry(x, y, w, h);
    return label;
}

UxComboInput* makeCombo(QWidget* parent, int x, int y, int w, int h) {
    auto* input = new UxComboInput(QString(), parent);
    input->setGeometry(x, y, w, h);
    return input;
}

QGroupBox* makeGroup(QWidget* parent, int x, int y, int w, int h) {
    auto* group = new QGroupBox(parent);
    group->setGeometry(x, y, w, h);
    return group;
}

QDate parseDate(const QString& text) {
    if (text.trimmed().isEmpty())
        return QDate();
    return QDate::fromString(text.trimmed(),
                             QString::fromLatin1(UxDateField::kDateFormat));
}

QString formatDate(const QDate& date) {
    if (!date.isValid())
        return QString();
    return date.toString(QString::fromLatin1(UxDateField::kDateFormat));
}

} // namespace

PediatricHistoryWindow::PediatricHistoryWindow(qlonglong patientId,
                                               const QString& patientName,
                                               QWidget* parent)
    : QDialog(parent), m_patientId(patientId), m_patientName(patientName) {
    // Frameless modal dialog: the OS title bar is replaced by
    // the custom TitleBar in close-only mode.
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setWindowModality(Qt::WindowModal);
    setModal(true);

    buildUi();
    populateCombos();
    retranslate();

    if (!loadHistory()) {
        QMessageBox::critical(this, tr("Error"), tr("Could not load the history."));
        m_valid = false;
        return;
    }
    m_valid = true;
    loadIntoFields();
}

void PediatricHistoryWindow::buildUi() {
    auto* content = new QWidget(this);
    content->setFixedSize(kContentWidth, kContentHeight);

    m_titleBar = new TitleBar();
    m_titleBar->setCloseOnly(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 0, 6, 6);
    layout->setSpacing(0);
    layout->addWidget(m_titleBar);
    layout->addWidget(content);
    layout->addStretch(1);
    setFixedSize(sizeHint());

    // Same highlighted button style as the main window, following the active
    // theme (it cannot change while this modal dialog is open).
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const QString base = dark ? QStringLiteral("#27496D") : QStringLiteral("#CFE2F3");
    const QString hover = dark ? QStringLiteral("#2F5A85") : QStringLiteral("#BBD6EE");
    const QString press = dark ? QStringLiteral("#1B3550") : QStringLiteral("#A9C9E8");
    const QString border = dark ? QStringLiteral("#1B3550") : QStringLiteral("#9FC3E8");
    const QString text = dark ? QStringLiteral("white") : QStringLiteral("#1A1A1A");
    const QString disabledBackground = dark ? QStringLiteral("#3A3A3A") : QStringLiteral("#E4E3DF");
    const QString disabledText = dark ? QStringLiteral("#7A7A7A") : QStringLiteral("#A6A6A6");
    const QString disabledBorder = dark ? QStringLiteral("#333333") : QStringLiteral("#CFCFCF");
    const QString buttonStyle =
        QStringLiteral("QPushButton { background:%1; color:%2; border:1px solid %3;"
                       " border-radius:4px; padding:4px 10px; font-weight:bold; }"
                       " QPushButton:hover:enabled { background:%4; }"
                       " QPushButton:pressed:enabled { background:%5; }"
                       " QPushButton:disabled { background:%6; color:%7; border-color:%8; }")
            .arg(base, text, border, hover, press,
                 disabledBackground, disabledText, disabledBorder);
    const QString closeStyle =
        QStringLiteral("QToolButton { background:transparent; border:none;"
                       " padding:4px 8px; font-weight:bold; }"
                       " QToolButton:hover:enabled { background:#E81123; color:white; }"
                       " QToolButton:pressed:enabled { background:#A00A18; color:white; }");
    if (QToolButton* close = m_titleBar->closeButton())
        close->setStyleSheet(closeStyle);

    // --- Personal data (2, 2, 482, 116). ---
    // Taller group so the top field clears the frame; inner controls keep a
    // 7 px bottom padding like the birth history group. The three groups
    // below are pushed 7 px down to keep the gaps.
    QGroupBox* personal = makeGroup(content, 2, 2, 482, 116);
    m_nameInput = makeText(personal, 15, 29, 359, 21, 200);
    m_nameInput->field()->setEnabled(false); // display only: patient name shown here
    // Opening date, birth weight and height are sized for the longest label
    // across languages (222/148/123 px), so the input field keeps room for
    // its value in every language.
    m_openingDateInput = makeDate(personal, 15, 58, 222, 21);
    m_allergiesInput = makeText(personal, 15, 88, 462, 21, 100);

    // --- Birth history (2, 120, 482, 108). ---
    QGroupBox* birth = makeGroup(content, 2, 120, 482, 108);
    m_birthWeightInput = makeNumber(birth, 13, 26, 148, 21, 5);
    m_birthHeightInput = makeNumber(birth, 181, 26, 123, 21, 3);
    m_deliveryIncidentsInput = makeText(birth, 15, 53, 462, 21, 100);
    m_malformationsInput = makeText(birth, 15, 80, 462, 21, 100);

    // --- Child history (2, 233, 482, 129). ---
    // Inner controls keep a 7 px bottom padding like the birth history group.
    QGroupBox* child = makeGroup(content, 2, 233, 482, 129);
    m_chronicLabel = makeLabel(child, 15, 24, 138, 17);
    m_diseaseTypeInput = makeText(child, 15, 47, 461, 21, 3);
    m_diseaseTreatmentInput = makeText(child, 15, 74, 461, 21, 100);
    m_otherDiseasesInput = makeText(child, 15, 101, 462, 21, 200);

    // --- Housing (2, 368, 482, 174). ---
    QGroupBox* housing = makeGroup(content, 2, 368, 482, 174);
    m_latrineCheck = makeCheck(housing, 15, 31);
    m_mosquitoNetCheck = makeCheck(housing, 175, 31);
    m_netUseCheck = makeCheck(housing, 293, 31);
    m_domesticHygieneCheck = makeCheck(housing, 15, 55);
    m_domesticAnimalsInput = makeCombo(housing, 175, 55, 284, 23);
    m_domesticAnimalsInput->comboBox()->setEditable(false);
    m_conflictsInput = makeText(housing, 14, 91, 462, 38, 200);
    m_economicSituationInput = makeText(housing, 15, 143, 461, 21, 0);

    // --- Feeding (492, 2, 514, 117). ---
    // Group enlarged to align the treated-water details field with its check
    // caption; inner controls keep a 7 px bottom padding. The two groups
    // below are pushed 8 px down to keep the gaps.
    QGroupBox* feeding = makeGroup(content, 492, 2, 514, 117);
    m_breastfeedingCheck = makeCheck(feeding, 17, 30);
    m_treatedWaterCheck = makeCheck(feeding, 193, 30);
    m_treatedWaterDetailsInput = makeText(feeding, 328, 31, 103, 21, 0);
    m_feedingProblemsInput = makeText(feeding, 17, 62, 491, 21, 100);
    m_additionalFeedingInput = makeText(feeding, 17, 89, 491, 21, 100);

    // --- Previous treatments (492, 121, 514, 173). ---
    // First row carries its labels above the fields; the other rows have no
    // labels, so their inner captions are hidden for the fields to span the
    // full width (VB zero-width labels). All fields are 22 px tall.
    QGroupBox* treatments = makeGroup(content, 492, 121, 514, 173);
    const int dateX[4] = {17, 17, 17, 17};
    const int dateY[4] = {29, 78, 106, 134};
    const int dateH[4] = {44, 22, 22, 22};
    const int comboY[4] = {30, 78, 106, 134};
    const int comboH[4] = {44, 22, 22, 22};
    for (int i = 0; i < 4; ++i) {
        m_supplementDateInputs[i] = makeDate(treatments, dateX[i], dateY[i], 100, dateH[i]);
        m_supplementTreatmentInputs[i] = makeCombo(treatments, 123, comboY[i], 221, comboH[i]);
        m_supplementTreatmentInputs[i]->comboBox()->setEditable(true);
        if (i > 0) {
            // No caption: hide it so the field uses the whole composite width.
            if (QLabel* caption = m_supplementDateInputs[i]->findChild<QLabel*>(
                    QString(), Qt::FindDirectChildrenOnly))
                caption->hide();
            if (QLabel* caption = m_supplementTreatmentInputs[i]->findChild<QLabel*>(
                    QString(), Qt::FindDirectChildrenOnly))
                caption->hide();
        }
    }
    m_supplementDateInputs[0]->setLabelPosition(UxInput::Above);
    m_supplementTreatmentInputs[0]->setLabelPosition(UxComboInput::Above);

    // --- Vaccines (492, 289, 371, 169). ---
    // Inner rows pushed 8 px down so the first row clears the group frame;
    // the group grows to keep the bottom padding.
    QGroupBox* vaccines = makeGroup(content, 492, 297, 371, 169);
    const int rowY[6] = {27, 50, 73, 96, 119, 142};
    for (int i = 0; i < 6; ++i)
        m_vaccineRowLabels[i] = makeLabel(vaccines, 17, rowY[i], 92, 17);
    m_bcgCheck = makeCheck(vaccines, 115, 26);
    m_opvChecks[0] = makeCheck(vaccines, 225, 26);
    m_dptHibChecks[0] = makeCheck(vaccines, 115, 49);
    m_opvChecks[1] = makeCheck(vaccines, 225, 49);
    m_hepBChecks[0] = makeCheck(vaccines, 302, 49);
    m_dptHibChecks[1] = makeCheck(vaccines, 115, 72);
    m_opvChecks[2] = makeCheck(vaccines, 225, 72);
    m_hepBChecks[1] = makeCheck(vaccines, 302, 72);
    m_dptHibChecks[2] = makeCheck(vaccines, 115, 95);
    m_opvChecks[3] = makeCheck(vaccines, 225, 95);
    m_hepBChecks[2] = makeCheck(vaccines, 302, 96);
    m_measles9Check = makeCheck(vaccines, 115, 118);
    m_measles10Check = makeCheck(vaccines, 115, 141);

    // --- Bottom buttons. ---
    m_deleteButton = new QPushButton(content);
    m_deleteButton->setGeometry(570, 524, 134, 32);
    m_saveButton = new QPushButton(content);
    m_saveButton->setGeometry(720, 524, 134, 32);
    m_exitButton = new QPushButton(content);
    m_exitButton->setGeometry(871, 524, 134, 32);
    for (QPushButton* b : {m_deleteButton, m_saveButton, m_exitButton})
        b->setStyleSheet(buttonStyle);
    connect(m_deleteButton, &QPushButton::clicked, this, &PediatricHistoryWindow::onDelete);
    connect(m_saveButton, &QPushButton::clicked, this, &PediatricHistoryWindow::onSave);
    connect(m_exitButton, &QPushButton::clicked, this, &PediatricHistoryWindow::onExit);
}

void PediatricHistoryWindow::populateCombos() {
    auto fillAnimals = [this]() {
        QComboBox* combo = m_domesticAnimalsInput->comboBox();
        const int current = combo->currentData().toInt();
        combo->clear();
        for (int v = 0; v <= 3; ++v) {
            const auto value = static_cast<PediatricHistory::DomesticAnimals>(v);
            combo->addItem(domesticAnimalsLabel(value), v);
        }
        combo->setCurrentIndex(combo->findData(current));
    };
    fillAnimals();

    for (UxComboInput* input : {m_supplementTreatmentInputs[0], m_supplementTreatmentInputs[1],
                                m_supplementTreatmentInputs[2], m_supplementTreatmentInputs[3]}) {
        QComboBox* combo = input->comboBox();
        const int current = combo->currentData().toInt();
        const QString typed = combo->currentText();
        combo->clear();
        for (int v = 0; v <= 7; ++v) {
            const auto value = static_cast<PediatricHistory::TreatmentType>(v);
            combo->addItem(treatmentTypeLabel(value), v);
        }
        const int restored = combo->findData(current);
        combo->setCurrentIndex(restored);
        if (restored < 0 && !typed.isEmpty())
            combo->setCurrentText(typed);
    }
}

void PediatricHistoryWindow::retranslate() {
    setWindowTitle(tr("Pediatric History"));
    // The native bar is never visible: show the window title in the custom
    // bar instead of the application name.
    if (m_titleBar)
        m_titleBar->setTitle(windowTitle());

    auto* personal = m_nameInput->parentWidget();
    auto* birth = m_birthWeightInput->parentWidget();
    auto* child = m_chronicLabel->parentWidget();
    auto* housing = m_latrineCheck->parentWidget();
    auto* feeding = m_breastfeedingCheck->parentWidget();
    auto* treatments = m_supplementDateInputs[0]->parentWidget();
    auto* vaccines = m_bcgCheck->parentWidget();
    if (auto* g = qobject_cast<QGroupBox*>(personal)) g->setTitle(tr("Personal data"));
    if (auto* g = qobject_cast<QGroupBox*>(birth)) g->setTitle(tr("Birth history"));
    if (auto* g = qobject_cast<QGroupBox*>(child)) g->setTitle(tr("Child history"));
    if (auto* g = qobject_cast<QGroupBox*>(housing)) g->setTitle(tr("Housing"));
    if (auto* g = qobject_cast<QGroupBox*>(feeding)) g->setTitle(tr("Feeding"));
    if (auto* g = qobject_cast<QGroupBox*>(treatments)) g->setTitle(tr("Previous treatments"));
    if (auto* g = qobject_cast<QGroupBox*>(vaccines)) g->setTitle(tr("Vaccines"));

    m_nameInput->setLabelText(tr("Name:"));
    m_openingDateInput->setLabelText(tr("Opening date:"));
    m_allergiesInput->setLabelText(tr("Allergies:"));

    m_birthWeightInput->setLabelText(tr("Weight (grams):"));
    m_birthHeightInput->setLabelText(tr("Height (cm):"));
    m_deliveryIncidentsInput->setLabelText(tr("Delivery incidents:"));
    m_malformationsInput->setLabelText(tr("Malformations or disabilities:"));

    m_chronicLabel->setText(tr("Chronic diseases"));
    m_diseaseTypeInput->setLabelText(tr("Disease type:"));
    m_diseaseTreatmentInput->setLabelText(tr("Treatment:"));
    m_otherDiseasesInput->setLabelText(tr("Other diseases:"));

    m_latrineCheck->setText(tr("Latrine"));
    m_mosquitoNetCheck->setText(tr("Mosquito net"));
    m_netUseCheck->setText(tr("Net use"));
    m_domesticHygieneCheck->setText(tr("Domestic hygiene"));
    m_domesticAnimalsInput->setLabelText(tr("Domestic animals:"));
    m_conflictsInput->setLabelText(tr("Conflicts, relationships, family problems:"));
    m_economicSituationInput->setLabelText(tr("Economic situation:"));

    m_breastfeedingCheck->setText(tr("Breastfeeding"));
    m_treatedWaterCheck->setText(tr("Treated water WITH:"));
    m_feedingProblemsInput->setLabelText(tr("Feeding problems:"));
    m_additionalFeedingInput->setLabelText(tr("Additional feeding:"));

    m_supplementDateInputs[0]->setLabelText(tr("Date:"));
    m_supplementTreatmentInputs[0]->setLabelText(tr("Medication:"));

    m_vaccineRowLabels[0]->setText(tr("Birth"));
    m_vaccineRowLabels[1]->setText(tr("6 weeks"));
    m_vaccineRowLabels[2]->setText(tr("10 weeks"));
    m_vaccineRowLabels[3]->setText(tr("14 weeks"));
    m_vaccineRowLabels[4]->setText(tr("9 months"));
    m_vaccineRowLabels[5]->setText(tr("10 months"));

    m_bcgCheck->setText(QStringLiteral("BCG"));
    m_opvChecks[0]->setText(QStringLiteral("OPV-0"));
    m_opvChecks[1]->setText(QStringLiteral("OPV-1"));
    m_opvChecks[2]->setText(QStringLiteral("OPV-2"));
    m_opvChecks[3]->setText(QStringLiteral("OPV-3"));
    m_dptHibChecks[0]->setText(QStringLiteral("DPT+HIB-1"));
    m_dptHibChecks[1]->setText(QStringLiteral("DPT+HIB-2"));
    m_dptHibChecks[2]->setText(QStringLiteral("DPT+HIB-3"));
    m_hepBChecks[0]->setText(QStringLiteral("Hep B1"));
    m_hepBChecks[1]->setText(QStringLiteral("Hep B2"));
    m_hepBChecks[2]->setText(QStringLiteral("Hep B3"));
    m_measles9Check->setText(tr("Measles"));
    m_measles10Check->setText(tr("Measles"));

    for (UxCheck* c : {m_latrineCheck, m_mosquitoNetCheck, m_netUseCheck, m_domesticHygieneCheck,
                       m_breastfeedingCheck, m_treatedWaterCheck, m_bcgCheck, m_opvChecks[0],
                       m_opvChecks[1], m_opvChecks[2], m_opvChecks[3], m_dptHibChecks[0],
                       m_dptHibChecks[1], m_dptHibChecks[2], m_hepBChecks[0], m_hepBChecks[1],
                       m_hepBChecks[2], m_measles9Check, m_measles10Check})
        c->adjustSize();

    populateCombos();

    m_deleteButton->setText(tr("Delete history"));
    m_saveButton->setText(tr("Save history"));
    m_exitButton->setText(tr("Exit"));
}

bool PediatricHistoryWindow::loadHistory() {
    return m_service.load(m_patientId, m_history, m_isNew);
}

void PediatricHistoryWindow::loadIntoFields() {
    m_nameInput->setText(m_patientName);
    m_openingDateInput->setText(formatDate(m_history.openingDate));
    m_allergiesInput->setText(m_history.allergies);

    m_birthWeightInput->setText(QString::number(m_history.neonatalWeight));
    m_birthHeightInput->setText(QString::number(m_history.neonatalHeight));
    m_deliveryIncidentsInput->setText(m_history.neonatalDeliveryIncidents);
    m_malformationsInput->setText(m_history.neonatalMalformations);

    m_diseaseTypeInput->setText(m_history.clinicalDiseaseType);
    m_diseaseTreatmentInput->setText(m_history.clinicalDiseaseTreatment);
    m_otherDiseasesInput->setText(m_history.otherDiseases);

    m_latrineCheck->setChecked(m_history.latrine);
    m_mosquitoNetCheck->setChecked(m_history.mosquitoNet);
    m_netUseCheck->setChecked(m_history.netUse);
    m_domesticHygieneCheck->setChecked(m_history.domesticHygiene);
    m_domesticAnimalsInput->comboBox()->setCurrentIndex(
        m_domesticAnimalsInput->comboBox()->findData(static_cast<int>(m_history.domesticAnimals)));
    m_conflictsInput->setText(m_history.conflicts);
    m_economicSituationInput->setText(m_history.economicSituation);

    m_breastfeedingCheck->setChecked(m_history.feedingBreastfeeding);
    m_treatedWaterCheck->setChecked(m_history.feedingTreatedWater);
    m_treatedWaterDetailsInput->setText(m_history.feedingTreatedWaterDetails);
    m_feedingProblemsInput->setText(m_history.feedingFeedingProblems);
    m_additionalFeedingInput->setText(m_history.feedingAdditionalFeeding);

    const QDate dates[4] = {m_history.supplementDate1, m_history.supplementDate2,
                            m_history.supplementDate3, m_history.supplementDate4};
    const PediatricHistory::TreatmentType kinds[4] = {
        m_history.supplementTreatment1, m_history.supplementTreatment2,
        m_history.supplementTreatment3, m_history.supplementTreatment4};
    for (int i = 0; i < 4; ++i) {
        m_supplementDateInputs[i]->setText(formatDate(dates[i]));
        m_supplementTreatmentInputs[i]->comboBox()->setCurrentIndex(
            m_supplementTreatmentInputs[i]->comboBox()->findData(static_cast<int>(kinds[i])));
    }

    m_bcgCheck->setChecked(m_history.vaccineBcg);
    const bool opv[4] = {m_history.vaccineOpv0, m_history.vaccineOpv1,
                         m_history.vaccineOpv2, m_history.vaccineOpv3};
    for (int i = 0; i < 4; ++i)
        m_opvChecks[i]->setChecked(opv[i]);
    const bool dpt[3] = {m_history.vaccineDptHib1, m_history.vaccineDptHib2,
                         m_history.vaccineDptHib3};
    for (int i = 0; i < 3; ++i)
        m_dptHibChecks[i]->setChecked(dpt[i]);
    const bool hep[3] = {m_history.vaccineHepB1, m_history.vaccineHepB2, m_history.vaccineHepB3};
    for (int i = 0; i < 3; ++i)
        m_hepBChecks[i]->setChecked(hep[i]);
    m_measles9Check->setChecked(m_history.vaccineMeasles9);
    m_measles10Check->setChecked(m_history.vaccineMeasles10);

    // A new history cannot be deleted yet.
    m_deleteButton->setEnabled(!m_isNew);
}

static int treatmentValue(QComboBox* combo) {
    const int index = combo->currentIndex();
    if (index >= 0)
        return combo->itemData(index).toInt();
    // Editable combo with custom text: match it against the list, if possible.
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemText(i).compare(combo->currentText(), Qt::CaseInsensitive) == 0)
            return combo->itemData(i).toInt();
    }
    return 0;
}

void PediatricHistoryWindow::gatherFromFields(PediatricHistory& history) const {
    history.patientId = m_patientId;
    history.openingDate = parseDate(m_openingDateInput->text());
    if (!history.openingDate.isValid())
        history.openingDate = QDate(1900, 1, 1);
    history.allergies = m_allergiesInput->text().trimmed();

    history.neonatalWeight = qBound(0, m_birthWeightInput->text().toInt(), 99999);
    history.neonatalHeight = qBound(0, m_birthHeightInput->text().toInt(), 999);
    history.neonatalDeliveryIncidents = m_deliveryIncidentsInput->text().trimmed();
    history.neonatalMalformations = m_malformationsInput->text().trimmed();

    history.clinicalDiseaseType = m_diseaseTypeInput->text().trimmed();
    history.clinicalDiseaseTreatment = m_diseaseTreatmentInput->text().trimmed();
    history.otherDiseases = m_otherDiseasesInput->text().trimmed();

    history.latrine = m_latrineCheck->isChecked();
    history.mosquitoNet = m_mosquitoNetCheck->isChecked();
    history.netUse = m_netUseCheck->isChecked();
    history.domesticHygiene = m_domesticHygieneCheck->isChecked();
    history.domesticAnimals = static_cast<PediatricHistory::DomesticAnimals>(
        m_domesticAnimalsInput->comboBox()->currentData().toInt());
    history.conflicts = m_conflictsInput->text().trimmed();
    history.economicSituation = m_economicSituationInput->text().trimmed();

    history.feedingBreastfeeding = m_breastfeedingCheck->isChecked();
    history.feedingTreatedWater = m_treatedWaterCheck->isChecked();
    history.feedingTreatedWaterDetails = m_treatedWaterDetailsInput->text().trimmed();
    history.feedingFeedingProblems = m_feedingProblemsInput->text().trimmed();
    history.feedingAdditionalFeeding = m_additionalFeedingInput->text().trimmed();

    QDate* dates[4] = {&history.supplementDate1, &history.supplementDate2,
                       &history.supplementDate3, &history.supplementDate4};
    PediatricHistory::TreatmentType* kinds[4] = {
        &history.supplementTreatment1, &history.supplementTreatment2,
        &history.supplementTreatment3, &history.supplementTreatment4};
    for (int i = 0; i < 4; ++i) {
        *dates[i] = parseDate(m_supplementDateInputs[i]->text());
        *kinds[i] = static_cast<PediatricHistory::TreatmentType>(
            treatmentValue(m_supplementTreatmentInputs[i]->comboBox()));
    }

    history.vaccineBcg = m_bcgCheck->isChecked();
    history.vaccineOpv0 = m_opvChecks[0]->isChecked();
    history.vaccineOpv1 = m_opvChecks[1]->isChecked();
    history.vaccineOpv2 = m_opvChecks[2]->isChecked();
    history.vaccineOpv3 = m_opvChecks[3]->isChecked();
    history.vaccineDptHib1 = m_dptHibChecks[0]->isChecked();
    history.vaccineDptHib2 = m_dptHibChecks[1]->isChecked();
    history.vaccineDptHib3 = m_dptHibChecks[2]->isChecked();
    history.vaccineHepB1 = m_hepBChecks[0]->isChecked();
    history.vaccineHepB2 = m_hepBChecks[1]->isChecked();
    history.vaccineHepB3 = m_hepBChecks[2]->isChecked();
    history.vaccineMeasles9 = m_measles9Check->isChecked();
    history.vaccineMeasles10 = m_measles10Check->isChecked();
}

void PediatricHistoryWindow::onSave() {
    PediatricHistory history;
    gatherFromFields(history);
    if (m_service.save(m_isNew, history) != PediatricHistoryService::SaveResult::Saved) {
        QMessageBox::critical(this, tr("Error"), tr("Could not save the history."));
        return;
    }
    accept();
}

void PediatricHistoryWindow::onDelete() {
    if (m_isNew)
        return;
    if (QMessageBox::question(this, tr("Delete history"),
                              tr("Delete the pediatric history of \"%1\"?").arg(m_patientName))
        != QMessageBox::Yes)
        return;
    if (!m_service.remove(m_patientId)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not delete the history."));
        return;
    }
    accept();
}

void PediatricHistoryWindow::onExit() {
    reject();
}

void PediatricHistoryWindow::changeEvent(QEvent* event) {
    if (event && event->type() == QEvent::LanguageChange)
        retranslate();
    if (event && event->type() == QEvent::WindowStateChange && m_titleBar)
        m_titleBar->refreshMaximizeGlyph();
    QDialog::changeEvent(event);
}

} // namespace gambasse
