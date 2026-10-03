pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Gambasse

// Tab container of the forms (the QTabControl of the original screens): a row
// of tabs over a page frame, each page a child of this item, stacked so that
// only the current one shows.
//
// QTabBar look (Fusion): each tab as wide as its caption, the selected one
// 2 px taller and open towards the page, the others closed by the page frame
// line. A plain row rather than a TabBar: the TabBar list view shifts and
// stretches its buttons, and these tabs need exact geometry.
Item {
    id: root

    property list<string> titles
    property int currentIndex: 0
    // Horizontal padding of each caption. A zone with many tabs (the eleven
    // exam tabs of the pediatric consultation) may tighten it so they fit.
    property int tabPadding: 13
    // The pages, in tab order.
    default property alias pages: stack.data

    Item {
        id: tabRow
        objectName: "tabRow"
        // Above the page frame: the selected tab hides its top line.
        z: 1
        width: parent.width
        height: 25
        Row {
            // Neighbouring tabs share their border line.
            spacing: -1
            Repeater {
                model: root.titles
                delegate: Tab {
                }
            }
        }
    }

    // Page frame of the tab widget; the page content sits 2 px inside it,
    // where the QTabWidget places its stacked page.
    Rectangle {
        objectName: "tabPageFrame"
        x: 0
        y: 24
        width: root.width
        height: root.height - 25
        color: Theme.tabPage
        border.color: Theme.inputBorder
        border.width: 1

        Item {
            x: 2
            y: 2
            width: parent.width - 4
            height: parent.height - 3

            StackLayout {
                id: stack
                objectName: "tabStack"
                anchors.fill: parent
                currentIndex: root.currentIndex
            }
        }
    }

    component Tab: TabButton {
        id: tabButton
        required property int index
        required property string modelData
        objectName: "tab" + tabButton.index
        text: tabButton.modelData
        width: implicitWidth
        height: 25
        z: checked ? 1 : 0
        checked: root.currentIndex === index
        leftPadding: root.tabPadding
        rightPadding: root.tabPadding
        topPadding: 0
        bottomPadding: 1
        onClicked: root.currentIndex = index
        // Painted by a plain child rather than the background: the
        // control re-positions its background item on state changes.
        background: null
        Item {
            z: -1
            width: tabButton.width
            height: tabButton.height
            Rectangle {
                y: tabButton.checked ? 0 : 2
                width: parent.width
                height: tabButton.checked ? parent.height : parent.height - 3
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: tabButton.checked ? Theme.tabTop : Theme.tabInactiveTop
                    }
                    GradientStop {
                        position: 1.0
                        color: tabButton.checked ? Theme.tabPage : Theme.tabInactiveBottom
                    }
                }
            }
            Rectangle {
                y: tabButton.checked ? 0 : 2
                width: parent.width
                height: 1
                color: tabButton.checked ? Theme.inputBorder : Theme.checkBorder
            }
            Rectangle {
                x: 1
                y: tabButton.checked ? 1 : 3
                width: parent.width - 2
                height: 1
                color: tabButton.checked ? Theme.tabTopLine : Theme.tabInactiveTopLine
            }
            Rectangle {
                y: tabButton.checked ? 0 : 2
                width: 1
                height: tabButton.checked ? parent.height : parent.height - 3
                color: tabButton.checked ? Theme.inputBorder : Theme.checkBorder
            }
            Rectangle {
                anchors.right: parent.right
                y: tabButton.checked ? 0 : 2
                width: 1
                height: tabButton.checked ? parent.height : parent.height - 3
                color: tabButton.checked ? Theme.inputBorder : Theme.checkBorder
            }
        }
        contentItem: Text {
            // Unselected captions follow their tab 2 px down.
            topPadding: tabButton.checked ? 1 : 3
            text: tabButton.text
            color: Theme.windowText
            font: tabButton.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
