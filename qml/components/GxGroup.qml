import QtQuick
import Gambasse

// Titled section of a screen, used by every screen for a homogeneous look:
// a rounded card whose background stands out from the window grey, with its
// title in a badge set on the card's top border. Two ways to fill it:
// - absolute: children placed with their own x/y from the group's top-left
//   corner, title strip included (the history forms, transcribed from the
//   original coordinates);
// - layout: a layout anchored to `body`, the padded area inside the card
//   (the main screen detail groups).
Item {
    id: root

    property string title: ""
    // A group inside another one (the pregnancy exam subgroups) keeps only
    // the rounded border: its parent's background shows through.
    property bool nested: false
    // Padding of `body` inside the card.
    property int padding: 8

    // Padded area inside the card, for a layout: `anchors.fill: group.body`.
    readonly property alias body: bodyArea

    // A control that overruns the group (the edema checks of the pregnancy
    // form) is cut at its edge.
    clip: true

    // A titled card starts halfway down the 18 px title strip, so the title
    // badge sits on its top border, half outside and half inside, and ends
    // before the first row of controls (the history forms place it from
    // y = 17 on). A group without a title (the treatment container) keeps a
    // 4 px strip.
    readonly property int frameTop: root.title === "" ? 4 : 8
    // Where the contents begin: right below the title badge, so the card
    // gained above gives a layout more room.
    readonly property int contentTop: root.title === "" ? 4 : 16
    // Horizontal margin of the badge text: tighter in the narrow nested
    // groups (whose badge also starts closer to the edge), so their titles
    // still fit.
    readonly property int badgeMargin: root.nested ? 4 : 8

    Rectangle {
        objectName: "groupFrame"
        x: 0
        y: root.frameTop
        width: root.width
        height: root.height - root.frameTop
        radius: root.nested ? Theme.groupRadius - 2 : Theme.groupRadius
        color: root.nested ? "transparent" : Theme.groupBackground
        border.color: Theme.groupBorder
        border.width: 1
    }

    // Title badge: a rounded, filled label in the theme's inverted colours,
    // centred on the card's top border and drawn above the group's children.
    Rectangle {
        objectName: "groupTitleBadge"
        z: 1
        x: root.nested ? 4 : 8
        y: root.frameTop - height / 2
        width: Math.min(titleText.implicitWidth + 2 * root.badgeMargin, root.width - 2 * x)
        height: 16
        radius: height / 2
        visible: root.title !== ""
        color: Theme.groupTitleBackground

        Text {
            id: titleText
            objectName: "groupTitle"
            anchors.fill: parent
            anchors.leftMargin: root.badgeMargin
            anchors.rightMargin: root.badgeMargin
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.bold: !root.nested
            text: root.title
            color: Theme.groupTitleText
            elide: Text.ElideRight
        }
    }

    Item {
        id: bodyArea
        objectName: "groupBody"
        x: root.padding
        y: root.contentTop + root.padding
        width: root.width - 2 * root.padding
        height: root.height - root.contentTop - 2 * root.padding
    }
}
