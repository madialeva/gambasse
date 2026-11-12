import QtQuick
import Gambasse

// Frame shared by the history and consultation windows: a frameless modal
// window with the close-only title bar, and the zoom.
//
// Everything the window shows lives in `stage`, laid out at the base size
// (13 px letters, the coordinates of the original forms) and scaled as a whole
// by the zoom factor: the letters grow and every control grows with them,
// keeping its place. The window measures the base size scaled the same way.
// The children of a derived window go to the stage, so they keep using base
// coordinates.
Window {
    id: root

    property string screenTitle: ""
    // objectName of the title bar, which the tests look up.
    property string titleBarName: ""
    // Content area under the title bar, at the base size: 1008 px wide and as
    // tall as the derived window's form.
    readonly property int contentWidth: 1008
    property int contentHeight: 561
    readonly property int frameMargin: 6
    // Whole window at the base size: 6 px side margins around the content,
    // the title bar on top and 6 px below the content.
    readonly property int baseWidth: contentWidth + 2 * frameMargin
    readonly property int baseHeight: Theme.titleBarHeight + contentHeight + frameMargin
    // How much the stage is scaled. Popups, which do not inherit the scale of
    // the item they belong to, read it from their window.
    readonly property real zoomFactor: zoom.factor
    default property alias content: stage.data

    // Places the window (centred the first time and after a zoom change) and
    // shows it. Called on every open.
    function showPlaced() {
        zoom.placeForShow();
        root.show();
        root.requestActivate();
    }

    visible: false
    modality: Qt.WindowModal
    flags: Qt.FramelessWindowHint
    title: root.screenTitle
    color: Theme.windowBackground
    width: zoom.windowSize.width
    height: zoom.windowSize.height
    minimumWidth: zoom.windowSize.width
    maximumWidth: zoom.windowSize.width
    minimumHeight: zoom.windowSize.height
    maximumHeight: zoom.windowSize.height

    ZoomController {
        id: zoom
        objectName: "zoomController"
        settings: InterfaceSettings
        window: root
        baseSize: Qt.size(root.baseWidth, root.baseHeight)
    }

    Item {
        id: stage
        objectName: "stage"
        width: root.baseWidth
        height: root.baseHeight
        scale: zoom.factor
        transformOrigin: Item.TopLeft

        TitleBar {
            id: titleBar
            objectName: root.titleBarName
            // Edge to edge like the main window; only the content keeps the
            // side margins.
            x: 0
            y: 0
            width: stage.width
            height: Theme.titleBarHeight
            title: root.screenTitle
            closeOnly: true
            zoomFactor: zoom.factor
        }
    }
}
