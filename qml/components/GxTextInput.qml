import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Gambasse

// Labeled text field (mirrors UxTextInput + UxTextField): integrated label on
// the left or above the field, maximum length, optional uppercase forcing,
// required-empty highlighting and focus highlighting. Positioned absolutely
// by the parent form, like its Widgets counterpart.
Item {
    id: root

    property string labelText: ""
    property string labelPosition: "left" // or "above"
    property alias text: field.text
    property alias placeholderText: field.placeholderText
    property int maxLength: 32767
    property bool uppercase: false
    property bool required: false
    property alias readOnly: field.readOnly
    property alias acceptableInput: field.acceptableInput
    signal textEdited

    function focusField() {
        field.forceActiveFocus();
    }
    signal selectionRequested

    function fieldBackground() {
        if (field.activeFocus)
            return Theme.focusBackground;
        if (root.required && field.text.trim() === "")
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
        TextField {
            id: field
            objectName: "field"
            color: Theme.windowText
            maximumLength: root.maxLength
            Layout.fillWidth: true
            Layout.fillHeight: true
            verticalAlignment: TextInput.AlignVCenter
            background: Rectangle {
                objectName: "fieldBackground"
                color: root.fieldBackground()
                border.color: Theme.fieldBorder
                border.width: 1
                radius: 2
            }
            onTextChanged: {
                if (root.uppercase) {
                    const cursor = cursorPosition;
                    const upper = text.toUpperCase();
                    if (text !== upper) {
                        text = upper;
                        cursorPosition = Math.min(cursor, text.length);
                    }
                }
            }
            onTextEdited: root.textEdited()
        }
    }
}
