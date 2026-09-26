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
            horizontalAlignment: TextInput.AlignRight
            Layout.fillWidth: true
            Layout.fillHeight: true
            verticalAlignment: TextInput.AlignVCenter
            validator: RegularExpressionValidator {
                regularExpression: new RegExp(root._pattern)
            }
            background: Rectangle {
                objectName: "fieldBackground"
                color: root.fieldBackground()
                border.color: Theme.fieldBorder
                border.width: 1
                radius: 2
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
