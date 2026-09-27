import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Gambasse

// Labeled multi-line field, for the free text boxes the history forms show
// taller (the Widgets windows reserve a double-height box for them). Same
// integrated label, maximum length and focus/required visuals as GxTextInput.
Item {
    id: root

    property string labelText: ""
    property string labelPosition: "left" // or "above"
    property alias text: area.text
    property int maxLength: 32767
    property bool required: false
    property alias readOnly: area.readOnly
    signal textEdited

    function focusField() {
        area.forceActiveFocus();
    }

    function fieldBackground() {
        if (area.activeFocus)
            return Theme.focusBackground;
        if (root.required && area.text.trim() === "")
            return Theme.requiredBackground;
        return Theme.fieldBackground;
    }

    GridLayout {
        anchors.fill: parent
        rows: root.labelPosition === "above" ? 2 : 1
        columns: root.labelPosition === "above" ? 1 : 2
        columnSpacing: 6
        rowSpacing: 2
        Text {
            text: root.labelText
            visible: root.labelText !== ""
            color: Theme.windowText
            Layout.fillWidth: root.labelPosition !== "left"
            Layout.alignment: Qt.AlignVCenter
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            TextArea {
                id: area
                objectName: "field"
                color: Theme.windowText
                wrapMode: TextArea.Wrap
                selectByMouse: true
                background: Rectangle {
                    objectName: "fieldBackground"
                    color: root.fieldBackground()
                    border.color: area.activeFocus ? Theme.focusBorder : Theme.inputBorder
                    border.width: 1
                    radius: 2
                    // Sunken line under the top border, as Fusion paints it.
                    Rectangle {
                        x: 1
                        y: 1
                        width: parent.width - 2
                        height: 1
                        color: Theme.fieldShadow
                        visible: !area.activeFocus
                    }
                    // Fusion focus frame: highlight border plus a light inner ring.
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 1
                        radius: 1
                        color: "transparent"
                        border.color: Theme.focusRing
                        border.width: 1
                        visible: area.activeFocus
                    }
                }
                onTextChanged: root.textEdited()
            }
        }
    }
}
