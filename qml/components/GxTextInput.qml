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
        // QVBoxLayout spacing between a caption above and its field.
        rowSpacing: 6
        Text {
            id: caption
            text: root.labelText
            verticalAlignment: Text.AlignVCenter
            // Above the field the caption takes what the field leaves.
            Layout.fillHeight: root.labelPosition === "above"
            visible: root.labelText !== ""
            color: Theme.windowText
            // A QLabel squeezed by its layout cuts the caption instead of
            // drawing it over the field.
            clip: true
            // Beside the field the caption gives way first: it shrinks below
            // its text width before the field goes under its minimum.
            Layout.fillWidth: true
            // Whole pixels like a QLabel size hint, so the field starts on the
            // same column as its Widgets twin.
            Layout.preferredWidth: Math.ceil(implicitWidth)
            Layout.maximumWidth: root.labelPosition === "left" ? Math.ceil(implicitWidth) : Number.POSITIVE_INFINITY
            Layout.alignment: Qt.AlignVCenter
        }
        TextField {
            id: field
            objectName: "field"
            // Display-only fields are disabled line edits in the Widgets windows.
            color: readOnly ? Theme.disabledText : Theme.windowText
            placeholderTextColor: Theme.placeholderText
            maximumLength: root.maxLength
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Theme.fieldHeight
            // The Widgets field keeps its 22 px height inside a taller input,
            // centred beside its label or under it when the label is above.
            // Under a caption a short input squeezes the field, not the
            // caption (38 px: 14 caption + 6 + 18 field).
            Layout.maximumHeight: root.labelPosition === "above" ? Math.min(Theme.fieldHeight, root.height - 6 - Math.ceil(caption.implicitHeight)) : Theme.fieldHeight
            // Preferred at its minimum and grown by fillWidth, so a long label
            // cannot squeeze it away (QLineEdit::minimumSizeHint in Widgets).
            Layout.minimumWidth: Theme.fieldMinimumWidth
            Layout.preferredWidth: Theme.fieldMinimumWidth
            Layout.alignment: root.labelPosition === "above" ? Qt.AlignBottom : Qt.AlignTop
            // QBoxLayout centres with integer division: an odd spare pixel
            // goes below the field, never half above and half below.
            Layout.topMargin: root.labelPosition === "above" ? 0 : Math.max(0, Math.floor((root.height - Theme.fieldHeight) / 2))
            verticalAlignment: TextInput.AlignVCenter
            // QLineEdit text margins: 3 px on the left, 5 on the right, and no
            // vertical padding: the Basic style's 6 px would leave less than a
            // text line in the 21-22 px fields (the text is centred instead).
            topPadding: 0
            bottomPadding: 0
            leftPadding: 3
            // A display-only field is a disabled line edit in Widgets: the
            // tab order and the initial focus skip it.
            activeFocusOnTab: !readOnly
            rightPadding: 5
            background: Rectangle {
                objectName: "fieldBackground"
                color: root.fieldBackground()
                border.color: field.activeFocus ? Theme.focusBorder : Theme.inputBorder
                border.width: 1
                radius: 2
                // Sunken line under the top border, as Fusion paints it.
                Rectangle {
                    x: 1
                    y: 1
                    width: parent.width - 2
                    height: 1
                    color: Theme.fieldShadow
                    visible: !field.activeFocus
                }
                // Fusion focus frame: highlight border plus a light inner ring.
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: 1
                    color: "transparent"
                    border.color: Theme.focusRing
                    border.width: 1
                    visible: field.activeFocus
                }
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
