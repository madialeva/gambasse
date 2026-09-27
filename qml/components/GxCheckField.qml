import QtQuick
import Gambasse

// Check box bound to a controller field by name, the GxCheck counterpart of
// GxField for the boolean fields of the history forms. The stored value drives
// the tick, so a reload or a language change resyncs it.
GxCheck {
    id: root

    property HistoryController controller: null
    property string field: ""
    property bool readOnly: false

    readonly property var stored: controller && field !== "" ? controller.values[field] : undefined

    // readOnly keeps the look but ignores the toggles, like UxCheck.
    enabled: !readOnly
    checked: stored === undefined ? false : stored === true || stored === "true"

    onToggled: {
        if (controller && field !== "")
            controller.setValue(field, checked);
    }
    Connections {
        target: root
        function onStoredChanged() {
            const wanted = root.stored === true || root.stored === "true";
            if (root.checked !== wanted)
                root.checked = wanted;
        }
    }
}
