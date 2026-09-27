import QtQuick
import QtQuick.Controls
import Gambasse

// Modal shell shared by the three history screens (pediatric, adult and
// pregnancy): frameless window with the close-only title bar, the form loaded
// in the middle and the Delete / Save / Exit row at the bottom, exactly like
// the Widgets history dialogs. The controller decides what the form shows and
// performs the persistence; this window only wires the buttons and the
// confirmation.
Window {
    id: root

    // PediatricHistoryController, AdultHistoryController or
    // PregnancyHistoryController, created by PatientController.
    property HistoryController controller: null
    property string screenTitle: ""
    // Which history this window shows ("pediatric", "adult", "pregnancy").
    // Not named "screen": Window already declares a read-only screen
    // property (the QScreen), which cannot be replaced.
    property string screenId: ""
    // Confirmation text, kept as three separate strings so the catalogs reuse
    // the wording the Widgets windows already showed.
    function deleteQuestion(name) {
        if (root.screenId === "pediatric")
            return qsTr("Delete the pediatric history of \"%1\"?").arg(name);
        if (root.screenId === "pregnancy")
            return qsTr("Delete the pregnancy history of \"%1\"?").arg(name);
        return qsTr("Delete the adult history of \"%1\"?").arg(name);
    }
    // Source component of the form that matches the controller.
    property var formComponent: null
    readonly property bool ready: controller !== null && controller.loaded
    // Emitted after a successful save or delete, so the patient screen can
    // refresh what it offers (Window.closed is not a QML signal).
    signal historyChanged

    visible: false
    modality: Qt.WindowModal
    flags: Qt.FramelessWindowHint
    title: root.screenTitle
    color: Theme.windowBackground
    // Same fixed frame as the Widgets dialogs: 6 px side margins around a
    // 1008 px content area, the 32 px title bar on top and 6 px below it.
    readonly property int contentWidth: 1008
    readonly property int contentHeight: root.screenId === "adult" ? 383 : 561
    readonly property int frameMargin: 6
    readonly property int windowWidth: contentWidth + 2 * frameMargin
    readonly property int windowHeight: Theme.titleBarHeight + contentHeight + frameMargin
    width: windowWidth
    height: windowHeight
    minimumWidth: windowWidth
    maximumWidth: windowWidth
    minimumHeight: windowHeight
    maximumHeight: windowHeight
    // The Delete / Save / Exit row sits inside the content area, at the
    // coordinates the .cpp files give each button.
    readonly property rect deleteButtonRect: root.screenId === "pediatric" ? Qt.rect(570, 524, 134, 32) : (root.screenId === "pregnancy" ? Qt.rect(565, 524, 137, 32) : Qt.rect(560, 348, 137, 32))
    readonly property rect saveButtonRect: root.screenId === "pediatric" ? Qt.rect(720, 524, 134, 32) : (root.screenId === "pregnancy" ? Qt.rect(716, 524, 137, 32) : Qt.rect(714, 348, 137, 32))
    readonly property rect exitButtonRect: root.screenId === "pediatric" ? Qt.rect(871, 524, 134, 32) : (root.screenId === "pregnancy" ? Qt.rect(867, 524, 137, 32) : Qt.rect(867, 348, 137, 32))

    onControllerChanged: root.syncForm()

    onReadyChanged: {
        if (ready)
            root.present();
    }

    // Shows the screen. Called on every open: a controller that is already
    // loaded does not change `ready`, so onReadyChanged alone is not enough.
    function present() {
        root.show();
        root.requestActivate();
        // Like the Widgets windows, the first editable field (the opening
        // date: the patient name is display only) starts with the focus.
        // The tab chain after the loader starts inside the loaded form.
        const first = formLoader.item ? formLoader.nextItemInFocusChain(true) : null;
        if (first)
            first.forceActiveFocus(Qt.OtherFocusReason);
    }

    function requestSave() {
        if (!ready)
            return;
        // 0 Saved, 1 Error (mirrors the history services).
        if (controller.save() !== 0) {
            messageDialog.showInfo(qsTr("Error"), qsTr("Could not save the history."));
            return;
        }
        root.historyChanged();
        root.close();
    }

    function requestDelete() {
        if (!ready || !controller.canDelete)
            return;
        deleteConfirm.messageText = root.deleteQuestion(controller.patientName);
        deleteConfirm.titleText = qsTr("Delete history");
        deleteConfirm.yesNo = true;
        deleteConfirm.present();
    }

    TitleBar {
        id: titleBar
        objectName: "historyTitleBar"
        // Edge to edge like the main window; only the content keeps the side
        // margins.
        x: 0
        y: 0
        width: root.width
        height: Theme.titleBarHeight
        title: root.screenTitle
        closeOnly: true
    }

    // The form is created by the Loader, so the controller is pushed into it
    // whenever it is set or the form is (re)loaded.
    function syncForm() {
        if (formLoader.item)
            formLoader.item.controller = root.controller;
    }

    // The form owns the whole content area (1008 x contentHeight): it places
    // its groups and fields with the absolute coordinates of the .cpp files.
    Loader {
        id: formLoader
        objectName: "historyForm"
        x: root.frameMargin
        y: Theme.titleBarHeight
        width: root.contentWidth
        height: root.contentHeight
        sourceComponent: root.formComponent
        visible: root.ready
        onLoaded: root.syncForm()
    }

    // The three buttons are children of the content area in Widgets, so they
    // are positioned at content coordinates too.
    Item {
        objectName: "historyButtons"
        x: root.frameMargin
        y: Theme.titleBarHeight
        width: root.contentWidth
        height: root.contentHeight

        ActionButton {
            objectName: "deleteHistoryButton"
            x: root.deleteButtonRect.x
            y: root.deleteButtonRect.y
            width: root.deleteButtonRect.width
            height: root.deleteButtonRect.height
            text: qsTr("Delete history")
            // A history that does not exist yet cannot be deleted.
            enabled: root.ready && root.controller.canDelete
            onClicked: root.requestDelete()
        }
        ActionButton {
            objectName: "saveHistoryButton"
            x: root.saveButtonRect.x
            y: root.saveButtonRect.y
            width: root.saveButtonRect.width
            height: root.saveButtonRect.height
            text: qsTr("Save history")
            enabled: root.ready
            onClicked: root.requestSave()
        }
        ActionButton {
            objectName: "exitHistoryButton"
            x: root.exitButtonRect.x
            y: root.exitButtonRect.y
            width: root.exitButtonRect.width
            height: root.exitButtonRect.height
            text: qsTr("Exit")
            onClicked: root.close()
        }
    }

    GxDialog {
        id: messageDialog
        objectName: "historyMessageDialog"
        parent: Overlay.overlay
    }
    GxDialog {
        id: deleteConfirm
        objectName: "deleteHistoryConfirm"
        parent: Overlay.overlay
        onAccepted: {
            if (!root.controller.remove()) {
                messageDialog.showInfo(qsTr("Error"), qsTr("Could not delete the history."));
                return;
            }
            // The history is gone: reopen the screen as a new one, like the
            // Widgets windows did after a successful delete.
            root.controller.load(root.controller.patientId, root.controller.patientName);
            root.historyChanged();
        }
    }

    component ActionButton: Button {
        id: actionButton
        font.bold: true
        background: Rectangle {
            color: !actionButton.enabled ? Theme.accentDisabledBackground : (actionButton.down ? Theme.accentPressed : (actionButton.hovered ? Theme.accentHover : Theme.accentBackground))
            border.color: !actionButton.enabled ? Theme.accentDisabledBorder : Theme.accentBorder
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
}
