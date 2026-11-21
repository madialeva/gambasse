import QtQuick
import QtQuick.Controls
import Gambasse

// Main shell window: frameless, with the custom title bar (help, language,
// theme and zoom controls) over the patient screen. Everything shown lives in
// an elastic `stage` scaled by the shared zoom factor; ShellWindowController
// manages the startup state (maximized by default), persisted normal size and
// zoom limits.
Window {
    id: root
    flags: Qt.FramelessWindowHint
    // Shown once ShellWindowController applies the persisted geometry and
    // maximized/normal state, avoiding a base-size flash on startup.
    visible: false
    minimumWidth: shellWindow.minimumSize.width
    minimumHeight: shellWindow.minimumSize.height
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
    // How much the stage is scaled. Popups, which do not inherit the scale of
    // the item they belong to, read it from their window.
    readonly property real zoomFactor: shellWindow.factor

    PatientController {
        id: patients
        objectName: "patientController"
    }

    ShellWindowController {
        id: shellWindow
        objectName: "shellWindowController"
        settings: InterfaceSettings
        window: root
        baseSize: Qt.size(Theme.baseWidth, Theme.baseHeight)
    }

    // The catalog is already switched when the signal arrives, so the grid
    // headers and language-dependent cells are rebuilt in the new language.
    Connections {
        target: InterfaceSettings
        function onLanguageChanged() {
            patients.refreshLanguage();
        }
    }

    Component.onCompleted: {
        patients.load();
        shellWindow.applyInitialGeometry();
    }

    Item {
        id: stage
        objectName: "stage"
        width: Math.max(Theme.baseWidth, root.width / root.zoomFactor)
        height: Math.max(Theme.baseHeight, root.height / root.zoomFactor)
        scale: root.zoomFactor
        transformOrigin: Item.TopLeft

        MainWindow {
            id: mainView
            objectName: "mainView"
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            controller: patients
            zoomFactor: root.zoomFactor
        }
        TitleBar {
            id: titleBar
            objectName: "titleBar"
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            showLanguageTheme: true
            showHelp: true
            showZoom: true
            customMinimize: true
            zoomFactor: root.zoomFactor
            canZoomIn: shellWindow.canZoomIn
            canZoomOut: shellWindow.canZoomOut
            onAboutRequested: aboutDialog.present()
            onMinimizeRequested: shellWindow.minimize()
            onZoomInRequested: shellWindow.zoomIn()
            onZoomOutRequested: shellWindow.zoomOut()
        }

        // Keyboard zoom inside the stage so any open menu, combo list,
        // calendar or modal dialog blocks the shortcuts automatically.
        // Disabled while a child history/consultation window is open so its
        // own shortcuts handle the key without ambiguity.
        Shortcut {
            enabled: !mainView.formWindowOpen
            sequences: ["Ctrl++", "Ctrl+="]
            onActivated: shellWindow.zoomIn()
            onActivatedAmbiguously: shellWindow.zoomIn()
        }
        Shortcut {
            enabled: !mainView.formWindowOpen
            sequences: ["Ctrl+-"]
            onActivated: shellWindow.zoomOut()
            onActivatedAmbiguously: shellWindow.zoomOut()
        }
        Shortcut {
            enabled: !mainView.formWindowOpen
            sequences: ["Ctrl+0"]
            onActivated: shellWindow.reset()
            onActivatedAmbiguously: shellWindow.reset()
        }
    }
    ResizeHandles {
        anchors.fill: parent
        minWidth: root.minimumWidth
        minHeight: root.minimumHeight
    }
    AboutDialog {
        id: aboutDialog
        objectName: "aboutDialog"
        parent: Overlay.overlay
        zoomFactor: root.zoomFactor
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
