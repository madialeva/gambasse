#pragma once

#include <QMainWindow>
#include <QVector>
#include <QTranslator>

#include <data/Patient.h>

class QTableView;
class QLineEdit;
class QLabel;
class QPushButton;
class QGroupBox;
class QAction;
class QMenu;
class QEvent;
class QToolBar;
class QToolButton;
class UxTextInput;
class UxNumberInput;
class UxDateInput;
class UxComboInput;
class UxLabel;

namespace gambasse {

class PatientsModel;
class ColumnFilterProxy;
class TitleBar;

// Main screen (equivalent to FrmInicial), redesigned as a master-detail view
// with in-panel patient CRUD.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onSelectionChanged();
    void onActionNotImplemented();
    void onAdd();
    void onEdit();
    void onDelete();
    void onSave();
    void onCancel();

private:
    void buildUi();
    QWidget* buildListPanel();
    void buildToolbar();

    void positionFilters();
    void loadIntoFields(const Patient* p);
    void gatherFromFields(Patient& p) const;
    void setEditMode(bool editing);
    void alignDetailLabels();
    int  selectedSourceRow() const;
    const Patient* selectedPatient() const;
    void updateContextButtons(const Patient* p);
    void loadPhoto(const Patient* p);
    void clearDetail();

    void changeLanguage(const QString& code);
    void updateLanguageButton();
    void changeTheme(const QString& mode);
    void applyTheme(const QString& mode);
    void styleToolbarButtons();
    void updateThemeButton();
    void retranslate();

    // Manual edge resizing for the frameless window (the event filter on the
    // title bar and the central widget feeds these).
    int resizeEdgesAt(const QPoint& globalPos) const;
    static Qt::CursorShape cursorForEdges(int edges);

    // Data
    PatientsModel*     m_model = nullptr;
    ColumnFilterProxy* m_proxy = nullptr;

    // List and filter
    QTableView*           m_table = nullptr;
    QWidget*              m_filterRow = nullptr;
    QVector<QLineEdit*>   m_filters;

    // Detail: title labels and editable fields.
    QGroupBox* m_dataGroup = nullptr;
    QGroupBox* m_addressGroup = nullptr;
    QLabel*   m_photo = nullptr;

    // Identifier field (read-only; never editable).
    UxLabel* m_codeCaption = nullptr;
    UxLabel* m_codeValue = nullptr;

    // Editable detail fields (custom UxWidgets composite controls with an
    // integrated label).
    UxTextInput*   m_nameInput = nullptr;
    UxComboInput*  m_sexInput = nullptr;
    UxDateInput*   m_dateInput = nullptr;
    UxNumberInput* m_ageInput = nullptr;
    UxTextInput*   m_addressInput = nullptr;
    UxTextInput*   m_cohabitantsInput = nullptr;
    UxTextInput*   m_contactInput = nullptr;
    UxNumberInput* m_siblingsInput = nullptr;

    // Patient CRUD bar above "Basic data".
    QPushButton* m_addButton = nullptr;
    QPushButton* m_editButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_cancelButton = nullptr;

    // Editing state.
    bool m_isEditing = false;
    int  m_editingRow = -1;     // source row being edited; -1 = new patient
    Patient m_previousPatient;   // previous values, for photo renaming

    // History/consultation entry buttons in the left side of the toolbar.
    // Keep the QAction returned by QToolBar::addWidget(): it, not the widget,
    // controls effective visibility in a toolbar.
    QToolButton* m_pediatricHistoryButton = nullptr;
    QToolButton* m_pediatricConsultationButton = nullptr;
    QToolButton* m_adultHistoryButton = nullptr;
    QToolButton* m_adultConsultationButton = nullptr;
    QToolButton* m_pregnancyHistoryButton = nullptr;
    QToolButton* m_pregnancyConsultationButton = nullptr;
    QAction* m_pediatricHistoryAction = nullptr;
    QAction* m_pediatricConsultationAction = nullptr;
    QAction* m_adultHistoryAction = nullptr;
    QAction* m_adultConsultationAction = nullptr;
    QAction* m_pregnancyHistoryAction = nullptr;
    QAction* m_pregnancyConsultationAction = nullptr;
    // History creation buttons below the detail sections.
    QPushButton* m_createPediatricButton = nullptr;
    QPushButton* m_createAdultButton = nullptr;
    QPushButton* m_createPregnancyButton = nullptr;

    // Custom title bar (frameless window) hosting logo, name, language,
    // theme and window controls.
    TitleBar* m_titleBar = nullptr;
    // Entry-button toolbar below the title bar (a plain layout widget, so it
    // stays under the custom bar instead of docking above the central area).
    QToolBar* m_toolbar = nullptr;

    // Active manual resize state (NoEdge = not resizing).
    int m_resizeEdges = 0;
    QPoint m_resizeStartPos;
    QRect m_resizeStartGeometry;

    // Toolbar language and theme controls (hosted by the title bar).
    QToolButton* m_languageButton = nullptr;
    QToolButton* m_themeButton = nullptr;
    QAction* m_actEs = nullptr;
    QAction* m_actPt = nullptr;
    QAction* m_lightAction = nullptr;
    QAction* m_darkAction = nullptr;

    // i18n and theme.
    QTranslator m_translator;
    QString m_languageCode = QStringLiteral("pt");
    QString m_theme = QStringLiteral("claro");
};

} // namespace gambasse
