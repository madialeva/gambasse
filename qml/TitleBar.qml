pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import Gambasse

// Custom title bar for frameless windows (mirrors the Widgets TitleBar):
// logo and title on the left, window buttons on the right, drag to move,
// double-click to toggle maximize/restore, close-only mode for children.
Item {
    id: root
    height: Theme.titleBarHeight

    property string title: "Gambasse"
    property bool closeOnly: false
    // Optional language/theme controls, bound to the InterfaceSettings
    // singleton (main window only; child windows leave it false).
    property bool showLanguageTheme: false
    // Optional Help menu (main window only). The bar only announces the choice:
    // the window that shows the bar owns the dialog.
    property bool showHelp: false
    signal aboutRequested
    // Optional zoom buttons (history and consultation windows). Like the Help
    // menu, the bar only announces the click: FormWindow zooms, and tells the
    // bar which way it still can.
    property bool showZoom: false
    property bool canZoomIn: true
    property bool canZoomOut: true
    signal zoomInRequested
    signal zoomOutRequested
    // Scale of the window the bar is drawn in (the zoom of a history or
    // consultation window): the logo is rendered at the size it is seen and
    // the tooltips, popups that do not inherit it, are scaled by it.
    property real zoomFactor: 1
    // config.ini is free text, so the code is checked before building any URL
    // (a wrong one used to point at a non-existing flag image).
    // Window.window is an attached property and only resolves on Items, so the
    // pointer handlers (which are not Items) read the window from here.
    readonly property var window: Window.window
    readonly property string languageCode: {
        const code = InterfaceSettings.language;
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

    // Own background so the bar stands out from the window grey, with a
    // 1 px line separating it from the content below.
    Rectangle {
        objectName: "titleBarBackground"
        anchors.fill: parent
        color: Theme.titleBarBackground
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.titleBarBorder
        }
    }

    // Double click toggles maximize/restore. A MouseArea, not a TapHandler:
    // TapHandler.doubleTapped carries a QEventPoint, which qmllint cannot type.
    // It sits below the controls, which therefore keep their own clicks.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onDoubleClicked: mouse => {
            if (mouse.button === Qt.LeftButton)
                root.maximizeFromDoubleClick();
        }
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

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 8
        // Centred in the button band, above the bottom line; every button
        // fills that band.
        y: Math.floor((Theme.titleBarButtonHeight - height) / 2)
        spacing: 6
        Image {
            objectName: "logoImage"
            source: "qrc:/img/logo.svg"
            // Rendered at the height it is seen, laid out 24 px high.
            sourceSize.height: Math.ceil(24 * root.zoomFactor)
            height: 24
            width: implicitWidth / root.zoomFactor
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.title
            // The application font, bold, one pixel larger.
            font.bold: true
            font.pixelSize: 14
            color: Theme.windowText
            anchors.verticalCenter: parent.verticalCenter
            // QLabel rounds the centring down to whole pixels.
            anchors.verticalCenterOffset: 1
        }
    }

    component WindowButton: Button {
        id: button
        property string glyph: ""
        property string tip: ""
        property color hoverColor: Theme.accentHover
        property color pressedColor: Theme.accentPressed
        width: 44
        height: Theme.titleBarButtonHeight
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
        // Its own tooltip, not the attached one shared by the application:
        // it is scaled with a zoomed window (a popup does not inherit the
        // scale) and shown under the button, as the bar is at the top edge.
        ToolTip {
            objectName: "windowButtonTip"
            text: button.tip
            visible: button.hovered && button.tip !== ""
            x: (button.width - width) / 2
            y: button.height
            scale: root.zoomFactor
            transformOrigin: Popup.TopLeft
        }
    }

    // Magnifying glass of the zoom buttons, with a plus or a minus in its
    // lens. Drawn: the interface font has no such symbol, and shapes stay
    // sharp in a zoomed window.
    component Magnifier: Item {
        id: glyph
        property bool plus: true
        property color colour: Theme.windowText
        Rectangle {
            id: lens
            objectName: "magnifierLens"
            x: Math.floor(glyph.width / 2) - 7
            y: Math.floor(glyph.height / 2) - 7
            width: 12
            height: 12
            radius: 6
            color: "transparent"
            border.color: glyph.colour
            border.width: 2
            antialiasing: true
        }
        // Handle, from the rim of the lens down to the right.
        Rectangle {
            x: lens.x + 10.2
            y: lens.y + 9.0
            width: 6
            height: 2.4
            radius: 1
            rotation: 45
            transformOrigin: Item.Left
            color: glyph.colour
            antialiasing: true
        }
        Rectangle {
            x: lens.x + 3
            y: lens.y + 5
            width: 6
            height: 2
            color: glyph.colour
        }
        Rectangle {
            objectName: "magnifierPlus"
            visible: glyph.plus
            x: lens.x + 5
            y: lens.y + 3
            width: 2
            height: 6
            color: glyph.colour
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 2
        anchors.top: parent.top
        spacing: 6
        ToolButton {
            id: helpButton
            objectName: "helpButton"
            height: Theme.titleBarButtonHeight
            visible: root.showHelp
            text: qsTr("Help")
            onClicked: helpMenu.open()
            Menu {
                id: helpMenu
                objectName: "helpMenu"
                y: helpButton.height
                MenuItem {
                    objectName: "aboutMenuItem"
                    text: qsTr("About")
                    onTriggered: root.aboutRequested()
                }
            }
        }
        ToolButton {
            id: languageButton
            objectName: "languageButton"
            height: Theme.titleBarButtonHeight
            visible: root.showLanguageTheme
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
                objectName: "languageMenu"
                y: languageButton.height
                MenuItem {
                    text: qsTr("Spanish")
                    icon.source: "qrc:/img/flag-es.png"
                    icon.color: "transparent" // keep the flag colours, no tint
                    checkable: true
                    checked: root.languageCode === "es"
                    onTriggered: InterfaceSettings.language = "es"
                }
                MenuItem {
                    text: qsTr("Portuguese")
                    icon.source: "qrc:/img/flag-pt.png"
                    icon.color: "transparent" // keep the flag colours, no tint
                    checkable: true
                    checked: root.languageCode === "pt"
                    onTriggered: InterfaceSettings.language = "pt"
                }
                MenuItem {
                    text: qsTr("English")
                    icon.source: "qrc:/img/flag-en.png"
                    icon.color: "transparent" // keep the flag colours, no tint
                    checkable: true
                    checked: root.languageCode === "en"
                    onTriggered: InterfaceSettings.language = "en"
                }
            }
        }
        ToolButton {
            id: themeButton
            objectName: "themeButton"
            height: Theme.titleBarButtonHeight
            visible: root.showLanguageTheme
            icon.source: InterfaceSettings.theme === "oscuro" ? "qrc:/img/moon.svg" : "qrc:/img/sun.svg"
            text: InterfaceSettings.theme === "oscuro" ? qsTr("Dark") : qsTr("Light")
            ToolTip.text: qsTr("Theme")
            ToolTip.visible: hovered
            onClicked: themeMenu.open()
            Menu {
                id: themeMenu
                objectName: "themeMenu"
                y: themeButton.height
                MenuItem {
                    text: qsTr("Light")
                    icon.source: "qrc:/img/sun.svg"
                    checkable: true
                    checked: InterfaceSettings.theme === "claro"
                    onTriggered: InterfaceSettings.theme = "claro"
                }
                MenuItem {
                    text: qsTr("Dark")
                    icon.source: "qrc:/img/moon.svg"
                    checkable: true
                    checked: InterfaceSettings.theme === "oscuro"
                    onTriggered: InterfaceSettings.theme = "oscuro"
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
            id: maximizeButton
            objectName: "maximizeButton"
            readonly property bool maximized: Window.window.visibility === Window.Maximized
            tip: maximized ? qsTr("Restore") : qsTr("Maximize")
            visible: !root.closeOnly
            onClicked: root.toggleMaximize()
            // Drawn: the interface font has no squares.
            contentItem: Item {
                Rectangle {
                    visible: maximizeButton.maximized
                    x: Math.floor(parent.width / 2) - 3
                    y: Math.floor(parent.height / 2) - 6
                    width: 9
                    height: 2
                    color: Theme.windowText
                }
                Rectangle {
                    visible: maximizeButton.maximized
                    x: Math.floor(parent.width / 2) + 4
                    y: Math.floor(parent.height / 2) - 6
                    width: 2
                    height: 9
                    color: Theme.windowText
                }
                Rectangle {
                    x: Math.floor(parent.width / 2) - (maximizeButton.maximized ? 6 : 5)
                    y: Math.floor(parent.height / 2) - (maximizeButton.maximized ? 3 : 5)
                    width: maximizeButton.maximized ? 9 : 11
                    height: width
                    color: "transparent"
                    border.color: Theme.windowText
                    border.width: 2
                }
            }
        }
        WindowButton {
            id: zoomOutButton
            objectName: "zoomOutButton"
            tip: qsTr("Zoom out")
            visible: root.showZoom
            enabled: root.canZoomOut
            onClicked: root.zoomOutRequested()
            contentItem: Magnifier {
                plus: false
                colour: zoomOutButton.enabled ? Theme.windowText : Theme.mutedText
            }
        }
        WindowButton {
            id: zoomInButton
            objectName: "zoomInButton"
            tip: qsTr("Zoom in")
            visible: root.showZoom
            enabled: root.canZoomIn
            onClicked: root.zoomInRequested()
            contentItem: Magnifier {
                plus: true
                colour: zoomInButton.enabled ? Theme.windowText : Theme.mutedText
            }
        }
        WindowButton {
            objectName: "closeButton"
            glyph: "\u00D7"
            font.pixelSize: 20
            tip: qsTr("Close")
            hoverColor: Theme.danger
            pressedColor: Theme.dangerPressed
            onClicked: Window.window.close()
        }
    }
}
