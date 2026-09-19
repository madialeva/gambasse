#pragma once

#include <QDialog>

#include <data/model/PediatricHistory.h>
#include <logic/PediatricHistoryService.h>

class QEvent;
class QGroupBox;
class QPushButton;
class UxCheck;
class UxComboInput;
class UxDateInput;
class UxLabel;
class UxNumberInput;
class UxTextInput;

namespace gambasse {

class TitleBar;

// Pediatric history window: clinical history of pediatric patients (client
// area 1008x561 with absolute control positions for small monitors).
// Frameless modal dialog with the custom title bar in close-only mode, built
// only with custom uxwidgets controls. It opens the patient's existing
// history or a new one (opening date = today), saves (insert/update) and
// deletes it, then closes.
class PediatricHistoryWindow : public QDialog {
    Q_OBJECT
public:
    explicit PediatricHistoryWindow(qlonglong patientId, const QString& patientName,
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
    void gatherFromFields(PediatricHistory& history) const;

    TitleBar* m_titleBar = nullptr;

    // Personal data.
    UxTextInput* m_nameInput = nullptr;
    UxDateInput* m_openingDateInput = nullptr;
    UxTextInput* m_allergiesInput = nullptr;

    // Birth history.
    UxNumberInput* m_birthWeightInput = nullptr;
    UxNumberInput* m_birthHeightInput = nullptr;
    UxTextInput* m_deliveryIncidentsInput = nullptr;
    UxTextInput* m_malformationsInput = nullptr;

    // Child history.
    UxLabel* m_chronicLabel = nullptr;
    UxTextInput* m_diseaseTypeInput = nullptr;
    UxTextInput* m_diseaseTreatmentInput = nullptr;
    UxTextInput* m_otherDiseasesInput = nullptr;

    // Housing.
    UxCheck* m_latrineCheck = nullptr;
    UxCheck* m_mosquitoNetCheck = nullptr;
    UxCheck* m_netUseCheck = nullptr;
    UxCheck* m_domesticHygieneCheck = nullptr;
    UxComboInput* m_domesticAnimalsInput = nullptr;
    UxTextInput* m_conflictsInput = nullptr;
    UxTextInput* m_economicSituationInput = nullptr;

    // Feeding.
    UxCheck* m_breastfeedingCheck = nullptr;
    UxCheck* m_treatedWaterCheck = nullptr;
    UxTextInput* m_treatedWaterDetailsInput = nullptr;
    UxTextInput* m_feedingProblemsInput = nullptr;
    UxTextInput* m_additionalFeedingInput = nullptr;

    // Previous treatments.
    UxDateInput* m_supplementDateInputs[4] = {nullptr, nullptr, nullptr, nullptr};
    UxComboInput* m_supplementTreatmentInputs[4] = {nullptr, nullptr, nullptr, nullptr};

    // Vaccines.
    UxLabel* m_vaccineRowLabels[6] = {nullptr, nullptr, nullptr,
                                      nullptr, nullptr, nullptr};
    UxCheck* m_bcgCheck = nullptr;
    UxCheck* m_opvChecks[4] = {nullptr, nullptr, nullptr, nullptr};
    UxCheck* m_dptHibChecks[3] = {nullptr, nullptr, nullptr};
    UxCheck* m_hepBChecks[3] = {nullptr, nullptr, nullptr};
    UxCheck* m_measles9Check = nullptr;
    UxCheck* m_measles10Check = nullptr;

    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_exitButton = nullptr;

    qlonglong m_patientId = 0;
    QString m_patientName;
    PediatricHistory m_history;
    bool m_isNew = true;
    bool m_valid = false;
    PediatricHistoryService m_service;
};

} // namespace gambasse
