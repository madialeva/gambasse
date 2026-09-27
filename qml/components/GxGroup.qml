import QtQuick
import Gambasse

// Titled box of a history form: the QML counterpart of the QGroupBox the
// Widgets history windows place with absolute geometry. The title sits above
// the frame (as the platform style paints it) and the children are placed
// with their own x/y, in the same coordinates as the Widgets controls, so a
// form can be a transcription of the .cpp layout.
Item {
    id: root

    property string title: ""

    // A QGroupBox clips its children: a control that overruns the box (the
    // edema checks of the pregnancy form) is cut at its edge.
    clip: true

    // The frame starts below the title line, like the Widgets group boxes; a
    // group without a title (the treatment container) keeps a 4 px strip, the
    // offset the platform style leaves in that case.
    readonly property int frameTop: root.title === "" ? 4 : 18

    Text {
        objectName: "groupTitle"
        // Where Fusion draws the caption: flush left, one pixel up.
        x: 0
        y: -1
        width: root.width - 1
        height: root.frameTop
        visible: root.title !== ""
        verticalAlignment: Text.AlignVCenter
        text: root.title
        color: Theme.windowText
        elide: Text.ElideRight
    }

    // Fusion leaves the left column of the box unpainted: the frame starts
    // 1 px in and reaches the other three edges.
    Rectangle {
        objectName: "groupFrame"
        x: 1
        y: root.frameTop
        width: root.width - 1
        height: root.height - root.frameTop
        color: Theme.groupBackground
        border.color: Theme.groupBorder
        border.width: 1

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            color: "transparent"
            border.color: Theme.groupInnerLine
            border.width: 1
        }
    }
}
