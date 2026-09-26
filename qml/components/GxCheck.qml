import QtQuick
import QtQuick.Controls
import Gambasse

// Custom check box (mirrors UxCheck): focus highlight background, distinct
// text color while checked, and a read-only mode that ignores user toggles
// (mouse and keyboard) while keeping the normal look. Both colors follow
// the active theme.
CheckBox {
    id: root

    property bool readOnly: false

    // Like UxCheck::setReadOnly: drop focus when entering read-only, so the
    // focus-policy change applies cleanly and user toggles stay ignored.
    onReadOnlyChanged: {
        if (readOnly)
            focus = false;
    }

    spacing: 6
    focusPolicy: root.readOnly ? Qt.NoFocus : Qt.StrongFocus
    contentItem: Text {
        objectName: "checkText"
        text: root.text
        color: root.checked ? Theme.checkedText : Theme.windowText
        font: root.font
        verticalAlignment: Text.AlignVCenter
        leftPadding: root.indicator.width + root.spacing
    }
    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.fieldBackground
        border.color: Theme.fieldBorder
        border.width: 1
        radius: 2
        Text {
            anchors.centerIn: parent
            visible: root.checked
            text: "\u2713"
            color: Theme.windowText
            font.pixelSize: 13
            font.bold: true
        }
    }
    background: Rectangle {
        objectName: "checkBackground"
        visible: root.activeFocus && !root.readOnly
        color: Theme.checkFocusBackground
    }
    // Swallows user toggles in read-only mode while keeping the normal look
    // (never the disabled look): the program can still use setChecked().
    MouseArea {
        objectName: "readOnlyShield"
        anchors.fill: parent
        enabled: root.readOnly
        acceptedButtons: Qt.AllButtons
        hoverEnabled: false
    }
}
