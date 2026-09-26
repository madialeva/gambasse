import QtQuick

// Manual edge resizing for frameless windows (mirrors the Widgets event
// filter): 6 px strips on every edge and corner, honoring the minimum size.
// Must sit above the content (later sibling) so edges win over inner areas.
// Note: eight explicit areas instead of a nested component, so qmllint keeps
// resolving the outer id without ComponentBehavior pragmas.
Item {
    id: root

    property int margin: 6
    property int minWidth: 960
    property int minHeight: 540
    property int activeEdges: 0
    property point lastLocal: Qt.point(0, 0)

    readonly property int edgeLeft: 1
    readonly property int edgeRight: 2
    readonly property int edgeTop: 4
    readonly property int edgeBottom: 8
    readonly property bool resizable: Window.window.visibility !== Window.Maximized

    // Incremental math on local coordinates (same rationale as the title
    // bar drag): duplicate deliveries apply a zero delta, and clamps keep
    // the minimum size with the edge following the clamp.
    function beginResize(edges, localPos) {
        root.activeEdges = edges;
        root.lastLocal = localPos;
    }

    function updateResize(localPos) {
        if (root.activeEdges === 0)
            return;
        const dx = localPos.x - root.lastLocal.x;
        const dy = localPos.y - root.lastLocal.y;
        if (root.activeEdges & root.edgeLeft) {
            Window.window.x += dx;
            Window.window.width -= dx;
            if (Window.window.width < root.minWidth) {
                Window.window.x -= root.minWidth - Window.window.width;
                Window.window.width = root.minWidth;
            }
        }
        if (root.activeEdges & root.edgeRight)
            Window.window.width = Math.max(root.minWidth, Window.window.width + dx);
        if (root.activeEdges & root.edgeTop) {
            Window.window.y += dy;
            Window.window.height -= dy;
            if (Window.window.height < root.minHeight) {
                Window.window.y -= root.minHeight - Window.window.height;
                Window.window.height = root.minHeight;
            }
        }
        if (root.activeEdges & root.edgeBottom)
            Window.window.height = Math.max(root.minHeight, Window.window.height + dy);
        root.lastLocal = localPos;
    }

    function endResize() {
        root.activeEdges = 0;
    }

    component Zone: MouseArea {
        property int edges: 0
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        enabled: true
    }

    // Edges first, corners last so corners win the ties. Each zone calls
    // the shared functions with its own edges (no nested component, so the
    // outer id keeps resolving without ComponentBehavior pragmas).
    Zone {
        width: root.margin
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        cursorShape: Qt.SizeHorCursor
        enabled: root.resizable
        onPressed: mouse => root.beginResize(root.edgeLeft, Qt.point(mouse.x, mouse.y))
        onPositionChanged: mouse => root.updateResize(Qt.point(mouse.x, mouse.y))
        onReleased: root.endResize()
    }
    Zone {
        width: root.margin
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        cursorShape: Qt.SizeHorCursor
        enabled: root.resizable
        onPressed: mouse => root.beginResize(root.edgeRight, Qt.point(mouse.x, mouse.y))
        onPositionChanged: mouse => root.updateResize(Qt.point(mouse.x, mouse.y))
        onReleased: root.endResize()
    }
    Zone {
        height: root.margin
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        cursorShape: Qt.SizeVerCursor
        enabled: root.resizable
        onPressed: mouse => root.beginResize(root.edgeTop, Qt.point(mouse.x, mouse.y))
        onPositionChanged: mouse => root.updateResize(Qt.point(mouse.x, mouse.y))
        onReleased: root.endResize()
    }
    Zone {
        height: root.margin
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        cursorShape: Qt.SizeVerCursor
        enabled: root.resizable
        onPressed: mouse => root.beginResize(root.edgeBottom, Qt.point(mouse.x, mouse.y))
        onPositionChanged: mouse => root.updateResize(Qt.point(mouse.x, mouse.y))
        onReleased: root.endResize()
    }
    Zone {
        width: root.margin
        height: root.margin
        anchors.top: parent.top
        anchors.left: parent.left
        cursorShape: Qt.SizeFDiagCursor
        enabled: root.resizable
        onPressed: mouse => root.beginResize(root.edgeLeft | root.edgeTop, Qt.point(mouse.x, mouse.y))
        onPositionChanged: mouse => root.updateResize(Qt.point(mouse.x, mouse.y))
        onReleased: root.endResize()
    }
    Zone {
        width: root.margin
        height: root.margin
        anchors.top: parent.top
        anchors.right: parent.right
        cursorShape: Qt.SizeBDiagCursor
        enabled: root.resizable
        onPressed: mouse => root.beginResize(root.edgeRight | root.edgeTop, Qt.point(mouse.x, mouse.y))
        onPositionChanged: mouse => root.updateResize(Qt.point(mouse.x, mouse.y))
        onReleased: root.endResize()
    }
    Zone {
        width: root.margin
        height: root.margin
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        cursorShape: Qt.SizeBDiagCursor
        enabled: root.resizable
        onPressed: mouse => root.beginResize(root.edgeLeft | root.edgeBottom, Qt.point(mouse.x, mouse.y))
        onPositionChanged: mouse => root.updateResize(Qt.point(mouse.x, mouse.y))
        onReleased: root.endResize()
    }
    Zone {
        width: root.margin
        height: root.margin
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        cursorShape: Qt.SizeFDiagCursor
        enabled: root.resizable
        onPressed: mouse => root.beginResize(root.edgeRight | root.edgeBottom, Qt.point(mouse.x, mouse.y))
        onPositionChanged: mouse => root.updateResize(Qt.point(mouse.x, mouse.y))
        onReleased: root.endResize()
    }
}
