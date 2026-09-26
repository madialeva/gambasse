import QtQuick
import QtQuick.Controls
import Gambasse

// qmllint disable signal-handler-parameters

// Custom title bar for frameless windows (mirrors the Widgets TitleBar):
// logo and title on the left, window buttons on the right, drag to move,
// double-click to toggle maximize/restore, close-only mode for children.
Item {
    id: root
    height: 32

    property string title: "Gambasse"
    property bool closeOnly: false
    // Interface bridge for the optional language/theme controls (main
    // window only; child windows leave showLanguageTheme false).
    property var uiSettings: null
    property bool showLanguageTheme: false
    // config.ini is free text, so the code is checked before building any URL
    // (a wrong one used to point at a non-existing flag image).
    // Window.window is an attached property and only resolves on Items, so the
    // pointer handlers (which are not Items) read the window from here.
    readonly property var window: Window.window
    readonly property string languageCode: {
        const code = root.uiSettings ? root.uiSettings.language : "en";
        return (code === "es" || code === "pt") ? code : "en";
    }
    readonly property string flagSource: "qrc:/img/flag-" + languageCode + ".png"

    // Separate function: the handler must not take QEventPoint (qmllint).
    function maximizeFromDoubleClick() {
        if (!root.closeOnly)
            root.toggleMaximize();
    }

    function toggleMaximize() {
        if (Window.window.visibility === Window.Maximized)
            Window.window.showNormal();
        else
            Window.window.showMaximized();
    }

    // Moving the window: startSystemMove() hands the drag to the window
    // manager, so the window follows the pointer natively instead of jumping
    // step by step as the incremental x/y math did.
    DragHandler {
        id: windowDrag
        objectName: "windowDrag"
        target: null
        acceptedButtons: Qt.LeftButton
        onActiveChanged: {
            if (active && root.window && root.window.visibility !== Window.Maximized)
                root.window.startSystemMove();
        }
    }
    // Double click still toggles maximize (the drag handler owns the press).
    TapHandler {
        acceptedButtons: Qt.LeftButton
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onDoubleTapped: root.maximizeFromDoubleClick()
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6
        Image {
            objectName: "logoImage"
            source: "qrc:/img/logo.svg"
            sourceSize.height: 24
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.title
            font.bold: true
            color: Theme.windowText
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    component WindowButton: Button {
        id: button
        property string glyph: ""
        property string tip: ""
        property color hoverColor: Theme.accentHover
        property color pressedColor: Theme.accentPressed
        width: 44
        height: 28
        focusPolicy: Qt.NoFocus
        font.bold: true
        contentItem: Text {
            text: button.glyph
            color: Theme.windowText
            font: button.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: button.hovered ? (button.pressed ? button.pressedColor : button.hoverColor) : "transparent"
        }
        ToolTip.text: tip
        ToolTip.visible: hovered
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6
        ToolButton {
            id: languageButton
            objectName: "languageButton"
            visible: root.showLanguageTheme && root.uiSettings !== null
            text: {
                if (root.languageCode === "es")
                    return qsTr("Spanish");
                if (root.languageCode === "pt")
                    return qsTr("Portuguese");
                return qsTr("English");
            }
            contentItem: Row {
                spacing: 4
                Image {
                    objectName: "languageFlag"
                    source: root.flagSource
                    sourceSize.width: 24
                    fillMode: Image.PreserveAspectFit
                    height: 16
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: languageButton.text
                    color: Theme.windowText
                    font: languageButton.font
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            ToolTip.text: qsTr("Language")
            ToolTip.visible: hovered
            onClicked: languageMenu.open()
            Menu {
                id: languageMenu
                MenuItem {
                    text: qsTr("Spanish")
                    icon.source: "qrc:/img/flag-es.png"
                    icon.color: "transparent" // keep the flag colours, no tint
                    checkable: true
                    checked: root.languageCode === "es"
                    onTriggered: root.uiSettings.language = "es"
                }
                MenuItem {
                    text: qsTr("Portuguese")
                    icon.source: "qrc:/img/flag-pt.png"
                    icon.color: "transparent" // keep the flag colours, no tint
                    checkable: true
                    checked: root.languageCode === "pt"
                    onTriggered: root.uiSettings.language = "pt"
                }
                MenuItem {
                    text: qsTr("English")
                    icon.source: "qrc:/img/flag-en.png"
                    icon.color: "transparent" // keep the flag colours, no tint
                    checkable: true
                    checked: root.languageCode === "en"
                    onTriggered: root.uiSettings.language = "en"
                }
            }
        }
        ToolButton {
            objectName: "themeButton"
            visible: root.showLanguageTheme && root.uiSettings !== null
            icon.source: root.uiSettings && root.uiSettings.theme === "oscuro" ? "qrc:/img/moon.svg" : "qrc:/img/sun.svg"
            text: root.uiSettings && root.uiSettings.theme === "oscuro" ? qsTr("Dark") : qsTr("Light")
            ToolTip.text: qsTr("Theme")
            ToolTip.visible: hovered
            onClicked: themeMenu.open()
            Menu {
                id: themeMenu
                MenuItem {
                    text: qsTr("Light")
                    icon.source: "qrc:/img/sun.svg"
                    checkable: true
                    checked: !root.uiSettings || root.uiSettings.theme === "claro"
                    onTriggered: root.uiSettings.theme = "claro"
                }
                MenuItem {
                    text: qsTr("Dark")
                    icon.source: "qrc:/img/moon.svg"
                    checkable: true
                    checked: root.uiSettings && root.uiSettings.theme === "oscuro"
                    onTriggered: root.uiSettings.theme = "oscuro"
                }
            }
        }
        WindowButton {
            objectName: "minimizeButton"
            glyph: "\u2014"
            tip: qsTr("Minimize")
            visible: !root.closeOnly
            onClicked: Window.window.showMinimized()
        }
        WindowButton {
            objectName: "maximizeButton"
            glyph: Window.window.visibility === Window.Maximized ? "\u2750" : "\u25A1"
            tip: Window.window.visibility === Window.Maximized ? qsTr("Restore") : qsTr("Maximize")
            visible: !root.closeOnly
            onClicked: root.toggleMaximize()
        }
        WindowButton {
            objectName: "closeButton"
            glyph: "\u2715"
            tip: qsTr("Close")
            hoverColor: Theme.danger
            pressedColor: Theme.dangerPressed
            onClicked: Window.window.close()
        }
    }
}
