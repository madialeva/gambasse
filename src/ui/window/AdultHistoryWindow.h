#pragma once

#include <QDialog>

#include <data/model/AdultHistory.h>
#include <logic/AdultHistoryService.h>

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

// Adult history window: clinical history of adult patients (client area
// 1008x383 with absolute control positions for small monitors).
// Frameless modal dialog with the custom title bar in close-only mode, built
// only with custom uxwidgets controls. It opens the patient's existing
// history or a new one (opening date = today), saves (insert/update) and
// deletes it, then closes.
class AdultHistoryWindow : public QDialog {
    Q_OBJECT
public:
    explicit AdultHistoryWindow(qlonglong patientId, const QString& patientName,
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
    void gatherFromFields(AdultHistory& history) const;

    TitleBar* m_titleBar = nullptr;

    // Personal data.
    UxTextInput* m_nameInput = nullptr;
    UxDateInput* m_openingDateInput = nullptr;
    UxTextInput* m_allergiesInput = nullptr;

    // Adult history.
    UxLabel* m_chronicLabel = nullptr;
    UxCheck* m_diabetesCheck = nullptr;
    UxTextInput* m_diabetesTreatmentInput = nullptr;
    UxCheck* m_hepatitisCheck = nullptr;
    UxTextInput* m_hepatitisTypeInput = nullptr;
    UxTextInput* m_hepatitisTreatmentInput = nullptr;
    UxCheck* m_hivCheck = nullptr;
    UxTextInput* m_hivTreatmentInput = nullptr;
    UxTextInput* m_otherDiseasesInput = nullptr;
    UxTextInput* m_occupationInput = nullptr;
    UxNumberInput* m_childCountInput = nullptr;
    UxTextInput* m_disabilitiesInput = nullptr;
    UxTextInput* m_conflictsInput = nullptr;

    // Housing.
    UxCheck* m_potableWaterCheck = nullptr;
    UxCheck* m_mosquitoNetCheck = nullptr;
    UxCheck* m_netUseCheck = nullptr;
    UxCheck* m_latrineCheck = nullptr;
    UxComboInput* m_domesticAnimalsInput = nullptr;
    UxCheck* m_domesticHygieneCheck = nullptr;
    UxCheck* m_sanitaryControlCheck = nullptr;

    // Vital signs.
    UxNumberInput* m_weightInput = nullptr;
    UxNumberInput* m_temperatureInput = nullptr;
    UxNumberInput* m_heightInput = nullptr;
    UxNumberInput* m_bmiInput = nullptr;
    UxNumberInput* m_abdominalPerimeterInput = nullptr;
    UxTextInput* m_consciousnessInput = nullptr;
    UxTextInput* m_heartRateInput = nullptr;
    UxTextInput* m_respiratoryRateInput = nullptr;
    UxTextInput* m_capillaryGlucoseInput = nullptr;

    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_exitButton = nullptr;

    qlonglong m_patientId = 0;
    QString m_patientName;
    AdultHistory m_history;
    bool m_isNew = true;
    bool m_valid = false;
    AdultHistoryService m_service;
};

} // namespace gambasse
