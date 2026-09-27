import QtQuick
import QtQuick.Controls
import Gambasse

// Modal confirmation/information dialog with the application theme (native
// dialogs would ignore it and cannot be driven offscreen). One instance is
// reused for every message: set titleText/messageText, choose the buttons
// with yesNo, then open(). Result arrives through accepted()/rejected().
Popup {
    id: root
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    anchors.centerIn: parent
    width: 420
    padding: 16

    property string titleText: ""
    property string messageText: ""
    property bool yesNo: false
    signal accepted
    signal rejected

    // Opening the popup straight from the click that asks for it fights the
    // focus change of that same event (the popup opens and closes again), so it
    // is shown once the event has been delivered.
    function present() {
        Qt.callLater(function () {
            root.open();
        });
    }

    function showInfo(title, message) {
        titleText = title;
        messageText = message;
        yesNo = false;
        present();
    }

    background: Rectangle {
        color: Theme.fieldBackground
        border.color: Theme.fieldBorder
        radius: 6
    }
    // Implicit height follows the message, so the buttons stay right under it.
    contentItem: Column {
        spacing: 12
        width: root.availableWidth
        Text {
            width: parent.width
            text: root.titleText
            font.bold: true
            font.pixelSize: 15
            color: Theme.windowText
        }
        Text {
            width: parent.width
            text: root.messageText
            wrapMode: Text.Wrap
            color: Theme.windowText
        }
        Row {
            spacing: 8
            anchors.right: parent.right
            DialogButton {
                objectName: "dialogOkButton"
                visible: !root.yesNo
                text: qsTr("OK")
                onClicked: {
                    root.close();
                    root.accepted();
                }
            }
            DialogButton {
                objectName: "dialogYesButton"
                visible: root.yesNo
                text: qsTr("Yes")
                onClicked: {
                    root.close();
                    root.accepted();
                }
            }
            DialogButton {
                objectName: "dialogNoButton"
                visible: root.yesNo
                text: qsTr("No")
                onClicked: {
                    root.close();
                    root.rejected();
                }
            }
        }
    }
    component DialogButton: Button {
        id: dialogButton
        font.bold: true
        background: Rectangle {
            color: dialogButton.hovered ? Theme.accentHover : Theme.accentBackground
            border.color: Theme.accentBorder
            radius: 4
        }
        contentItem: Text {
            text: dialogButton.text
            color: Theme.accentText
            font: dialogButton.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
