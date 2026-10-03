import QtQuick
import QtQuick.Controls
import Gambasse

// Modal shell shared by the three consultation screens (pediatric, adult and
// pregnancy): frameless window with the close-only title bar, the form loaded
// in the middle and the four buttons (Exit / Save / New / Delete) at the
// coordinates of each original form. Unlike a history, saving keeps the
// window open: the controller keeps the list and the edited consultation,
// this window only wires the buttons, the confirmation and the messages.
Window {
    id: root

    // PediatricConsultationController, AdultConsultationController or
    // PregnancyConsultationController, created by PatientController.
    property ConsultationController controller: null
    property string screenTitle: ""
    // Which consultation this window shows ("pediatric", "adult",
    // "pregnancy"). Not named "screen": Window already declares one.
    property string screenId: ""
    // Source component of the form that matches the controller.
    property var formComponent: null
    readonly property bool ready: controller !== null && controller.loaded

    visible: false
    modality: Qt.WindowModal
    flags: Qt.FramelessWindowHint
    title: root.screenTitle
    color: Theme.windowBackground
    // Same fixed frame as the history windows: 6 px side margins around the
    // 1008 px content area, the title bar on top and 6 px below it.
    readonly property int contentWidth: 1008
    readonly property int contentHeight: root.screenId === "pregnancy" ? 341 : 561
    readonly property int frameMargin: 6
    readonly property int windowWidth: contentWidth + 2 * frameMargin
    readonly property int windowHeight: Theme.titleBarHeight + contentHeight + frameMargin
    width: windowWidth
    height: windowHeight
    minimumWidth: windowWidth
    maximumWidth: windowWidth
    minimumHeight: windowHeight
    maximumHeight: windowHeight
    // The buttons sit inside the content area, at the coordinates of each
    // original designer.
    readonly property rect exitButtonRect: root.screenId === "pregnancy" ? Qt.rect(735, 300, 103, 32) : (root.screenId === "pediatric" ? Qt.rect(837, 388, 158, 32) : Qt.rect(842, 399, 158, 32))
    readonly property rect saveButtonRect: root.screenId === "pregnancy" ? Qt.rect(569, 300, 158, 32) : (root.screenId === "pediatric" ? Qt.rect(838, 430, 158, 32) : Qt.rect(842, 436, 158, 32))
    readonly property rect newButtonRect: root.screenId === "pregnancy" ? Qt.rect(846, 262, 158, 32) : (root.screenId === "pediatric" ? Qt.rect(837, 471, 158, 32) : Qt.rect(842, 475, 158, 32))
    readonly property rect deleteButtonRect: root.screenId === "pregnancy" ? Qt.rect(846, 300, 158, 32) : (root.screenId === "pediatric" ? Qt.rect(838, 511, 158, 32) : Qt.rect(842, 513, 158, 32))

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
        root.focusReason();
    }

    // Like the original forms, the reason field takes the focus when a new
    // consultation starts. Fields are named after their controller field.
    function focusReason() {
        const reason = formLoader.item ? root.findItem(formLoader.item, "reason") : null;
        const input = reason ? reason.nextItemInFocusChain(true) : null;
        if (input)
            input.forceActiveFocus(Qt.OtherFocusReason);
    }

    function findItem(item: Item, name: string): Item {
        if (item.objectName === name)
            return item;
        for (let i = 0; i < item.children.length; ++i) {
            const found = root.findItem(item.children[i], name);
            if (found)
                return found;
        }
        return null;
    }

    function requestSave() {
        if (!ready)
            return;
        // 0 Saved, 1 Error (mirrors the consultation services).
        if (controller.save() !== 0)
            messageDialog.showInfo(qsTr("Error"), qsTr("Could not save the consultation."));
    }

    function requestNew() {
        if (!ready || !controller.canStartNew)
            return;
        controller.startNew();
        root.focusReason();
    }

    function requestDelete() {
        if (!ready || !controller.canDelete)
            return;
        deleteConfirm.messageText = qsTr("Delete the consultation of %1?").arg(controller.dateText);
        deleteConfirm.titleText = qsTr("Delete consultation");
        deleteConfirm.yesNo = true;
        deleteConfirm.present();
    }

    TitleBar {
        id: titleBar
        objectName: "consultationTitleBar"
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
    // its groups, fields and list with the coordinates of the designer.
    Loader {
        id: formLoader
        objectName: "consultationForm"
        x: root.frameMargin
        y: Theme.titleBarHeight
        width: root.contentWidth
        height: root.contentHeight
        sourceComponent: root.formComponent
        visible: root.ready
        onLoaded: root.syncForm()
    }

    Item {
        objectName: "consultationButtons"
        x: root.frameMargin
        y: Theme.titleBarHeight
        width: root.contentWidth
        height: root.contentHeight

        GxActionButton {
            objectName: "exitConsultationButton"
            x: root.exitButtonRect.x
            y: root.exitButtonRect.y
            width: root.exitButtonRect.width
            height: root.exitButtonRect.height
            text: qsTr("Exit")
            onClicked: root.close()
        }
        GxActionButton {
            objectName: "saveConsultationButton"
            x: root.saveButtonRect.x
            y: root.saveButtonRect.y
            width: root.saveButtonRect.width
            height: root.saveButtonRect.height
            text: qsTr("Save consultation")
            enabled: root.ready
            onClicked: root.requestSave()
        }
        GxActionButton {
            objectName: "newConsultationButton"
            x: root.newButtonRect.x
            y: root.newButtonRect.y
            width: root.newButtonRect.width
            height: root.newButtonRect.height
            text: qsTr("New consultation")
            // Already on a new consultation: nothing to start.
            enabled: root.ready && root.controller.canStartNew
            onClicked: root.requestNew()
        }
        GxActionButton {
            objectName: "deleteConsultationButton"
            x: root.deleteButtonRect.x
            y: root.deleteButtonRect.y
            width: root.deleteButtonRect.width
            height: root.deleteButtonRect.height
            text: qsTr("Delete consultation")
            // A consultation not saved yet cannot be deleted.
            enabled: root.ready && root.controller.canDelete
            onClicked: root.requestDelete()
        }
    }

    GxDialog {
        id: messageDialog
        objectName: "consultationMessageDialog"
        parent: Overlay.overlay
    }
    GxDialog {
        id: deleteConfirm
        objectName: "deleteConsultationConfirm"
        parent: Overlay.overlay
        onAccepted: {
            if (!root.controller.remove()) {
                messageDialog.showInfo(qsTr("Error"), qsTr("Could not delete the consultation."));
                return;
            }
            root.focusReason();
        }
    }
}
