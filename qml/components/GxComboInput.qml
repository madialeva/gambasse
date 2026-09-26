import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Gambasse

// Labeled combo box (mirrors UxComboInput): label on the left or above the
// box, key/value items added programmatically with the persisted datum
// independent of the language, and optional in-place editing.
Item {
    id: root

    property string labelText: ""
    property string labelPosition: "left" // or "above"
    property alias currentIndex: combo.currentIndex
    property alias currentText: combo.currentText
    // Typed text in editable mode (mirrors QComboBox::currentText(), which
    // always reflects the line edit; in QML currentText follows selection).
    property alias editText: combo.editText
    property alias editable: combo.editable
    // Internal editor, for direct focus control in tests and windows.
    property alias editor: combo.contentItem

    function addItem(text, value) {
        comboModel.append({
            "text": text,
            "value": value
        });
    }

    function clear() {
        comboModel.clear();
    }

    function findData(value) {
        for (let i = 0; i < comboModel.count; ++i) {
            if (comboModel.get(i).value === value)
                return i;
        }
        return -1;
    }

    function currentData() {
        if (combo.currentIndex < 0 || combo.currentIndex >= comboModel.count)
            return 0;
        return comboModel.get(combo.currentIndex).value;
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
        ComboBox {
            id: combo
            objectName: "comboBox"
            textRole: "text"
            valueRole: "value"
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: ListModel {
                id: comboModel
            }
        }
    }
}
