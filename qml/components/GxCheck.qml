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

    // Fusion metrics used by UxCheck: 20 px tall row (3 px above/below the
    // 14 px indicator) and the label 19 px from the left edge. The Basic
    // style would pad the control by 6 on every side, pushing both the
    // indicator and the label off the Widgets geometry.
    topPadding: 3
    bottomPadding: 3
    leftPadding: 0
    rightPadding: 0
    spacing: 5
    focusPolicy: root.readOnly ? Qt.NoFocus : Qt.StrongFocus
    contentItem: Text {
        objectName: "checkText"
        text: root.text
        color: root.checked ? Theme.checkedText : Theme.windowText
        font: root.font
        verticalAlignment: Text.AlignVCenter
        // A caption longer than the box is cut, like QCheckBox's.
        clip: true
        // The control places contentItem at (leftPadding, topPadding), so the
        // label offset is carried here: indicator width + spacing = 19 px.
        leftPadding: root.indicator.width + root.spacing
    }
    indicator: Rectangle {
        // Same box as the Widgets UxCheck: 14 px square, neutral border.
        implicitWidth: 14
        implicitHeight: 14
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.fieldBackground
        border.color: Theme.checkBorder
        border.width: 1
        Rectangle {
            x: 1
            y: 1
            width: parent.width - 2
            height: 1
            color: Theme.checkShadow
        }
        Text {
            anchors.centerIn: parent
            visible: root.checked
            text: "\u2713"
            color: Theme.windowText
            font.pixelSize: 11
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
