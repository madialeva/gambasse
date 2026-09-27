pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Gambasse

// One labeled control of a history form, bound to a controller field by name.
// The type picks the control and the value always comes from the controller
// (read through the notifying `values` map, so a save or a reload repaints the
// form); the form itself keeps no state.
Item {
    id: root

    // text | multiline | number | date | combo
    property string type: "text"
    property HistoryController controller: null
    property string field: ""
    property string labelText: ""
    property string labelPosition: "left"
    property int maxLength: 0
    property int integerDigits: 3
    property int decimalDigits: 0
    // [{ "value": int, "label": string }] for the combo type.
    property var options: null
    // Editable combos accept a value that is not in the list (the previous
    // treatment rows of the pediatric form do).
    property bool editable: false
    // Display-only fields (the patient name) have no controller field.
    property string staticText: ""
    // Fields are addressable by their controller name (the forms may still
    // set an explicit objectName).
    objectName: root.field
    property bool readOnly: false

    readonly property var stored: controller && field !== "" ? controller.values[field] : (staticText !== "" ? staticText : undefined)

    Layout.fillWidth: true
    Layout.preferredHeight: type === "multiline" ? 44 : 30

    function write(value) {
        if (controller && field !== "")
            controller.setValue(field, value);
    }

    // Index of the stored value inside the option list (-1 when unknown).
    function indexOfStored() {
        if (!options || root.stored === undefined)
            return -1;
        for (let i = 0; i < options.length; ++i) {
            if (options[i].value === root.stored)
                return i;
        }
        return -1;
    }

    Loader {
        id: loader
        anchors.fill: parent
        sourceComponent: root.type === "date" ? dateComp : root.type === "number" ? numberComp : root.type === "combo" ? comboComp : root.type === "multiline" ? multilineComp : textComp

        Component {
            id: textComp
            GxTextInput {
                labelText: root.labelText
                labelPosition: root.labelPosition
                maxLength: root.maxLength > 0 ? root.maxLength : 32767
                readOnly: root.readOnly
                text: root.stored === undefined ? "" : root.stored
                onTextEdited: root.write(text)
            }
        }
        Component {
            id: multilineComp
            GxTextArea {
                labelText: root.labelText
                labelPosition: root.labelPosition
                maxLength: root.maxLength > 0 ? root.maxLength : 32767
                readOnly: root.readOnly
                text: root.stored === undefined ? "" : root.stored
                onTextEdited: root.write(text)
            }
        }
        Component {
            id: numberComp
            GxNumberInput {
                labelText: root.labelText
                labelPosition: root.labelPosition
                integerDigits: root.integerDigits
                decimalDigits: root.decimalDigits
                readOnly: root.readOnly
                text: root.stored === undefined ? "" : root.stored
                onTextEdited: root.write(text)
            }
        }
        Component {
            id: dateComp
            GxDateInput {
                labelText: root.labelText
                labelPosition: root.labelPosition
                readOnly: root.readOnly
                text: root.stored === undefined ? "" : root.stored
                onTextEdited: root.write(text)
            }
        }
        Component {
            id: comboComp
            GxComboInput {
                id: combo
                labelText: root.labelText
                labelPosition: root.labelPosition
                editable: root.editable
                // Rebuilt whenever the list or the stored value changes, so the
                // translated labels follow the language and a reload resyncs.
                function rebuild() {
                    if (!root.options)
                        return;
                    clear();
                    for (let i = 0; i < root.options.length; ++i)
                        addItem(root.options[i].label, root.options[i].value);
                    const wanted = root.indexOfStored();
                    currentIndex = wanted >= 0 ? wanted : 0;
                }
                Component.onCompleted: rebuild()
                onValueSelected: root.write(currentData())
                Connections {
                    target: root
                    function onStoredChanged() {
                        // Programmatic resync only: never while the user types.
                        if (combo.currentIndex !== root.indexOfStored())
                            combo.rebuild();
                    }
                    function onOptionsChanged() {
                        combo.rebuild();
                    }
                }
            }
        }
    }
}
