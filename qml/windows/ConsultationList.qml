pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import Gambasse

// Consultation list of the consultation screens (the xdgConsultas grid of the
// original forms): Date and Reason columns, most recent first, framed like the
// patient grid. Clicking a row edits that consultation.
Item {
    id: root

    property ConsultationController controller: null
    // Width of the Date column, like dgcData.
    property int dateColumnWidth: 130
    readonly property int headerHeight: 22
    readonly property int rowHeight: 20

    // Header and rows inside a 1 px frame, like the patient grid.
    Item {
        anchors.fill: parent
        anchors.margins: 1

        Row {
            objectName: "consultationHeader"
            width: parent.width
            height: root.headerHeight
            HeaderCell {
                width: root.dateColumnWidth
                text: qsTr("Date")
            }
            HeaderCell {
                width: parent.width - root.dateColumnWidth
                text: qsTr("Reason")
            }
        }

        ListView {
            id: list
            objectName: "consultationRows"
            y: root.headerHeight
            width: parent.width
            height: parent.height - root.headerHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: root.controller ? root.controller.consultations : []
            currentIndex: root.controller ? root.controller.currentIndex : -1
            // The edited consultation stays in view after a save.
            onCurrentIndexChanged: {
                if (currentIndex >= 0)
                    positionViewAtIndex(currentIndex, ListView.Contain);
            }
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
            delegate: Rectangle {
                id: row
                objectName: "consultationRow"
                required property int index
                required property var modelData
                readonly property bool current: root.controller !== null && root.controller.currentIndex === row.index
                width: list.width
                height: root.rowHeight
                color: row.current ? Theme.highlight : (row.index % 2 ? Theme.alternateRowBackground : Theme.fieldBackground)
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Theme.fieldBorder
                }
                Text {
                    x: 4
                    width: root.dateColumnWidth - 8
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    color: row.current ? Theme.highlightedText : Theme.windowText
                    text: row.modelData.date
                }
                Text {
                    x: root.dateColumnWidth + 4
                    width: parent.width - root.dateColumnWidth - 8
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    color: row.current ? Theme.highlightedText : Theme.windowText
                    text: row.modelData.reason
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.controller.select(row.index)
                }
            }
        }
    }

    // Drawn above the rows so a scrolled row never covers it.
    Rectangle {
        objectName: "consultationBorder"
        anchors.fill: parent
        z: 1
        color: "transparent"
        border.color: Theme.inputBorder
        border.width: 1
    }

    component HeaderCell: Rectangle {
        property alias text: caption.text
        height: root.headerHeight
        color: Theme.windowBackground
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.inputBorder
        }
        Text {
            id: caption
            x: 4
            width: parent.width - 8
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            font.bold: true
            color: Theme.windowText
        }
    }
}
