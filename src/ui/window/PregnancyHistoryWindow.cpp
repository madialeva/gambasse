#include <ui/window/PregnancyHistoryWindow.h>

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
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace gambasse {

namespace {

// Client area replicating FrmHistoriaMullerGravida (usable on small monitors).
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

UxNumberInput* makeNumber(QWidget* parent, int x, int y, int w, int h,
                          int integerDigits, int decimalDigits = 0) {
    auto* input = new UxNumberInput(QString(), parent);
    input->setGeometry(x, y, w, h);
    input->numberField()->setIntegerDigits(integerDigits);
    input->numberField()->setDecimalDigits(decimalDigits);
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

// Rows below the first one carry no caption: hide it so the field uses the
// whole composite width (VB zero-width labels).
void hideCaption(QWidget* input) {
    if (QLabel* caption = input->findChild<QLabel*>(QString(), Qt::FindDirectChildrenOnly))
        caption->hide();
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

// The number field shows decimals with a comma (as in the VB.NET form).
double parseDecimal(const QString& text) {
    QString normalized = text.trimmed();
    normalized.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    const double value = normalized.toDouble(&ok);
    return ok ? value : 0.0;
}

QString formatDecimal(double value, int decimals) {
    return QString::number(value, 'f', decimals).replace(QLatin1Char('.'), QLatin1Char(','));
}

} // namespace

PregnancyHistoryWindow::PregnancyHistoryWindow(qlonglong patientId,
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

void PregnancyHistoryWindow::buildUi() {
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

    // --- Personal data (2, 2, 482, 109). ---
    QGroupBox* personal = makeGroup(content, 2, 2, 482, 109);
    m_nameInput = makeText(personal, 15, 18, 359, 21, 200);
    m_nameInput->field()->setEnabled(false); // display only: patient name shown here
    m_openingDateInput = makeDate(personal, 15, 47, 196, 21);
    m_allergiesInput = makeText(personal, 15, 77, 462, 21, 100);

    // --- Pregnancy history (2, 113, 482, 181). ---
    QGroupBox* pregnancy = makeGroup(content, 2, 113, 482, 181);
    m_lastMenstruationInput = makeDate(pregnancy, 15, 21, 261, 21);
    m_deceasedSiblingCountInput = makeNumber(pregnancy, 298, 21, 178, 21, 2);
    m_expectedDeliveryInput = makeDate(pregnancy, 15, 49, 231, 21);
    m_deliveryCountInput = makeNumber(pregnancy, 252, 47, 103, 21, 2);
    m_abortionCountInput = makeNumber(pregnancy, 361, 47, 116, 21, 3);
    m_deliveryProblemsInput = makeText(pregnancy, 15, 74, 462, 21, 200);
    m_gynecologicalDiseasesInput = makeText(pregnancy, 14, 101, 462, 21, 200);
    m_barrierMethodsInput = makeText(pregnancy, 15, 128, 462, 21, 200);
    m_hivWomanCheck = makeCheck(pregnancy, 15, 155);
    m_hivManCheck = makeCheck(pregnancy, 125, 155);
    m_serologiesInput = makeText(pregnancy, 252, 154, 224, 21, 100);

    // --- Medical history (2, 295, 482, 263). ---
    QGroupBox* medical = makeGroup(content, 2, 295, 482, 263);
    m_chronicLabel = makeLabel(medical, 15, 17, 138, 17);
    m_diabetesCheck = makeCheck(medical, 15, 42);
    m_diabetesTreatmentInput = makeText(medical, 99, 39, 378, 21, 100);
    m_hepatitisCheck = makeCheck(medical, 15, 73);
    m_hepatitisTypeInput = makeText(medical, 99, 73, 73, 21, 3);
    m_hepatitisTreatmentInput = makeText(medical, 190, 73, 286, 21, 100);
    m_hypertensionCheck = makeCheck(medical, 15, 105);
    m_hypertensionTreatmentInput = makeText(medical, 99, 105, 378, 21, 100);
    m_otherDiseasesInput = makeText(medical, 15, 133, 462, 21, 200);
    m_disabilitiesInput = makeText(medical, 15, 175, 462, 21, 200);
    m_conflictsInput = makeText(medical, 15, 214, 462, 38, 200);

    // --- Housing (492, 2, 514, 87). ---
    QGroupBox* housing = makeGroup(content, 492, 2, 514, 87);
    m_potableWaterCheck = makeCheck(housing, 17, 18);
    m_latrineCheck = makeCheck(housing, 129, 18);
    m_mosquitoNetParentsCheck = makeCheck(housing, 316, 18);
    m_netUseParentsCheck = makeCheck(housing, 449, 18);
    m_treatedWaterCheck = makeCheck(housing, 17, 38);
    m_domesticHygieneCheck = makeCheck(housing, 129, 38);
    m_mosquitoNetChildrenCheck = makeCheck(housing, 316, 38);
    m_netUseChildrenCheck = makeCheck(housing, 449, 38);
    m_domesticAnimalsInput = makeCombo(housing, 17, 57, 284, 23);
    m_domesticAnimalsInput->comboBox()->setEditable(false);

    // --- Physical examination (492, 95, 514, 126). ---
    QGroupBox* exam = makeGroup(content, 492, 95, 514, 126);
    m_weightInput = makeNumber(exam, 6, 22, 105, 21, 3, 1);
    m_temperatureInput = makeNumber(exam, 121, 22, 81, 21, 2, 1);
    m_heightInput = makeNumber(exam, 6, 47, 110, 21, 3);
    m_abdominalPerimeterInput = makeNumber(exam, 6, 74, 109, 21, 3);
    m_vomitingInput = makeText(exam, 6, 99, 150, 21, 100);
    m_fetalAuscultationInput = makeText(exam, 167, 99, 166, 21, 100);
    m_fetalPositionInput = makeText(exam, 342, 99, 166, 21, 100);
    QGroupBox* uterine = makeGroup(exam, 208, 11, 122, 66);
    m_uterineWeeksInput = makeNumber(uterine, 6, 17, 110, 21, 2);
    m_uterineCmInput = makeNumber(uterine, 6, 40, 110, 21, 2);
    QGroupBox* urine = makeGroup(exam, 337, 11, 97, 66);
    m_leukocytosisCheck = makeCheck(urine, 9, 21);
    m_proteinuriaCheck = makeCheck(urine, 9, 40);
    QGroupBox* edemas = makeGroup(exam, 440, 11, 68, 83);
    m_mmiCheck = makeCheck(edemas, 9, 19);
    m_faceCheck = makeCheck(edemas, 9, 39);
    m_generalCheck = makeCheck(edemas, 9, 59);

    // --- Treatments (492, 223, 514, 295): tab container, no group title. ---
    QGroupBox* treatments = makeGroup(content, 492, 223, 514, 295);
    auto* tabs = new QTabWidget(treatments);
    tabs->setGeometry(7, 16, 501, 271);
    m_treatmentTabs = tabs;
    auto* prevPage = new QWidget(tabs);
    auto* nowPage = new QWidget(tabs);

    // Previous treatment: 5 rows of date / medication / dose.
    m_prevHeaderLabel = makeLabel(prevPage, 11, 3, 327, 52);
    const int prevX[3] = {5, 111, 296};
    const int prevW[3] = {100, 179, 183};
    const int prevY[5] = {51, 94, 122, 150, 178};
    const int prevH[5] = {38, 22, 22, 22, 22};
    for (int i = 0; i < 5; ++i) {
        m_supplementDateInputs[i] = makeDate(prevPage, prevX[0], prevY[i], prevW[0], prevH[i]);
        m_supplementMedicationInputs[i] =
            makeText(prevPage, prevX[1], prevY[i], prevW[1], prevH[i], 100);
        m_supplementDoseInputs[i] =
            makeText(prevPage, prevX[2], prevY[i], prevW[2], prevH[i], 100);
        if (i > 0) {
            hideCaption(m_supplementDateInputs[i]);
            hideCaption(m_supplementMedicationInputs[i]);
            hideCaption(m_supplementDoseInputs[i]);
        }
    }
    m_supplementDateInputs[0]->setLabelPosition(UxInput::Above);
    m_supplementMedicationInputs[0]->setLabelPosition(UxInput::Above);
    m_supplementDoseInputs[0]->setLabelPosition(UxInput::Above);

    // Current treatments: 7 rows of dose / medication / next dose.
    const int nowX[3] = {141, 240, 348};
    const int nowW[3] = {100, 109, 127};
    const int nowY[7] = {13, 50, 70, 90, 110, 130, 150};
    const int nowLabelY[7] = {32, 50, 70, 93, 114, 134, 153};
    UxTextInput** nowRows[7] = {m_calciumInputs, m_ironInputs, m_folicAcidInputs,
                                m_dewormingInputs, m_vitaminCInputs,
                                m_otherVitaminsInputs, m_malariaInputs};
    for (int row = 0; row < 7; ++row) {
        const int h = (row == 0) ? 38 : 21;
        m_nowRowLabels[row] = makeLabel(nowPage, (row == 5) ? 18 : 19, nowLabelY[row],
                                        (row >= 5) ? 120 : 116, 17);
        for (int col = 0; col < 3; ++col) {
            nowRows[row][col] = makeText(nowPage, nowX[col], nowY[row], nowW[col], h, 50);
            if (row > 0)
                hideCaption(nowRows[row][col]);
        }
        if (row == 0) {
            for (int col = 0; col < 3; ++col)
                nowRows[row][col]->setLabelPosition(UxInput::Above);
        }
    }
    // Rows without a column carry the fixed VB.NET values and are display
    // only: the VB form never persists them.
    for (UxTextInput** fixed : {m_ironInputs, m_folicAcidInputs, m_dewormingInputs})
        for (int col = 0; col < 3; ++col)
            fixed[col]->field()->setEnabled(false);

    tabs->addTab(prevPage, QString());
    tabs->addTab(nowPage, QString());

    // --- Bottom buttons. ---
    m_deleteButton = new QPushButton(content);
    m_deleteButton->setGeometry(565, 524, 137, 32);
    m_saveButton = new QPushButton(content);
    m_saveButton->setGeometry(716, 524, 137, 32);
    m_exitButton = new QPushButton(content);
    m_exitButton->setGeometry(867, 524, 137, 32);
    for (QPushButton* b : {m_deleteButton, m_saveButton, m_exitButton})
        b->setStyleSheet(buttonStyle);
    connect(m_deleteButton, &QPushButton::clicked, this, &PregnancyHistoryWindow::onDelete);
    connect(m_saveButton, &QPushButton::clicked, this, &PregnancyHistoryWindow::onSave);
    connect(m_exitButton, &QPushButton::clicked, this, &PregnancyHistoryWindow::onExit);
}

void PregnancyHistoryWindow::populateCombos() {
    QComboBox* combo = m_domesticAnimalsInput->comboBox();
    const int current = combo->currentData().toInt();
    combo->clear();
    for (int v = 0; v <= 3; ++v) {
        const auto value = static_cast<PediatricHistory::DomesticAnimals>(v);
        combo->addItem(domesticAnimalsLabel(value), v);
    }
    combo->setCurrentIndex(combo->findData(current));
}

void PregnancyHistoryWindow::retranslate() {
    setWindowTitle(tr("Pregnancy History"));
    // The native bar is never visible: show the window title in the custom
    // bar instead of the application name.
    if (m_titleBar)
        m_titleBar->setTitle(windowTitle());

    auto* personal = m_nameInput->parentWidget();
    auto* pregnancy = m_lastMenstruationInput->parentWidget();
    auto* medical = m_chronicLabel->parentWidget();
    auto* housing = m_potableWaterCheck->parentWidget();
    auto* exam = m_weightInput->parentWidget();
    auto* uterine = m_uterineWeeksInput->parentWidget();
    auto* urine = m_leukocytosisCheck->parentWidget();
    auto* edemas = m_mmiCheck->parentWidget();
    if (auto* g = qobject_cast<QGroupBox*>(personal)) g->setTitle(tr("Personal data"));
    if (auto* g = qobject_cast<QGroupBox*>(pregnancy)) g->setTitle(tr("Pregnancy history"));
    if (auto* g = qobject_cast<QGroupBox*>(medical)) g->setTitle(tr("Pregnant woman history"));
    if (auto* g = qobject_cast<QGroupBox*>(housing)) g->setTitle(tr("Housing"));
    if (auto* g = qobject_cast<QGroupBox*>(exam)) g->setTitle(tr("Physical examination"));
    if (auto* g = qobject_cast<QGroupBox*>(uterine)) g->setTitle(tr("Uterine height"));
    if (auto* g = qobject_cast<QGroupBox*>(urine)) g->setTitle(tr("Urine"));
    if (auto* g = qobject_cast<QGroupBox*>(edemas)) g->setTitle(tr("Edemas"));

    m_nameInput->setLabelText(tr("Name:"));
    m_openingDateInput->setLabelText(tr("Opening date:"));
    m_allergiesInput->setLabelText(tr("Allergies:"));

    m_lastMenstruationInput->setLabelText(tr("Last menstruation date:"));
    m_deceasedSiblingCountInput->setLabelText(tr("Deceased siblings at birth:"));
    m_expectedDeliveryInput->setLabelText(tr("Expected delivery date:"));
    m_deliveryCountInput->setLabelText(tr("Delivery count:"));
    m_abortionCountInput->setLabelText(tr("Abortion count:"));
    m_deliveryProblemsInput->setLabelText(tr("Delivery problems:"));
    m_gynecologicalDiseasesInput->setLabelText(tr("Gynecological diseases:"));
    m_barrierMethodsInput->setLabelText(tr("Barrier methods:"));
    m_hivWomanCheck->setText(tr("HIV (woman)"));
    m_hivManCheck->setText(tr("HIV (man)"));
    m_serologiesInput->setLabelText(tr("Serologies:"));

    m_chronicLabel->setText(tr("Chronic diseases"));
    m_diabetesCheck->setText(tr("Diabetes"));
    m_diabetesTreatmentInput->setLabelText(tr("Treatment:"));
    m_hepatitisCheck->setText(tr("Hepatitis"));
    m_hepatitisTypeInput->setLabelText(tr("Type:"));
    m_hepatitisTreatmentInput->setLabelText(tr("Treatment:"));
    m_hypertensionCheck->setText(QStringLiteral("HTA"));
    m_hypertensionTreatmentInput->setLabelText(tr("Treatment:"));
    m_otherDiseasesInput->setLabelText(tr("Other diseases:"));
    m_disabilitiesInput->setLabelText(tr("Disabilities:"));
    m_conflictsInput->setLabelText(tr("Conflicts, relationships, family problems:"));

    m_potableWaterCheck->setText(tr("Potable water"));
    m_latrineCheck->setText(tr("Latrine"));
    m_mosquitoNetParentsCheck->setText(tr("Parents mosquito net"));
    m_netUseParentsCheck->setText(tr("Parents net use"));
    m_treatedWaterCheck->setText(tr("Treated water"));
    m_domesticHygieneCheck->setText(tr("Domestic hygiene"));
    m_mosquitoNetChildrenCheck->setText(tr("Children mosquito net"));
    m_netUseChildrenCheck->setText(tr("Children net use"));
    m_domesticAnimalsInput->setLabelText(tr("Domestic animals:"));

    m_weightInput->setLabelText(tr("Weight (Kg):"));
    m_temperatureInput->setLabelText(tr("Temperature (ºC):"));
    m_heightInput->setLabelText(tr("Height (cm):"));
    m_abdominalPerimeterInput->setLabelText(tr("Abdominal P. (cm):"));
    m_vomitingInput->setLabelText(tr("Vomiting:"));
    m_fetalAuscultationInput->setLabelText(tr("Fetal auscultation:"));
    m_fetalPositionInput->setLabelText(tr("Fetal position:"));
    m_uterineWeeksInput->setLabelText(tr("Week:"));
    m_uterineCmInput->setLabelText(tr("Centimeters:"));
    m_leukocytosisCheck->setText(tr("Leukocytosis"));
    m_proteinuriaCheck->setText(tr("Proteinuria"));
    m_mmiCheck->setText(QStringLiteral("MMI"));
    m_faceCheck->setText(tr("Face"));
    m_generalCheck->setText(tr("General"));

    // Treatment tabs (the only standard control, themed by the palette).
    if (m_treatmentTabs) {
        m_treatmentTabs->setTabText(0, tr("Previous treatment"));
        m_treatmentTabs->setTabText(1, tr("Current treatments"));
    }

    if (m_prevHeaderLabel)
        m_prevHeaderLabel->setText(
            tr("MALARIA PROPHYLAXIS, DEWORMING, VITAMIN A, FOLIC ACID, IRON"));

    m_supplementDateInputs[0]->setLabelText(tr("Date:"));
    m_supplementMedicationInputs[0]->setLabelText(tr("Medication:"));
    m_supplementDoseInputs[0]->setLabelText(tr("Dose:"));

    const QString nowNames[7] = {tr("Calcium"), tr("Iron"), tr("Folic acid"),
                                 tr("Deworming"), tr("Vitamin C"),
                                 tr("Other vitamins"), tr("Malaria prophylaxis")};
    UxTextInput** nowRows[7] = {m_calciumInputs, m_ironInputs, m_folicAcidInputs,
                                m_dewormingInputs, m_vitaminCInputs,
                                m_otherVitaminsInputs, m_malariaInputs};
    for (int row = 0; row < 7; ++row) {
        m_nowRowLabels[row]->setText(nowNames[row]);
        nowRows[row][0]->setLabelText(tr("Dose:"));
        nowRows[row][1]->setLabelText(tr("Medication:"));
        nowRows[row][2]->setLabelText(tr("Next dose:"));
    }
    // Fixed VB.NET display values for the rows without a column.
    m_ironInputs[0]->setText(tr("60 mg a day"));
    m_ironInputs[1]->setText(tr("Ferrous sulfate"));
    m_ironInputs[2]->setText(tr("Until birth"));
    m_folicAcidInputs[0]->setText(tr("4 mg a day"));
    m_folicAcidInputs[1]->setText(tr("Vitamin B9"));
    m_folicAcidInputs[2]->setText(tr("Until the fifth month of pregnancy"));
    m_dewormingInputs[0]->setText(tr("2nd or 3rd trimester"));
    m_dewormingInputs[1]->setText(tr("Mebendazole"));
    m_dewormingInputs[2]->setText(tr("Single dose"));

    for (UxCheck* c : {m_hivWomanCheck, m_hivManCheck, m_diabetesCheck,
                       m_hepatitisCheck, m_hypertensionCheck, m_potableWaterCheck,
                       m_latrineCheck, m_mosquitoNetParentsCheck, m_netUseParentsCheck,
                       m_treatedWaterCheck, m_domesticHygieneCheck,
                       m_mosquitoNetChildrenCheck, m_netUseChildrenCheck,
                       m_leukocytosisCheck, m_proteinuriaCheck, m_mmiCheck,
                       m_faceCheck, m_generalCheck})
        c->adjustSize();

    populateCombos();

    m_deleteButton->setText(tr("Delete history"));
    m_saveButton->setText(tr("Save history"));
    m_exitButton->setText(tr("Exit"));
}

bool PregnancyHistoryWindow::loadHistory() {
    return m_service.load(m_patientId, m_history, m_isNew);
}

void PregnancyHistoryWindow::loadIntoFields() {
    m_nameInput->setText(m_patientName);
    m_openingDateInput->setText(formatDate(m_history.openingDate));
    m_allergiesInput->setText(m_history.allergies);

    m_lastMenstruationInput->setText(formatDate(m_history.obstetricLastMenstruation));
    m_deceasedSiblingCountInput->setText(QString::number(m_history.obstetricDeceasedSiblingCount));
    m_expectedDeliveryInput->setText(formatDate(m_history.obstetricExpectedDelivery));
    m_deliveryCountInput->setText(QString::number(m_history.obstetricDeliveryCount));
    m_abortionCountInput->setText(QString::number(m_history.obstetricAbortionCount));
    m_deliveryProblemsInput->setText(m_history.obstetricDeliveryProblems);
    m_gynecologicalDiseasesInput->setText(m_history.obstetricGynecologicalDiseases);
    m_barrierMethodsInput->setText(m_history.obstetricBarrierMethods);
    m_hivWomanCheck->setChecked(m_history.obstetricHivWoman);
    m_hivManCheck->setChecked(m_history.obstetricHivMan);
    m_serologiesInput->setText(m_history.obstetricSerologies);

    m_diabetesCheck->setChecked(m_history.medicalDiabetes);
    m_diabetesTreatmentInput->setText(m_history.medicalDiabetesTreatment);
    m_hepatitisCheck->setChecked(m_history.medicalHepatitis);
    m_hepatitisTypeInput->setText(m_history.medicalHepatitisType);
    m_hepatitisTreatmentInput->setText(m_history.medicalHepatitisTreatment);
    m_hypertensionCheck->setChecked(m_history.medicalHypertension);
    m_hypertensionTreatmentInput->setText(m_history.medicalHypertensionTreatment);
    m_otherDiseasesInput->setText(m_history.medicalOtherDiseases);
    m_disabilitiesInput->setText(m_history.medicalDisabilities);
    m_conflictsInput->setText(m_history.medicalConflicts);

    m_potableWaterCheck->setChecked(m_history.housingPotableWater);
    m_latrineCheck->setChecked(m_history.housingLatrine);
    m_mosquitoNetParentsCheck->setChecked(m_history.housingMosquitoNetParents);
    m_netUseParentsCheck->setChecked(m_history.housingMosquitoNetUseParents);
    m_treatedWaterCheck->setChecked(m_history.housingTreatedWater);
    m_domesticHygieneCheck->setChecked(m_history.housingDomesticHygiene);
    m_mosquitoNetChildrenCheck->setChecked(m_history.housingMosquitoNetChildren);
    m_netUseChildrenCheck->setChecked(m_history.housingMosquitoNetUseChildren);
    m_domesticAnimalsInput->comboBox()->setCurrentIndex(
        m_domesticAnimalsInput->comboBox()->findData(
            static_cast<int>(m_history.housingDomesticAnimals)));

    m_weightInput->setText(formatDecimal(m_history.physicalExamWeight, 1));
    m_temperatureInput->setText(formatDecimal(m_history.physicalExamTemperature, 1));
    m_heightInput->setText(QString::number(m_history.physicalExamHeight));
    m_abdominalPerimeterInput->setText(QString::number(m_history.physicalExamAbdominalPerimeter));
    m_vomitingInput->setText(m_history.physicalExamVomiting);
    m_fetalAuscultationInput->setText(m_history.physicalExamFetalAuscultation);
    m_fetalPositionInput->setText(m_history.physicalExamFetalPosition);
    m_uterineWeeksInput->setText(QString::number(m_history.physicalExamUterineHeightWeeks));
    m_uterineCmInput->setText(QString::number(m_history.physicalExamUterineHeightCm));
    m_leukocytosisCheck->setChecked(m_history.physicalExamLeukocytosis);
    m_proteinuriaCheck->setChecked(m_history.physicalExamProteinuria);
    m_mmiCheck->setChecked(m_history.physicalExamMmi);
    m_faceCheck->setChecked(m_history.physicalExamFace);
    m_generalCheck->setChecked(m_history.physicalExamGeneral);

    const QDate dates[5] = {m_history.supplementDate1, m_history.supplementDate2,
                            m_history.supplementDate3, m_history.supplementDate4,
                            m_history.supplementDate5};
    const QString meds[5] = {m_history.supplementMedication1, m_history.supplementMedication2,
                             m_history.supplementMedication3, m_history.supplementMedication4,
                             m_history.supplementMedication5};
    const QString doses[5] = {m_history.supplementDose1, m_history.supplementDose2,
                              m_history.supplementDose3, m_history.supplementDose4,
                              m_history.supplementDose5};
    for (int i = 0; i < 5; ++i) {
        m_supplementDateInputs[i]->setText(formatDate(dates[i]));
        m_supplementMedicationInputs[i]->setText(meds[i]);
        m_supplementDoseInputs[i]->setText(doses[i]);
    }

    const QString calcium[3] = {m_history.supplementCalciumD, m_history.supplementCalciumM,
                                m_history.supplementCalciumPd};
    const QString vitaminC[3] = {m_history.supplementVitaminCD, m_history.supplementVitaminCM,
                                 m_history.supplementVitaminCPd};
    const QString otherVitamins[3] = {m_history.supplementOtherVitaminsD,
                                      m_history.supplementOtherVitaminsM,
                                      m_history.supplementOtherVitaminsPd};
    const QString malaria[3] = {m_history.supplementMalariaProphylaxisD,
                                m_history.supplementMalariaProphylaxisM,
                                m_history.supplementMalariaProphylaxisPd};
    const QString* persisted[4] = {calcium, vitaminC, otherVitamins, malaria};
    UxTextInput** rows[4] = {m_calciumInputs, m_vitaminCInputs, m_otherVitaminsInputs,
                             m_malariaInputs};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 3; ++col)
            rows[row][col]->setText(persisted[row][col]);

    // A new history cannot be deleted yet.
    m_deleteButton->setEnabled(!m_isNew);
}

void PregnancyHistoryWindow::gatherFromFields(PregnancyHistory& history) const {
    history.patientId = m_patientId;
    history.openingDate = parseDate(m_openingDateInput->text());
    if (!history.openingDate.isValid())
        history.openingDate = QDate(1900, 1, 1);
    history.allergies = m_allergiesInput->text().trimmed();

    history.obstetricLastMenstruation = parseDate(m_lastMenstruationInput->text());
    history.obstetricDeceasedSiblingCount =
        qBound(0, m_deceasedSiblingCountInput->text().toInt(), 99);
    history.obstetricExpectedDelivery = parseDate(m_expectedDeliveryInput->text());
    history.obstetricDeliveryCount = qBound(0, m_deliveryCountInput->text().toInt(), 99);
    history.obstetricAbortionCount = qBound(0, m_abortionCountInput->text().toInt(), 999);
    history.obstetricDeliveryProblems = m_deliveryProblemsInput->text().trimmed();
    history.obstetricGynecologicalDiseases = m_gynecologicalDiseasesInput->text().trimmed();
    history.obstetricBarrierMethods = m_barrierMethodsInput->text().trimmed();
    history.obstetricHivWoman = m_hivWomanCheck->isChecked();
    history.obstetricHivMan = m_hivManCheck->isChecked();
    history.obstetricSerologies = m_serologiesInput->text().trimmed();

    history.medicalDiabetes = m_diabetesCheck->isChecked();
    history.medicalDiabetesTreatment = m_diabetesTreatmentInput->text().trimmed();
    history.medicalHepatitis = m_hepatitisCheck->isChecked();
    history.medicalHepatitisType = m_hepatitisTypeInput->text().trimmed();
    history.medicalHepatitisTreatment = m_hepatitisTreatmentInput->text().trimmed();
    history.medicalHypertension = m_hypertensionCheck->isChecked();
    history.medicalHypertensionTreatment = m_hypertensionTreatmentInput->text().trimmed();
    history.medicalOtherDiseases = m_otherDiseasesInput->text().trimmed();
    history.medicalDisabilities = m_disabilitiesInput->text().trimmed();
    history.medicalConflicts = m_conflictsInput->text().trimmed();

    history.housingPotableWater = m_potableWaterCheck->isChecked();
    history.housingLatrine = m_latrineCheck->isChecked();
    history.housingMosquitoNetParents = m_mosquitoNetParentsCheck->isChecked();
    history.housingMosquitoNetUseParents = m_netUseParentsCheck->isChecked();
    history.housingTreatedWater = m_treatedWaterCheck->isChecked();
    history.housingDomesticHygiene = m_domesticHygieneCheck->isChecked();
    history.housingMosquitoNetChildren = m_mosquitoNetChildrenCheck->isChecked();
    history.housingMosquitoNetUseChildren = m_netUseChildrenCheck->isChecked();
    history.housingDomesticAnimals = static_cast<PediatricHistory::DomesticAnimals>(
        m_domesticAnimalsInput->comboBox()->currentData().toInt());

    history.physicalExamWeight = qBound(0.0, parseDecimal(m_weightInput->text()), 999.9);
    history.physicalExamTemperature = qBound(0.0, parseDecimal(m_temperatureInput->text()), 99.9);
    history.physicalExamHeight = qBound(0, m_heightInput->text().toInt(), 999);
    history.physicalExamAbdominalPerimeter =
        qBound(0, m_abdominalPerimeterInput->text().toInt(), 999);
    history.physicalExamVomiting = m_vomitingInput->text().trimmed();
    history.physicalExamFetalAuscultation = m_fetalAuscultationInput->text().trimmed();
    history.physicalExamFetalPosition = m_fetalPositionInput->text().trimmed();
    history.physicalExamUterineHeightWeeks =
        qBound(0, m_uterineWeeksInput->text().toInt(), 99);
    history.physicalExamUterineHeightCm = qBound(0, m_uterineCmInput->text().toInt(), 99);
    history.physicalExamLeukocytosis = m_leukocytosisCheck->isChecked();
    history.physicalExamProteinuria = m_proteinuriaCheck->isChecked();
    history.physicalExamMmi = m_mmiCheck->isChecked();
    history.physicalExamFace = m_faceCheck->isChecked();
    history.physicalExamGeneral = m_generalCheck->isChecked();

    QDate* dates[5] = {&history.supplementDate1, &history.supplementDate2,
                       &history.supplementDate3, &history.supplementDate4,
                       &history.supplementDate5};
    QString* meds[5] = {&history.supplementMedication1, &history.supplementMedication2,
                        &history.supplementMedication3, &history.supplementMedication4,
                        &history.supplementMedication5};
    QString* doses[5] = {&history.supplementDose1, &history.supplementDose2,
                         &history.supplementDose3, &history.supplementDose4,
                         &history.supplementDose5};
    for (int i = 0; i < 5; ++i) {
        *dates[i] = parseDate(m_supplementDateInputs[i]->text());
        *meds[i] = m_supplementMedicationInputs[i]->text().trimmed();
        *doses[i] = m_supplementDoseInputs[i]->text().trimmed();
    }

    history.supplementCalciumD = m_calciumInputs[0]->text().trimmed();
    history.supplementCalciumM = m_calciumInputs[1]->text().trimmed();
    history.supplementCalciumPd = m_calciumInputs[2]->text().trimmed();
    history.supplementVitaminCD = m_vitaminCInputs[0]->text().trimmed();
    history.supplementVitaminCM = m_vitaminCInputs[1]->text().trimmed();
    history.supplementVitaminCPd = m_vitaminCInputs[2]->text().trimmed();
    history.supplementOtherVitaminsD = m_otherVitaminsInputs[0]->text().trimmed();
    history.supplementOtherVitaminsM = m_otherVitaminsInputs[1]->text().trimmed();
    history.supplementOtherVitaminsPd = m_otherVitaminsInputs[2]->text().trimmed();
    history.supplementMalariaProphylaxisD = m_malariaInputs[0]->text().trimmed();
    history.supplementMalariaProphylaxisM = m_malariaInputs[1]->text().trimmed();
    history.supplementMalariaProphylaxisPd = m_malariaInputs[2]->text().trimmed();
}

void PregnancyHistoryWindow::onSave() {
    PregnancyHistory history;
    gatherFromFields(history);
    if (m_service.save(m_isNew, history) != PregnancyHistoryService::SaveResult::Saved) {
        QMessageBox::critical(this, tr("Error"), tr("Could not save the history."));
        return;
    }
    accept();
}

void PregnancyHistoryWindow::onDelete() {
    if (m_isNew)
        return;
    if (QMessageBox::question(this, tr("Delete history"),
                              tr("Delete the pregnancy history of \"%1\"?").arg(m_patientName))
        != QMessageBox::Yes)
        return;
    if (!m_service.remove(m_patientId)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not delete the history."));
        return;
    }
    accept();
}

void PregnancyHistoryWindow::onExit() {
    reject();
}

void PregnancyHistoryWindow::changeEvent(QEvent* event) {
    if (event && event->type() == QEvent::LanguageChange)
        retranslate();
    if (event && event->type() == QEvent::WindowStateChange && m_titleBar)
        m_titleBar->refreshMaximizeGlyph();
    QDialog::changeEvent(event);
}

} // namespace gambasse
