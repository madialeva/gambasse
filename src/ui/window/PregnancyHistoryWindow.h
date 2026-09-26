#pragma once

#include <QDialog>

#include <data/model/PregnancyHistory.h>
#include <logic/PregnancyHistoryService.h>

class QEvent;
class QGroupBox;
class QPushButton;
class QTabWidget;
class UxCheck;
class UxComboInput;
class UxDateInput;
class UxLabel;
class UxNumberInput;
class UxTextInput;

namespace gambasse {

class TitleBar;

// Pregnancy history window: clinical history of pregnant patients (client
// area 1008x561 with absolute control positions for small monitors).
// Frameless modal dialog with the custom title bar in close-only mode, built
// with custom uxwidgets controls (the treatment tab container is a themed
// QTabWidget, the only standard control). It opens the patient's existing
// history or a new one (opening date = today), saves (insert/update) and
// deletes it, then closes.
class PregnancyHistoryWindow : public QDialog {
    Q_OBJECT
public:
    explicit PregnancyHistoryWindow(qlonglong patientId, const QString& patientName,
                                    QWidget* parent = nullptr);

    // False when the history could not be loaded (an error was already shown).
    bool isValid() const { return m_valid; }

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onSave();
    void onDelete();
    void onExit();

private:
    void buildUi();
    void populateCombos();
    void retranslate();
    bool loadHistory();
    void loadIntoFields();
    void gatherFromFields(PregnancyHistory& history) const;

    TitleBar* m_titleBar = nullptr;

    // Personal data.
    UxTextInput* m_nameInput = nullptr;
    UxDateInput* m_openingDateInput = nullptr;
    UxTextInput* m_allergiesInput = nullptr;

    // Pregnancy history.
    UxDateInput* m_lastMenstruationInput = nullptr;
    UxNumberInput* m_deceasedSiblingCountInput = nullptr;
    UxDateInput* m_expectedDeliveryInput = nullptr;
    UxNumberInput* m_deliveryCountInput = nullptr;
    UxNumberInput* m_abortionCountInput = nullptr;
    UxTextInput* m_deliveryProblemsInput = nullptr;
    UxTextInput* m_gynecologicalDiseasesInput = nullptr;
    UxTextInput* m_barrierMethodsInput = nullptr;
    UxCheck* m_hivWomanCheck = nullptr;
    UxCheck* m_hivManCheck = nullptr;
    UxTextInput* m_serologiesInput = nullptr;

    // Medical history.
    UxLabel* m_chronicLabel = nullptr;
    UxCheck* m_diabetesCheck = nullptr;
    UxTextInput* m_diabetesTreatmentInput = nullptr;
    UxCheck* m_hepatitisCheck = nullptr;
    UxTextInput* m_hepatitisTypeInput = nullptr;
    UxTextInput* m_hepatitisTreatmentInput = nullptr;
    UxCheck* m_hypertensionCheck = nullptr;
    UxTextInput* m_hypertensionTreatmentInput = nullptr;
    UxTextInput* m_otherDiseasesInput = nullptr;
    UxTextInput* m_disabilitiesInput = nullptr;
    UxTextInput* m_conflictsInput = nullptr;

    // Housing.
    UxCheck* m_potableWaterCheck = nullptr;
    UxCheck* m_latrineCheck = nullptr;
    UxCheck* m_mosquitoNetParentsCheck = nullptr;
    UxCheck* m_netUseParentsCheck = nullptr;
    UxCheck* m_treatedWaterCheck = nullptr;
    UxCheck* m_domesticHygieneCheck = nullptr;
    UxCheck* m_mosquitoNetChildrenCheck = nullptr;
    UxCheck* m_netUseChildrenCheck = nullptr;
    UxComboInput* m_domesticAnimalsInput = nullptr;

    // Physical examination.
    UxNumberInput* m_weightInput = nullptr;
    UxNumberInput* m_temperatureInput = nullptr;
    UxNumberInput* m_heightInput = nullptr;
    UxNumberInput* m_abdominalPerimeterInput = nullptr;
    UxTextInput* m_vomitingInput = nullptr;
    UxTextInput* m_fetalAuscultationInput = nullptr;
    UxTextInput* m_fetalPositionInput = nullptr;
    UxNumberInput* m_uterineWeeksInput = nullptr;
    UxNumberInput* m_uterineCmInput = nullptr;
    UxCheck* m_leukocytosisCheck = nullptr;
    UxCheck* m_proteinuriaCheck = nullptr;
    UxCheck* m_mmiCheck = nullptr;
    UxCheck* m_faceCheck = nullptr;
    UxCheck* m_generalCheck = nullptr;

    // Previous treatments (5 rows of date / medication / dose).
    UxLabel* m_prevHeaderLabel = nullptr;
    UxDateInput* m_supplementDateInputs[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    UxTextInput* m_supplementMedicationInputs[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    UxTextInput* m_supplementDoseInputs[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};

    // Current treatments (dose / medication / next dose rows).
    UxTextInput* m_calciumInputs[3] = {nullptr, nullptr, nullptr};
    UxTextInput* m_ironInputs[3] = {nullptr, nullptr, nullptr}; // fixed display values
    UxTextInput* m_folicAcidInputs[3] = {nullptr, nullptr, nullptr}; // fixed display values
    UxTextInput* m_dewormingInputs[3] = {nullptr, nullptr, nullptr}; // fixed display values
    UxTextInput* m_vitaminCInputs[3] = {nullptr, nullptr, nullptr};
    UxTextInput* m_otherVitaminsInputs[3] = {nullptr, nullptr, nullptr};
    UxTextInput* m_malariaInputs[3] = {nullptr, nullptr, nullptr};
    UxLabel* m_nowRowLabels[7] = {nullptr, nullptr, nullptr, nullptr,
                                  nullptr, nullptr, nullptr};

    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_exitButton = nullptr;
    QTabWidget* m_treatmentTabs = nullptr;

    qlonglong m_patientId = 0;
    QString m_patientName;
    PregnancyHistory m_history;
    bool m_isNew = true;
    bool m_valid = false;
    PregnancyHistoryService m_service;
};

} // namespace gambasse
