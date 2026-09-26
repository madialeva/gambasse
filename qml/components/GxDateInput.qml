pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Gambasse

// Labeled date field (mirrors UxDateInput + UxDateField): dd/MM/yyyy text
// (empty allowed, like the Widgets field when not required), a calendar
// button that opens a month popup writing the picked date, and the same
// focus/required visuals as GxTextInput.
Item {
    id: root

    property string labelText: ""
    property string labelPosition: "left" // or "above"
    property alias text: field.text
    property bool required: false
    property alias readOnly: field.readOnly
    property alias acceptableInput: field.acceptableInput

    // Month shown by the calendar popup (0-based month, like Date).
    property int shownYear: new Date().getFullYear()
    property int shownMonth: new Date().getMonth()

    function fieldBackground() {
        if (field.activeFocus)
            return Theme.focusBackground;
        if (root.required && field.text.trim() === "")
            return Theme.requiredBackground;
        return Theme.fieldBackground;
    }

    function openCalendar() {
        const match = /^(\d{2})\/(\d{2})\/(\d{4})$/.exec(field.text.trim());
        if (match) {
            shownYear = parseInt(match[3], 10);
            shownMonth = parseInt(match[2], 10) - 1;
        } else {
            const today = new Date();
            shownYear = today.getFullYear();
            shownMonth = today.getMonth();
        }
        calendarPopup.open();
        // Under the field, or above it when the panel leaves no room below.
        const overlay = calendarPopup.parent;
        const below = root.mapToItem(overlay, 0, root.height);
        const left = root.mapToItem(overlay, 0, 0).x;
        const fitsBelow = below.y + calendarPopup.height + 4 <= overlay.height;
        calendarPopup.x = Math.max(4, Math.min(left, overlay.width - calendarPopup.width - 4));
        calendarPopup.y = fitsBelow ? below.y : Math.max(4, below.y - root.height - calendarPopup.height);
    }

    function shiftMonth(delta) {
        const d = new Date(shownYear, shownMonth + delta, 1);
        shownMonth = d.getMonth();
        shownYear = d.getFullYear();
    }

    function pickDate(year, monthZeroBased, day) {
        const dd = String(day).padStart(2, "0");
        const mm = String(monthZeroBased + 1).padStart(2, "0");
        field.text = dd + "/" + mm + "/" + year;
        calendarPopup.close();
    }

    GridLayout {
        anchors.fill: parent
        rows: root.labelPosition === "above" ? 2 : 1
        columns: root.labelPosition === "above" ? 1 : 2
        columnSpacing: 6
        rowSpacing: 2
        Text {
            text: root.labelText
            visible: root.labelText !== ""
            color: Theme.windowText
            Layout.fillWidth: root.labelPosition !== "left"
            Layout.alignment: Qt.AlignVCenter
        }
        TextField {
            id: field
            objectName: "field"
            color: Theme.windowText
            Layout.fillWidth: true
            Layout.fillHeight: true
            verticalAlignment: TextInput.AlignVCenter
            rightPadding: calendarButton.width + 8
            validator: RegularExpressionValidator {
                regularExpression: /^(\d{2}\/\d{2}\/\d{4})?$/
            }
            background: Rectangle {
                objectName: "fieldBackground"
                color: root.fieldBackground()
                border.color: Theme.fieldBorder
                border.width: 1
                radius: 2
            }
            Button {
                id: calendarButton
                objectName: "calendarButton"
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                width: 22
                height: 22
                focusPolicy: Qt.NoFocus
                enabled: !field.readOnly
                background: Rectangle {
                    color: "transparent"
                }
                contentItem: Canvas {
                    id: calendarIcon
                    anchors.fill: parent
                    onPaint: {
                        const ctx = getContext("2d");
                        ctx.clearRect(0, 0, width, height);
                        ctx.fillStyle = Theme.windowText;
                        ctx.fillRect(2, 5, width - 4, height - 7);
                        ctx.fillStyle = Theme.fieldBackground;
                        ctx.fillRect(3, 8, width - 6, height - 11);
                        ctx.fillStyle = Theme.accentBackground;
                        ctx.fillRect(2, 5, width - 4, 4);
                    }
                }
                onClicked: root.openCalendar()
            }
            Connections {
                target: Theme
                function onDarkChanged() {
                    calendarIcon.requestPaint();
                }
            }
        }
    }

    // Parent of the calendar popup: the window overlay, so it is painted on top
    // of the panel instead of being clipped inside the field's own item.
    Popup {
        id: calendarPopup
        objectName: "calendarPopup"
        parent: Overlay.overlay
        width: 260
        height: 280
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            color: Theme.fieldBackground
            border.color: Theme.fieldBorder
            radius: 4
        }
        contentItem: Column {
            spacing: 4
            Row {
                width: parent.width
                spacing: 4
                Button {
                    text: "\u2039"
                    width: 32
                    focusPolicy: Qt.NoFocus
                    onClicked: root.shiftMonth(-1)
                }
                Text {
                    width: parent.width - 72
                    height: 32
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.windowText
                    font.bold: true
                    text: Qt.formatDate(new Date(root.shownYear, root.shownMonth, 1), "MMMM yyyy")
                }
                Button {
                    text: "\u203A"
                    width: 32
                    focusPolicy: Qt.NoFocus
                    onClicked: root.shiftMonth(1)
                }
            }
            DayOfWeekRow {
                width: parent.width
                locale: Qt.locale()
            }
            MonthGrid {
                id: monthGrid
                objectName: "monthGrid"
                width: parent.width
                height: 180
                month: root.shownMonth
                year: root.shownYear
                locale: Qt.locale()
                delegate: Item {
                    id: cell
                    objectName: "dayCell"
                    required property var model
                    readonly property bool other: model.month !== monthGrid.month
                    width: monthGrid.width / 7
                    height: monthGrid.height / 6
                    Text {
                        anchors.centerIn: parent
                        text: cell.model.day
                        opacity: cell.other ? 0.4 : 1.0
                        color: Theme.windowText
                        font.bold: cell.model.today && !cell.other
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: {
                            if (!cell.other)
                                root.pickDate(cell.model.year, cell.model.month, cell.model.day);
                        }
                    }
                }
            }
        }
    }
}
