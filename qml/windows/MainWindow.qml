pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import Gambasse

// Patient master-detail screen (mirrors the Widgets MainWindow): entry
// toolbar, filterable grid, photo with history creation buttons, detail
// panel with in-place CRUD. Clinical logic lives in PatientController;
// this view only renders values, gathers the draft and shows messages.
Item {
    id: root

    property PatientController controller: null
    // A history or consultation window is open in front of this screen.
    readonly property bool formWindowOpen: historyWindow.visible || consultationWindow.visible
    // User-typed filter per column (survives language-driven rebuilds).
    property var filterTexts: ["", "", "", "", "", "", "", ""]
    // Fixed column widths (identifier column narrow); the last stretches.
    property var baseWidths: [60, 170, 95, 55, 75, 170, 120, 120]
    // Numeric columns (code and age) are centred.
    readonly property var centredColumns: [0, 3]

    // Width of each bold header title with its sort arrow, measured by laying
    // it out exactly as the header draws it (the arrow may come from a
    // fallback font, which font metrics alone misjudge), so no title is ever
    // cut in any language (the code column used to cut "Código").
    // Controls take their font from the controls theme, not from the
    // application font a bare Text uses: the probes borrow it from a hidden
    // button set up like the header cells.
    Button {
        id: headerFontSource
        visible: false
        font.bold: true
    }
    Repeater {
        id: headerProbes
        model: root.hasController() ? root.controller.columnNames : []
        onItemAdded: root.updateHeaderWidths()
        onItemRemoved: root.updateHeaderWidths()
        Text {
            required property string modelData
            visible: false
            font: headerFontSource.font
            text: modelData + " \u25B2"
            onImplicitWidthChanged: root.updateHeaderWidths()
        }
    }

    // Measured header widths (with the 6 px side padding of the header
    // cells). A notifying property, so the header row, the filter boxes and
    // the grid follow a new language or font.
    property var headerWidths: []

    function updateHeaderWidths() {
        const widths = [];
        for (let i = 0; i < headerProbes.count; ++i) {
            const probe = headerProbes.itemAt(i) as Text;
            widths.push(probe ? Math.ceil(probe.implicitWidth) + 12 : 0);
        }
        headerWidths = widths;
    }

    // The table only asks columnWidthProvider again when told to.
    onHeaderWidthsChanged: gridTable.forceLayout()

    function headerWidth(col) {
        return col < headerWidths.length ? headerWidths[col] : 0;
    }

    function fixedWidth(col) {
        return Math.max(baseWidths[col], headerWidth(col));
    }

    function columnWidth(col) {
        if (col < 7)
            return fixedWidth(col);
        let used = 0;
        for (let c = 0; c < 7; ++c)
            used += fixedWidth(c);
        // The last column takes the rest, minus the vertical scroll bar and
        // the two sides of the grid frame.
        return Math.max(Math.max(baseWidths[7], headerWidth(7)), listArea.width - used - 14 - 2);
    }

    function columnAlignment(col) {
        return centredColumns.indexOf(col) >= 0 ? Text.AlignHCenter : Text.AlignLeft;
    }

    function hasController() {
        return controller !== null && controller !== undefined;
    }

    // The controller owns the patient selection, so the highlight follows it
    // instead of the selection model, which Qt resets on every click.
    function isCurrentRow(row) {
        return hasController() && controller.currentRow === row;
    }

    function loadDetail() {
        if (!hasController())
            return;
        codeLabel.text = qsTr("Code:") + (controller.patientId > 0 ? " " + controller.patientId : "");
        nameInput.text = controller.patientName;
        populateSex();
        dateInput.text = controller.birthDateText;
        ageInput.text = "" + controller.ageRange;
        addressInput.text = controller.address;
        cohabitantsInput.text = controller.cohabitants;
        contactInput.text = controller.contactPerson;
        siblingsInput.text = "" + controller.siblingCount;
    }

    function populateSex() {
        if (!hasController())
            return;
        const current = sexInput.currentData();
        sexInput.clear();
        sexInput.addItem(controller.sexHomeLabel, 0);
        sexInput.addItem(controller.sexMullerLabel, 1);
        const wanted = sexInput.findData(controller.patientSex);
        sexInput.currentIndex = wanted >= 0 ? wanted : sexInput.findData(current);
    }

    function syncSelection() {
        if (!hasController())
            return;
        const r = controller.currentRow;
        if (gridSelection.model === null || gridSelection.model === undefined)
            return;
        if (r >= 0 && gridSelection.currentIndex.row !== r)
            gridSelection.setCurrentIndex(controller.gridModel.index(r, 0), ItemSelectionModel.Rows | ItemSelectionModel.Select | ItemSelectionModel.Current);
        else if (r < 0)
            gridSelection.clear();
    }

    function gatherSave() {
        const result = controller.saveDraft(nameInput.text, sexInput.currentData(), dateInput.text, parseInt(ageInput.text) || 0, addressInput.text, cohabitantsInput.text, contactInput.text, parseInt(siblingsInput.text) || 0);
        if (result === 1) {
            messageDialog.showInfo(qsTr("Incomplete data"), qsTr("Name is required."));
            nameInput.focusField();
        } else if (result === 2) {
            messageDialog.showInfo(qsTr("Duplicate patient"), qsTr("A patient with the same name, birth date, sex, and age already exists."));
        } else if (result === 3) {
            messageDialog.showInfo(qsTr("Error"), qsTr("Could not save the patient."));
        }
    }

    Connections {
        target: root.controller
        function onOpenHistoryScreen(screen, historyController) {
            historyWindow.screenId = screen;
            historyWindow.screenTitle = screen === "adult" ? qsTr("Adult History") : screen === "pediatric" ? qsTr("Pediatric History") : qsTr("Pregnancy History");
            historyWindow.controller = historyController;
            historyWindow.present();
        }
        function onOpenConsultationScreen(screen, consultationController) {
            consultationWindow.screenId = screen;
            consultationWindow.screenTitle = screen === "adult" ? qsTr("Adult Consultation") : screen === "pediatric" ? qsTr("Pediatric Consultation") : qsTr("Pregnancy Consultation");
            consultationWindow.controller = consultationController;
            consultationWindow.present();
        }
        function onEditingChanged() {
            // Saving or cancelling gives the keyboard back to the grid.
            if (!root.controller.editing)
                gridTable.forceActiveFocus();
        }
        function onDetailChanged() {
            root.loadDetail();
        }
        function onHeadersChanged() {
            root.populateSex();
        }
        function onCurrentChanged() {
            root.syncSelection();
        }
    }
    onControllerChanged: {
        root.loadDetail();
        root.syncSelection();
        // Focus the grid once the controller is in place, so the keyboard and
        // the wheel act on the patient list without clicking it first.
        gridTable.forceActiveFocus();
    }
    Component.onCompleted: {
        root.loadDetail();
        root.syncSelection();
    }

    component ActionButton: Button {
        id: actionButton
        font.bold: true
        background: Rectangle {
            color: !actionButton.enabled ? Theme.accentDisabledBackground : (actionButton.hovered ? Theme.accentHover : Theme.accentBackground)
            border.color: !actionButton.enabled ? Theme.accentDisabledText : Theme.accentBorder
            radius: 4
        }
        contentItem: Text {
            text: actionButton.text
            color: !actionButton.enabled ? Theme.accentDisabledText : Theme.accentText
            font: actionButton.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        // History/consultation entry buttons (visible only with history).
        Row {
            spacing: 6
            ActionButton {
                objectName: "entryPediatricHistory"
                visible: root.hasController() && root.controller.showPediatric
                text: qsTr("Pediatric history")
                onClicked: root.controller.openPediatricHistory()
            }
            ActionButton {
                objectName: "entryPediatricConsultation"
                visible: root.hasController() && root.controller.showPediatric
                text: qsTr("Pediatric consultation")
                onClicked: root.controller.openPediatricConsultation()
            }
            ActionButton {
                objectName: "entryAdultHistory"
                visible: root.hasController() && root.controller.showAdult
                text: qsTr("Adult history")
                onClicked: root.controller.openAdultHistory()
            }
            ActionButton {
                objectName: "entryAdultConsultation"
                visible: root.hasController() && root.controller.showAdult
                text: qsTr("Adult consultation")
                onClicked: root.controller.openAdultConsultation()
            }
            ActionButton {
                objectName: "entryPregnancyHistory"
                visible: root.hasController() && root.controller.showPregnancy
                text: qsTr("Pregnancy history")
                onClicked: root.controller.openPregnancyHistory()
            }
            ActionButton {
                objectName: "entryPregnancyConsultation"
                visible: root.hasController() && root.controller.showPregnancy
                text: qsTr("Pregnancy consultation")
                onClicked: root.controller.openPregnancyConsultation()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            // Patient list with live column filters.
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 2
                enabled: root.hasController() && !root.controller.editing

                Item {
                    id: listArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 2

                        // Header, filter row and rows inside a 1 px frame, like the grid
                        // of the original application.
                        Item {
                            objectName: "gridFrame"
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 1
                                spacing: 2

                                // Header row with sort indicators.
                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 28
                                    clip: true
                                    RowLayout {
                                        x: -gridTable.contentX
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        spacing: 0
                                        Repeater {
                                            objectName: "headerRow"
                                            model: root.hasController() ? root.controller.columnNames : []
                                            Button {
                                                id: headerButton
                                                required property int index
                                                required property string modelData
                                                objectName: "headerCell"
                                                Layout.preferredWidth: root.columnWidth(index)
                                                Layout.preferredHeight: 28
                                                flat: true
                                                // 6 px each side, the 12 px headerWidth() reserves.
                                                leftPadding: 6
                                                rightPadding: 6
                                                font.bold: true
                                                text: modelData + (root.hasController() && root.controller.sortColumn === index ? (root.controller.sortOrder === 0 ? " \u25B2" : " \u25BC") : "")
                                                contentItem: Text {
                                                    horizontalAlignment: root.columnAlignment(headerButton.index)
                                                    text: headerButton.text
                                                    color: Theme.windowText
                                                    font: headerButton.font
                                                    elide: Text.ElideRight
                                                }
                                                background: Rectangle {
                                                    color: Theme.windowBackground
                                                }
                                                onClicked: root.controller.sortByColumn(index)
                                            }
                                        }
                                    }
                                }

                                // Filter row inside the grid, between the header and the
                                // first row: one box per column, aligned with it.
                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 26
                                    clip: true
                                    RowLayout {
                                        x: -gridTable.contentX
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        spacing: 0
                                        Repeater {
                                            objectName: "filterBoxes"
                                            model: root.hasController() ? root.controller.columnNames : []
                                            GxTextInput {
                                                required property int index
                                                required property string modelData
                                                objectName: "filterBox"
                                                Layout.preferredWidth: root.columnWidth(index)
                                                Layout.preferredHeight: 26
                                                placeholderText: modelData
                                                text: root.filterTexts[index]
                                                onTextEdited: {
                                                    root.filterTexts[index] = text;
                                                    root.controller.setColumnFilter(index, text);
                                                }
                                            }
                                        }
                                    }
                                }

                                TableView {
                                    id: gridTable
                                    objectName: "gridTable"
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true
                                    // The grid takes the focus when the screen opens, so
                                    // the arrow keys and the wheel work right away.
                                    focus: true
                                    keyNavigationEnabled: true
                                    // A data grid must stop at the ends: the default
                                    // overscroll left a blank band above the first row
                                    // that sprang back on its own.
                                    boundsBehavior: Flickable.StopAtBounds
                                    model: root.hasController() ? root.controller.gridModel : null
                                    selectionModel: ItemSelectionModel {
                                        id: gridSelection
                                        model: root.hasController() ? root.controller.gridModel : null
                                        // The model can arrive after the controller (it is
                                        // injected from C++), so the grid selection is
                                        // (re)applied whenever the model is set.
                                        onModelChanged: root.syncSelection()
                                        onCurrentChanged: (current, previous) => {
                                            if (root.hasController() && current.row !== root.controller.currentRow)
                                                root.controller.selectRow(current.row);
                                        }
                                    }
                                    selectionBehavior: TableView.SelectRows
                                    selectionMode: TableView.SingleSelection
                                    columnWidthProvider: function (col) {
                                        return root.columnWidth(col);
                                    }
                                    rowHeightProvider: function (row) {
                                        return 24;
                                    }
                                    ScrollBar.vertical: ScrollBar {
                                        policy: ScrollBar.AlwaysOn
                                    }
                                    ScrollBar.horizontal: ScrollBar {
                                        policy: ScrollBar.AsNeeded
                                    }
                                    delegate: Rectangle {
                                        id: cell
                                        objectName: "gridCell"
                                        required property string display
                                        required property int row
                                        required property int column
                                        readonly property bool current: root.isCurrentRow(cell.row)
                                        color: cell.current ? Theme.highlight : (cell.row % 2 ? Theme.alternateRowBackground : Theme.fieldBackground)
                                        Rectangle {
                                            anchors.bottom: parent.bottom
                                            width: parent.width
                                            height: 1
                                            color: Theme.fieldBorder
                                        }
                                        Text {
                                            anchors.fill: parent
                                            anchors.leftMargin: 4
                                            anchors.rightMargin: 4
                                            horizontalAlignment: root.columnAlignment(cell.column)
                                            verticalAlignment: Text.AlignVCenter
                                            elide: Text.ElideRight
                                            color: cell.current ? Theme.highlightedText : Theme.windowText
                                            text: cell.display
                                        }
                                    }
                                }
                            }

                            // Drawn above the cells so a scrolled row never covers it.
                            Rectangle {
                                objectName: "gridBorder"
                                anchors.fill: parent
                                z: 1
                                color: "transparent"
                                border.color: Theme.inputBorder
                                border.width: 1
                            }
                        }
                    }
                }
            }

            // Photo with history creation buttons below.
            ColumnLayout {
                Layout.preferredWidth: 240
                Layout.fillHeight: true
                spacing: 6
                // 240x204 like pbxFotoPaciente in the original application: the
                // picture is centred at its natural size (CenterImage) and only
                // reduced, keeping its proportions, when it is larger.
                Item {
                    objectName: "photoFrame"
                    Layout.preferredWidth: 240
                    Layout.preferredHeight: 204
                    Image {
                        id: photo
                        objectName: "photoImage"
                        readonly property real fit: implicitWidth > 0 && implicitHeight > 0 ? Math.min(1, parent.width / implicitWidth, parent.height / implicitHeight) : 1
                        anchors.centerIn: parent
                        width: Math.round(implicitWidth * fit)
                        height: Math.round(implicitHeight * fit)
                        smooth: true
                        source: root.hasController() ? root.controller.photoSource : ""
                    }
                    // 1 px border like the grid's, drawn above the picture so a
                    // photo that fills the frame never hides it.
                    Rectangle {
                        objectName: "photoBorder"
                        anchors.fill: parent
                        z: 1
                        color: "transparent"
                        border.color: Theme.inputBorder
                        border.width: 1
                    }
                }
                ActionButton {
                    objectName: "createPediatricButton"
                    Layout.preferredWidth: 240
                    text: qsTr("Create pediatric history")
                    enabled: root.hasController() && root.controller.canCreatePediatric
                    onClicked: root.controller.openPediatricHistory()
                }
                ActionButton {
                    objectName: "createAdultButton"
                    Layout.preferredWidth: 240
                    text: qsTr("Create adult history")
                    enabled: root.hasController() && root.controller.canCreateAdult
                    onClicked: root.controller.openAdultHistory()
                }
                ActionButton {
                    objectName: "createPregnancyButton"
                    Layout.preferredWidth: 240
                    text: qsTr("Create pregnancy history")
                    enabled: root.hasController() && root.controller.canCreatePregnancy
                    onClicked: root.controller.openPregnancyHistory()
                }
                // Takes the spare height, so the photo lines up with the top of
                // the grid and the history buttons follow right below it.
                Item {
                    Layout.fillHeight: true
                }
            }
        }

        // Detail panel: the action row on top and both groups side by side
        // below, the same structure as the Widgets window, so the two boxes
        // share the same height and vertical position. Half the width each:
        // zero preferred width makes the layout share the extra space evenly
        // instead of following their implicit widths.
        ColumnLayout {
            objectName: "detailPanel"
            Layout.fillWidth: true
            spacing: 4
            Row {
                objectName: "detailActions"
                spacing: 6
                ActionButton {
                    objectName: "addButton"
                    text: qsTr("Add")
                    enabled: root.hasController() && !root.controller.editing
                    onClicked: {
                        root.controller.startAdd();
                        nameInput.focusField();
                    }
                }
                ActionButton {
                    objectName: "editButton"
                    text: qsTr("Edit")
                    enabled: root.hasController() && !root.controller.editing
                    onClicked: {
                        root.controller.startEdit();
                        nameInput.focusField();
                    }
                }
                ActionButton {
                    objectName: "deleteButton"
                    text: qsTr("Delete")
                    enabled: root.hasController() && !root.controller.editing
                    onClicked: {
                        deleteConfirm.messageText = qsTr("Delete patient \"%1\" and all their histories and consultations?").arg(root.controller.patientName);
                        deleteConfirm.yesNo = true;
                        deleteConfirm.titleText = qsTr("Delete patient");
                        deleteConfirm.open();
                    }
                }
                ActionButton {
                    objectName: "saveButton"
                    text: qsTr("Save")
                    enabled: root.hasController() && root.controller.editing
                    onClicked: root.gatherSave()
                }
                ActionButton {
                    objectName: "cancelButton"
                    text: qsTr("Cancel")
                    enabled: root.hasController() && root.controller.editing
                    onClicked: root.controller.cancelEdit()
                }
            }
            RowLayout {
                objectName: "detailGroups"
                Layout.fillWidth: true
                spacing: 8
                GxGroup {
                    id: basicDataGroup
                    objectName: "basicDataGroup"
                    title: qsTr("Basic data")
                    Layout.fillWidth: true
                    // As tall as the taller content of the two groups, rows packed
                    // at the field height, so the panel fits the base window.
                    Layout.preferredHeight: Math.max(basicDataLayout.implicitHeight, addressLayout.implicitHeight) + basicDataGroup.contentTop + 2 * basicDataGroup.padding
                    Layout.preferredWidth: 0
                    GridLayout {
                        id: basicDataLayout
                        anchors.fill: basicDataGroup.body
                        columns: 1
                        rowSpacing: 4
                        // Line 1: the identifier (read-only).
                        GxLabel {
                            id: codeLabel
                            objectName: "codeLabel"
                            Layout.fillWidth: true
                            Layout.preferredHeight: Theme.fieldHeight
                        }
                        // Line 2: the name on its own.
                        GxTextInput {
                            id: nameInput
                            objectName: "nameInput"
                            labelText: qsTr("Name:")
                            uppercase: true
                            required: true
                            maxLength: 200
                            Layout.fillWidth: true
                            Layout.preferredHeight: Theme.fieldHeight
                            enabled: root.hasController() && root.controller.editing
                        }
                        // Line 3: age (three digits), birth date and sex. The
                        // date takes what is left so it shows a full date.
                        GridLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: Theme.fieldHeight
                            columns: 3
                            columnSpacing: 6
                            GxNumberInput {
                                id: ageInput
                                objectName: "ageInput"
                                labelText: qsTr("Age:")
                                integerDigits: 3
                                Layout.preferredWidth: 84
                                Layout.preferredHeight: Theme.fieldHeight
                                enabled: root.hasController() && root.controller.editing
                            }
                            GxDateInput {
                                id: dateInput
                                objectName: "dateInput"
                                labelText: qsTr("Birth date:")
                                Layout.fillWidth: true
                                Layout.preferredHeight: Theme.fieldHeight
                                enabled: root.hasController() && root.controller.editing
                            }
                            GxComboInput {
                                id: sexInput
                                objectName: "sexInput"
                                labelText: qsTr("Sex:")
                                Layout.minimumWidth: 150
                                Layout.preferredWidth: 150
                                Layout.preferredHeight: Theme.fieldHeight
                                enabled: root.hasController() && root.controller.editing
                            }
                        }
                    }
                }
                GxGroup {
                    id: addressGroup
                    objectName: "addressGroup"
                    title: qsTr("Address")
                    Layout.fillWidth: true
                    // As tall as the taller content of the two groups, rows packed
                    // at the field height, so the panel fits the base window.
                    Layout.preferredHeight: Math.max(basicDataLayout.implicitHeight, addressLayout.implicitHeight) + addressGroup.contentTop + 2 * addressGroup.padding
                    Layout.preferredWidth: 0
                    GridLayout {
                        id: addressLayout
                        anchors.fill: addressGroup.body
                        columns: 1
                        rowSpacing: 4
                        GxTextInput {
                            id: addressInput
                            objectName: "addressInput"
                            labelText: qsTr("Address:")
                            maxLength: 100
                            Layout.fillWidth: true
                            Layout.preferredHeight: Theme.fieldHeight
                            enabled: root.hasController() && root.controller.editing
                        }
                        // Line 2: cohabitants and contact side by side.
                        GridLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: Theme.fieldHeight
                            columns: 2
                            columnSpacing: 6
                            GxTextInput {
                                id: cohabitantsInput
                                objectName: "cohabitantsInput"
                                labelText: qsTr("Cohabitants:")
                                maxLength: 100
                                Layout.fillWidth: true
                                Layout.preferredHeight: Theme.fieldHeight
                                enabled: root.hasController() && root.controller.editing
                            }
                            GxTextInput {
                                id: contactInput
                                objectName: "contactInput"
                                labelText: qsTr("Contact:")
                                maxLength: 100
                                Layout.fillWidth: true
                                Layout.preferredHeight: Theme.fieldHeight
                                enabled: root.hasController() && root.controller.editing
                            }
                        }
                        // Line 3: the number of siblings, after the contact.
                        GxNumberInput {
                            id: siblingsInput
                            objectName: "siblingsInput"
                            labelText: qsTr("Siblings:")
                            integerDigits: 2
                            Layout.preferredWidth: 116
                            Layout.preferredHeight: Theme.fieldHeight
                            enabled: root.hasController() && root.controller.editing
                        }
                    }
                }
            }
        }
    }

    // History screen: PatientController prepares the data and asks for it
    // through openHistoryScreen; the window here is the QML presentation.
    HistoryWindow {
        id: historyWindow
        objectName: "historyWindow"
        // Saving or deleting a history changes what the patient screen offers,
        // like the availability refresh the Widgets window did when it closed.
        onHistoryChanged: {
            if (root.hasController())
                root.controller.refreshAvailability();
        }
        formComponent: historyWindow.screenId === "pediatric" ? pediatricForm : historyWindow.screenId === "pregnancy" ? pregnancyForm : adultForm
    }
    Component {
        id: adultForm
        AdultHistoryForm {
        }
    }
    Component {
        id: pediatricForm
        PediatricHistoryForm {
        }
    }
    Component {
        id: pregnancyForm
        PregnancyHistoryForm {
        }
    }

    // Consultation screen: same pattern as the history screen. Consultations
    // do not change what the patient screen offers, so nothing is refreshed.
    ConsultationWindow {
        id: consultationWindow
        objectName: "consultationWindow"
        formComponent: consultationWindow.screenId === "pediatric" ? pediatricConsultationForm : consultationWindow.screenId === "pregnancy" ? pregnancyConsultationForm : adultConsultationForm
    }
    Component {
        id: adultConsultationForm
        AdultConsultationForm {
        }
    }
    Component {
        id: pregnancyConsultationForm
        PregnancyConsultationForm {
        }
    }
    Component {
        id: pediatricConsultationForm
        PediatricConsultationForm {
        }
    }

    GxDialog {
        id: messageDialog
        objectName: "messageDialog"
        parent: Overlay.overlay
    }
    GxDialog {
        id: deleteConfirm
        objectName: "deleteConfirm"
        parent: Overlay.overlay
        onAccepted: {
            if (!root.controller.removeCurrent())
                messageDialog.showInfo(qsTr("Error"), qsTr("Could not delete the patient."));
        }
    }
}
