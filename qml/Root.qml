import QtQuick
import Gambasse

// Empty shell window (screens land in later phases).
// uiSettings is injected from C++ after loading (see main.cpp).
Window {
    id: root
    flags: Qt.FramelessWindowHint
    visible: true
    minimumWidth: 960
    minimumHeight: 540
    width: Theme.baseWidth
    height: Theme.baseHeight
    title: "Gambasse"
    color: Theme.windowBackground

    property var uiSettings: null
    property var patientController: null

    onUiSettingsChanged: {
        if (root.uiSettings)
            Theme.mode = root.uiSettings.theme;
    }

    Connections {
        target: root.uiSettings
        function onThemeChanged() {
            Theme.mode = root.uiSettings.theme;
        }
    }

    MainWindow {
        objectName: "mainView"
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        controller: root.patientController
    }
    TitleBar {
        id: titleBar
        objectName: "titleBar"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        uiSettings: root.uiSettings
        showLanguageTheme: true
    }
    ResizeHandles {
        anchors.fill: parent
    }
}
