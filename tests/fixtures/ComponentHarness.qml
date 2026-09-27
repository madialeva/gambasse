import QtQuick
import Gambasse

// Test-only shell (never shipped, never scanned by lupdate): loads one
// library component under test and drives the shared Theme singleton, so
// tests assert both themes through the production path.
Item {
    id: harness
    property string themeMode: "claro"
    property string componentFile: ""
    onThemeModeChanged: Theme.mode = themeMode
    Component.onCompleted: Theme.mode = themeMode
    Loader {
        id: loader
        objectName: "componentLoader"
        anchors.fill: parent
        source: harness.componentFile
    }
}
