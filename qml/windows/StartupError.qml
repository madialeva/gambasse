import QtQuick
import QtQuick.Controls
import Gambasse

// Shown instead of the main shell when the start-up cannot go on (the
// database does not open): the QML counterpart of the QMessageBox the Widgets
// start-up used. Dismissing it ends the application.
Window {
    id: root

    property string titleText: ""
    property string messageText: ""

    visible: true
    width: 460
    height: 220
    title: root.titleText
    color: Theme.windowBackground

    GxDialog {
        objectName: "startupErrorDialog"
        titleText: root.titleText
        messageText: root.messageText
        closePolicy: Popup.NoAutoClose
        onAccepted: Qt.quit()
        Component.onCompleted: present()
    }
}
