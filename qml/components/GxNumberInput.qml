import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Gambasse

// Labeled numeric field (mirrors UxNumberInput + UxNumberField): configurable
// integer/decimal digits with a comma decimal separator, optional thousands
// separator (period, formatted on focus out and stripped on focus in),
// right-aligned content and the same focus/required visuals as GxTextInput.
Item {
    id: root

    property string labelText: ""
    property string labelPosition: "left" // or "above"
    property alias text: field.text
    property int integerDigits: 9
    property int decimalDigits: 0
    property bool thousandsSeparator: false
    property bool required: false
    property alias readOnly: field.readOnly
    property alias acceptableInput: field.acceptableInput
    signal textEdited

    onThousandsSeparatorChanged: {
        if (root.thousandsSeparator) {
            if (!field.activeFocus)
                root.formatThousands();
        } else {
            root.stripThousands();
        }
    }

    property string _pattern: root.decimalDigits > 0 ? "^\\d{0," + root.integerDigits + "}([,]\\d{0," + root.decimalDigits + "})?$" : "^\\d{0," + root.integerDigits + "}$"

    function fieldBackground() {
        if (field.activeFocus)
            return Theme.focusBackground;
        if (root.required && field.text.trim() === "")
            return Theme.requiredBackground;
        return Theme.fieldBackground;
    }

    function stripThousands() {
        if (field.text.indexOf(".") < 0)
            return;
        const cursor = field.cursorPosition;
        field.text = field.text.split(".").join("");
        field.cursorPosition = Math.min(cursor, field.text.length);
    }

    function formatThousands() {
        if (!root.thousandsSeparator)
            return;
        const value = field.text.trim();
        if (value === "")
            return;
        const comma = value.indexOf(",");
        let integerPart = comma >= 0 ? value.slice(0, comma) : value;
        const decimalPart = comma >= 0 ? value.slice(comma + 1) : "";
        integerPart = integerPart.split(".").join("");
        let grouped = "";
        let digitCount = 0;
        for (let i = integerPart.length - 1; i >= 0; --i) {
            grouped = integerPart[i] + grouped;
            if (++digitCount % 3 === 0 && i > 0)
                grouped = "." + grouped;
        }
        field.text = comma >= 0 ? grouped + "," + decimalPart : grouped;
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
            onTextEdited: root.textEdited()
            objectName: "field"
            // Display-only fields are disabled line edits in the Widgets windows.
            color: readOnly ? Theme.disabledText : Theme.windowText
            placeholderTextColor: Theme.placeholderText
            horizontalAlignment: TextInput.AlignRight
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
            // QLineEdit text margins: 3 px on the left, 5 on the right.
            leftPadding: 3
            // A display-only field is a disabled line edit in Widgets: the
            // tab order and the initial focus skip it.
            activeFocusOnTab: !readOnly
            rightPadding: 5
            validator: RegularExpressionValidator {
                regularExpression: new RegExp(root._pattern)
            }
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
            onActiveFocusChanged: {
                if (activeFocus)
                    root.stripThousands();
                else
                    root.formatThousands();
            }
        }
    }
}
