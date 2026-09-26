#include <ui/window/AdultHistoryWindow.h>

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

// Client area replicating FrmHistoriaAdulto (usable on small monitors).
constexpr int kContentWidth = 1008;
constexpr int kContentHeight = 383;

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

AdultHistoryWindow::AdultHistoryWindow(qlonglong patientId,
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

void AdultHistoryWindow::buildUi() {
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

    // --- Personal data (2, 2, 482, 112). ---
    // Taller group so the inner controls keep a 7 px padding with the top
    // frame (below the title) and the bottom frame, like the pediatric
    // window. The group below is pushed 3 px down to keep the gap.
    QGroupBox* personal = makeGroup(content, 2, 2, 482, 112);
    m_nameInput = makeText(personal, 15, 25, 359, 21, 200);
    m_nameInput->field()->setEnabled(false); // display only: patient name shown here
    m_openingDateInput = makeDate(personal, 15, 54, 222, 21);
    m_allergiesInput = makeText(personal, 15, 84, 462, 21, 100);

    // --- Adult history (2, 120, 482, 260). ---
    // Inner controls keep a 7 px padding with the top frame (below the title)
    // and the bottom frame. The first five rows move 8 px down and the last
    // three 4 px, so the gaps between rows stay as in the designer. The group
    // is 3 px shorter so its bottom edge stays at 380, aligned with the
    // buttons like in the designer; the conflicts field gives those 3 px
    // (36 px still shows two lines).
    QGroupBox* adult = makeGroup(content, 2, 120, 482, 260);
    m_chronicLabel = makeLabel(adult, 15, 25, 138, 17);
    m_diabetesCheck = makeCheck(adult, 15, 50);
    m_diabetesTreatmentInput = makeText(adult, 99, 47, 378, 21, 100);
    m_hepatitisCheck = makeCheck(adult, 15, 81);
    m_hepatitisTypeInput = makeText(adult, 94, 81, 73, 21, 3);
    m_hepatitisTreatmentInput = makeText(adult, 175, 81, 302, 21, 100);
    m_hivCheck = makeCheck(adult, 15, 113);
    m_hivTreatmentInput = makeText(adult, 99, 113, 378, 21, 100);
    m_otherDiseasesInput = makeText(adult, 15, 141, 462, 21, 200);
    m_occupationInput = makeText(adult, 15, 165, 269, 21, 100);
    m_childCountInput = makeNumber(adult, 339, 165, 138, 21, 2);
    m_disabilitiesInput = makeText(adult, 15, 191, 462, 21, 200);
    m_conflictsInput = makeText(adult, 15, 217, 462, 36, 200);

    // --- Housing (492, 2, 514, 109). ---
    QGroupBox* housing = makeGroup(content, 492, 2, 514, 109);
    m_potableWaterCheck = makeCheck(housing, 17, 31);
    m_mosquitoNetCheck = makeCheck(housing, 175, 31);
    m_netUseCheck = makeCheck(housing, 293, 31);
    m_latrineCheck = makeCheck(housing, 17, 55);
    m_domesticAnimalsInput = makeCombo(housing, 175, 55, 284, 23);
    m_domesticAnimalsInput->comboBox()->setEditable(false);
    m_domesticHygieneCheck = makeCheck(housing, 17, 79);
    m_sanitaryControlCheck = makeCheck(housing, 175, 80);

    // --- Vital signs (492, 117, 514, 134). ---
    // The BMI field moves to the second row (first on the left, followed by
    // the abdominal perimeter) so the top row has room for the full labels
    // in every language; the top-row fields are widened accordingly.
    QGroupBox* vitals = makeGroup(content, 492, 117, 514, 134);
    m_weightInput = makeNumber(vitals, 6, 22, 150, 21, 3, 1);
    m_temperatureInput = makeNumber(vitals, 164, 22, 185, 21, 2, 1);
    m_heightInput = makeNumber(vitals, 357, 22, 140, 21, 3);
    m_bmiInput = makeNumber(vitals, 6, 49, 85, 21, 2, 2);
    m_abdominalPerimeterInput = makeNumber(vitals, 97, 49, 165, 21, 3);
    m_consciousnessInput = makeText(vitals, 6, 76, 212, 21, 100);
    m_heartRateInput = makeText(vitals, 281, 76, 70, 21, 100);
    m_respiratoryRateInput = makeText(vitals, 390, 76, 69, 21, 100);
    m_capillaryGlucoseInput = makeText(vitals, 6, 102, 212, 21, 100);

    // --- Bottom buttons. ---
    m_deleteButton = new QPushButton(content);
    m_deleteButton->setGeometry(560, 348, 137, 32);
    m_saveButton = new QPushButton(content);
    m_saveButton->setGeometry(714, 348, 137, 32);
    m_exitButton = new QPushButton(content);
    m_exitButton->setGeometry(867, 348, 137, 32);
    for (QPushButton* b : {m_deleteButton, m_saveButton, m_exitButton})
        b->setStyleSheet(buttonStyle);
    connect(m_deleteButton, &QPushButton::clicked, this, &AdultHistoryWindow::onDelete);
    connect(m_saveButton, &QPushButton::clicked, this, &AdultHistoryWindow::onSave);
    connect(m_exitButton, &QPushButton::clicked, this, &AdultHistoryWindow::onExit);
}

void AdultHistoryWindow::populateCombos() {
    QComboBox* combo = m_domesticAnimalsInput->comboBox();
    const int current = combo->currentData().toInt();
    combo->clear();
    for (int v = 0; v <= 3; ++v) {
        const auto value = static_cast<PediatricHistory::DomesticAnimals>(v);
        combo->addItem(domesticAnimalsLabel(value), v);
    }
    combo->setCurrentIndex(combo->findData(current));
}

void AdultHistoryWindow::retranslate() {
    setWindowTitle(tr("Adult History"));
    // The native bar is never visible: show the window title in the custom
    // bar instead of the application name.
    if (m_titleBar)
        m_titleBar->setTitle(windowTitle());

    auto* personal = m_nameInput->parentWidget();
    auto* adult = m_chronicLabel->parentWidget();
    auto* housing = m_potableWaterCheck->parentWidget();
    auto* vitals = m_weightInput->parentWidget();
    if (auto* g = qobject_cast<QGroupBox*>(personal)) g->setTitle(tr("Personal data"));
    if (auto* g = qobject_cast<QGroupBox*>(adult)) g->setTitle(tr("Adult history"));
    if (auto* g = qobject_cast<QGroupBox*>(housing)) g->setTitle(tr("Housing"));
    if (auto* g = qobject_cast<QGroupBox*>(vitals)) g->setTitle(tr("Vital signs"));

    m_nameInput->setLabelText(tr("Name:"));
    m_openingDateInput->setLabelText(tr("Opening date:"));
    m_allergiesInput->setLabelText(tr("Allergies:"));

    m_chronicLabel->setText(tr("Chronic diseases"));
    m_diabetesCheck->setText(tr("Diabetes"));
    m_diabetesTreatmentInput->setLabelText(tr("Treatment:"));
    m_hepatitisCheck->setText(tr("Hepatitis"));
    m_hepatitisTypeInput->setLabelText(tr("Type:"));
    m_hepatitisTreatmentInput->setLabelText(tr("Treatment:"));
    m_hivCheck->setText(QStringLiteral("HIV"));
    m_hivTreatmentInput->setLabelText(tr("Treatment:"));
    m_otherDiseasesInput->setLabelText(tr("Other diseases:"));
    m_occupationInput->setLabelText(tr("Occupation:"));
    m_childCountInput->setLabelText(tr("Number of children:"));
    m_disabilitiesInput->setLabelText(tr("Disabilities:"));
    m_conflictsInput->setLabelText(tr("Conflicts, relationships, family problems:"));

    m_potableWaterCheck->setText(tr("Potable water"));
    m_mosquitoNetCheck->setText(tr("Mosquito net"));
    m_netUseCheck->setText(tr("Net use"));
    m_latrineCheck->setText(tr("Latrine"));
    m_domesticAnimalsInput->setLabelText(tr("Domestic animals:"));
    m_domesticHygieneCheck->setText(tr("Domestic hygiene"));
    m_sanitaryControlCheck->setText(tr("Sanitary control"));

    m_weightInput->setLabelText(tr("Weight (Kg):"));
    m_temperatureInput->setLabelText(tr("Temperature (ºC):"));
    m_heightInput->setLabelText(tr("Height (cm):"));
    m_bmiInput->setLabelText(tr("BMI:"));
    m_abdominalPerimeterInput->setLabelText(tr("Abdominal P. (cm):"));
    m_consciousnessInput->setLabelText(tr("Consciousness:"));
    m_heartRateInput->setLabelText(tr("Heart rate:"));
    m_respiratoryRateInput->setLabelText(tr("Respiratory rate:"));
    m_capillaryGlucoseInput->setLabelText(tr("Capillary glucose:"));

    for (UxCheck* c : {m_diabetesCheck, m_hepatitisCheck, m_hivCheck,
                       m_potableWaterCheck, m_mosquitoNetCheck, m_netUseCheck,
                       m_latrineCheck, m_domesticHygieneCheck, m_sanitaryControlCheck})
        c->adjustSize();

    populateCombos();

    m_deleteButton->setText(tr("Delete history"));
    m_saveButton->setText(tr("Save history"));
    m_exitButton->setText(tr("Exit"));
}

bool AdultHistoryWindow::loadHistory() {
    return m_service.load(m_patientId, m_history, m_isNew);
}

void AdultHistoryWindow::loadIntoFields() {
    m_nameInput->setText(m_patientName);
    m_openingDateInput->setText(formatDate(m_history.openingDate));
    m_allergiesInput->setText(m_history.allergies);

    m_diabetesCheck->setChecked(m_history.historyDiabetes);
    m_diabetesTreatmentInput->setText(m_history.historyDiabetesTreatment);
    m_hepatitisCheck->setChecked(m_history.historyHepatitis);
    m_hepatitisTypeInput->setText(m_history.historyHepatitisType);
    m_hepatitisTreatmentInput->setText(m_history.historyHepatitisTreatment);
    m_hivCheck->setChecked(m_history.historyHiv);
    m_hivTreatmentInput->setText(m_history.historyHivTreatment);
    m_otherDiseasesInput->setText(m_history.historyOtherDiseases);
    m_occupationInput->setText(m_history.historyOccupation);
    m_childCountInput->setText(QString::number(m_history.historyChildCount));
    m_disabilitiesInput->setText(m_history.historyDisabilities);
    m_conflictsInput->setText(m_history.historyConflicts);

    m_potableWaterCheck->setChecked(m_history.housingPotableWater);
    m_mosquitoNetCheck->setChecked(m_history.housingMosquitoNet);
    m_netUseCheck->setChecked(m_history.housingMosquitoNetUse);
    m_latrineCheck->setChecked(m_history.housingLatrine);
    m_domesticAnimalsInput->comboBox()->setCurrentIndex(
        m_domesticAnimalsInput->comboBox()->findData(
            static_cast<int>(m_history.housingDomesticAnimals)));
    m_domesticHygieneCheck->setChecked(m_history.housingDomesticHygiene);
    m_sanitaryControlCheck->setChecked(m_history.housingSanitaryControl);

    m_weightInput->setText(formatDecimal(m_history.physicalExamWeight, 1));
    m_temperatureInput->setText(formatDecimal(m_history.physicalExamTemperature, 1));
    m_heightInput->setText(QString::number(m_history.physicalExamHeight));
    m_bmiInput->setText(formatDecimal(m_history.physicalExamBmi, 2));
    m_abdominalPerimeterInput->setText(QString::number(m_history.physicalExamAbdominalPerimeter));
    m_consciousnessInput->setText(m_history.physicalExamConsciousness);
    m_heartRateInput->setText(m_history.physicalExamHeartRate);
    m_respiratoryRateInput->setText(m_history.physicalExamRespiratoryRate);
    m_capillaryGlucoseInput->setText(m_history.physicalExamCapillaryGlucose);

    // A new history cannot be deleted yet.
    m_deleteButton->setEnabled(!m_isNew);
}

void AdultHistoryWindow::gatherFromFields(AdultHistory& history) const {
    history.patientId = m_patientId;
    history.openingDate = parseDate(m_openingDateInput->text());
    if (!history.openingDate.isValid())
        history.openingDate = QDate(1900, 1, 1);
    history.allergies = m_allergiesInput->text().trimmed();

    history.historyDiabetes = m_diabetesCheck->isChecked();
    history.historyDiabetesTreatment = m_diabetesTreatmentInput->text().trimmed();
    history.historyHepatitis = m_hepatitisCheck->isChecked();
    history.historyHepatitisType = m_hepatitisTypeInput->text().trimmed();
    history.historyHepatitisTreatment = m_hepatitisTreatmentInput->text().trimmed();
    history.historyHiv = m_hivCheck->isChecked();
    history.historyHivTreatment = m_hivTreatmentInput->text().trimmed();
    history.historyOtherDiseases = m_otherDiseasesInput->text().trimmed();
    history.historyOccupation = m_occupationInput->text().trimmed();
    history.historyChildCount = qBound(0, m_childCountInput->text().toInt(), 99);
    history.historyDisabilities = m_disabilitiesInput->text().trimmed();
    history.historyConflicts = m_conflictsInput->text().trimmed();

    history.housingPotableWater = m_potableWaterCheck->isChecked();
    history.housingMosquitoNet = m_mosquitoNetCheck->isChecked();
    history.housingMosquitoNetUse = m_netUseCheck->isChecked();
    history.housingLatrine = m_latrineCheck->isChecked();
    history.housingDomesticAnimals = static_cast<PediatricHistory::DomesticAnimals>(
        m_domesticAnimalsInput->comboBox()->currentData().toInt());
    history.housingDomesticHygiene = m_domesticHygieneCheck->isChecked();
    history.housingSanitaryControl = m_sanitaryControlCheck->isChecked();

    history.physicalExamWeight = qBound(0.0, parseDecimal(m_weightInput->text()), 999.9);
    history.physicalExamTemperature = qBound(0.0, parseDecimal(m_temperatureInput->text()), 99.9);
    history.physicalExamHeight = qBound(0, m_heightInput->text().toInt(), 999);
    history.physicalExamBmi = qBound(0.0, parseDecimal(m_bmiInput->text()), 99.99);
    history.physicalExamAbdominalPerimeter =
        qBound(0, m_abdominalPerimeterInput->text().toInt(), 999);
    history.physicalExamConsciousness = m_consciousnessInput->text().trimmed();
    history.physicalExamHeartRate = m_heartRateInput->text().trimmed();
    history.physicalExamRespiratoryRate = m_respiratoryRateInput->text().trimmed();
    history.physicalExamCapillaryGlucose = m_capillaryGlucoseInput->text().trimmed();
}

void AdultHistoryWindow::onSave() {
    AdultHistory history;
    gatherFromFields(history);
    if (m_service.save(m_isNew, history) != AdultHistoryService::SaveResult::Saved) {
        QMessageBox::critical(this, tr("Error"), tr("Could not save the history."));
        return;
    }
    accept();
}

void AdultHistoryWindow::onDelete() {
    if (m_isNew)
        return;
    if (QMessageBox::question(this, tr("Delete history"),
                              tr("Delete the adult history of \"%1\"?").arg(m_patientName))
        != QMessageBox::Yes)
        return;
    if (!m_service.remove(m_patientId)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not delete the history."));
        return;
    }
    accept();
}

void AdultHistoryWindow::onExit() {
    reject();
}

void AdultHistoryWindow::changeEvent(QEvent* event) {
    if (event && event->type() == QEvent::LanguageChange)
        retranslate();
    if (event && event->type() == QEvent::WindowStateChange && m_titleBar)
        m_titleBar->refreshMaximizeGlyph();
    QDialog::changeEvent(event);
}

} // namespace gambasse
