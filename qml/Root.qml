import QtQuick
import Gambasse

// Main shell window: frameless, with the custom title bar (language and
// theme controls) over the patient screen. It owns the patient controller,
// loaded from the database already opened by main.cpp; the theme follows the
// InterfaceSettings singleton through Theme.
Window {
    id: root
    flags: Qt.FramelessWindowHint
    visible: true
    // Opens at the base size, which is also the smallest it can shrink to.
    minimumWidth: Theme.baseWidth
    minimumHeight: Theme.baseHeight
    width: Theme.baseWidth
    height: Theme.baseHeight
    title: "Gambasse"
    color: Theme.windowBackground
    // The Gx controls take their colours from Theme; this palette is for the
    // Qt Quick controls that read the palette, chiefly the Linux photo file
    // chooser, which would otherwise keep the system's colours.
    palette {
        window: Theme.windowBackground
        base: Theme.fieldBackground
        alternateBase: Theme.buttonBottom
        button: Theme.buttonTop
        light: Theme.tabTop
        midlight: Theme.buttonBottom
        mid: Theme.inputBorder
        dark: Theme.inputBorder
        shadow: "black"
        highlight: Theme.highlight
        highlightedText: Theme.highlightedText
        placeholderText: Theme.placeholderText
        // The text roles per group: a role set for every group at once would
        // win over its disabled value, and disabled buttons would not grey out.
        active.windowText: Theme.windowText
        active.text: Theme.windowText
        active.buttonText: Theme.windowText
        inactive.windowText: Theme.windowText
        inactive.text: Theme.windowText
        inactive.buttonText: Theme.windowText
        disabled.windowText: Theme.disabledText
        disabled.text: Theme.disabledText
        disabled.buttonText: Theme.disabledText
    }

    readonly property alias patientController: patients

    PatientController {
        id: patients
        objectName: "patientController"
    }

    // The catalog is already switched when the signal arrives, so the grid
    // headers and language-dependent cells are rebuilt in the new language.
    Connections {
        target: InterfaceSettings
        function onLanguageChanged() {
            patients.refreshLanguage();
        }
    }

    Component.onCompleted: patients.load()

    MainWindow {
        id: mainView
        objectName: "mainView"
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        controller: patients
    }
    TitleBar {
        id: titleBar
        objectName: "titleBar"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        showLanguageTheme: true
    }
    ResizeHandles {
        anchors.fill: parent
    }

    // While a history or consultation window is open (modal, in front), the
    // whole shell, title bar included, fades behind a veil so it no longer
    // looks active. It also swallows the pointer, so nothing underneath
    // reacts to hovering.
    Rectangle {
        objectName: "modalVeil"
        anchors.fill: parent
        z: 10
        color: Theme.modalVeil
        opacity: mainView.formWindowOpen ? 1 : 0
        visible: opacity > 0
        Behavior on opacity {
            NumberAnimation {
                duration: Theme.modalVeilFade
            }
        }
        MouseArea {
            anchors.fill: parent
            // Only while the window is open: fading out never blocks.
            enabled: mainView.formWindowOpen
            hoverEnabled: true
            acceptedButtons: Qt.AllButtons
            onWheel: wheel => wheel.accepted = true
        }
    }
}
