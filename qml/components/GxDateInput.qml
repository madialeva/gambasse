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
    signal textEdited

    // Month shown by the calendar popup (0-based month, like Date).
    property int shownYear: new Date().getFullYear()
    property int shownMonth: new Date().getMonth()
    // Scale of the calendar: a popup does not inherit the scale of a zoomed
    // history or consultation window, so it takes it from the window.
    readonly property real zoomFactor: (root.Window.window as FormWindow)?.zoomFactor ?? 1

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
        // Under the field, or above it when the panel leaves no room below,
        // measured with the size it is drawn at.
        const overlay = calendarPopup.parent;
        const width = calendarPopup.width * root.zoomFactor;
        const height = calendarPopup.height * root.zoomFactor;
        const below = root.mapToItem(overlay, 0, root.height);
        const above = root.mapToItem(overlay, 0, 0);
        const fitsBelow = below.y + height + 4 <= overlay.height;
        calendarPopup.x = Math.max(4, Math.min(above.x, overlay.width - width - 4));
        calendarPopup.y = fitsBelow ? below.y : Math.max(4, above.y - height);
    }

    function shiftMonth(delta) {
        const d = new Date(shownYear, shownMonth + delta, 1);
        shownMonth = d.getMonth();
        shownYear = d.getFullYear();
    }

    // Qt's month and day names are lower case in Portuguese and Spanish; the
    // calendar shows them in sentence case, like the catalogs' titles.
    function sentenceCase(text: string): string {
        return text.charAt(0).toUpperCase() + text.slice(1);
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
        // QVBoxLayout spacing between a caption above and its field.
        rowSpacing: 6
        Text {
            id: caption
            text: root.labelText
            verticalAlignment: Text.AlignVCenter
            // Above the field the caption takes what the field leaves.
            Layout.fillHeight: root.labelPosition === "above"
            visible: root.labelText !== ""
            color: Theme.windowText
            // A QLabel squeezed by its layout cuts the caption instead of
            // drawing it over the field.
            clip: true
            // Beside the field the caption gives way first: it shrinks below
            // its text width before the field goes under its minimum.
            Layout.fillWidth: true
            // Whole pixels like a QLabel size hint, so the field starts on the
            // same column as its Widgets twin.
            Layout.preferredWidth: Math.ceil(implicitWidth)
            Layout.maximumWidth: root.labelPosition === "left" ? Math.ceil(implicitWidth) : Number.POSITIVE_INFINITY
            Layout.alignment: Qt.AlignVCenter
        }
        TextField {
            id: field
            objectName: "field"
            // Display-only fields are disabled line edits in the Widgets windows.
            color: readOnly ? Theme.disabledText : Theme.windowText
            placeholderTextColor: Theme.placeholderText
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Theme.fieldHeight
            // The Widgets field keeps its 22 px height inside a taller input,
            // centred beside its label or under it when the label is above.
            // Under a caption a short input squeezes the field, not the
            // caption (38 px: 14 caption + 6 + 18 field).
            Layout.maximumHeight: root.labelPosition === "above" ? Math.min(Theme.fieldHeight, root.height - 6 - Math.ceil(caption.implicitHeight)) : Theme.fieldHeight
            // Preferred at its minimum and grown by fillWidth, so a long label
            // cannot squeeze it away (QLineEdit::minimumSizeHint in Widgets).
            Layout.minimumWidth: Theme.fieldMinimumWidth + calendarButton.width
            Layout.preferredWidth: Theme.fieldMinimumWidth + calendarButton.width
            Layout.alignment: root.labelPosition === "above" ? Qt.AlignBottom : Qt.AlignTop
            // QBoxLayout centres with integer division: an odd spare pixel
            // goes below the field, never half above and half below.
            Layout.topMargin: root.labelPosition === "above" ? 0 : Math.max(0, Math.floor((root.height - Theme.fieldHeight) / 2))
            verticalAlignment: TextInput.AlignVCenter
            // QLineEdit text margins: 3 px on the left, 5 on the right, and no
            // vertical padding: the Basic style's 6 px would leave less than a
            // text line in the 21-22 px fields (the text is centred instead).
            topPadding: 0
            bottomPadding: 0
            leftPadding: 3
            rightPadding: calendarButton.width + 8
            // A display-only field is a disabled line edit in Widgets: the
            // tab order and the initial focus skip it.
            activeFocusOnTab: !readOnly
            placeholderText: "dd/mm/yyyy"
            validator: RegularExpressionValidator {
                regularExpression: /^(\d{2}\/\d{2}\/\d{4})?$/
            }
            background: Rectangle {
                objectName: "fieldBackground"
                color: root.fieldBackground()
                border.color: field.activeFocus ? Theme.focusBorder : Theme.inputBorder
                border.width: 1
                radius: 2
                // Sunken line under the top border, as Fusion paints it.
                Rectangle {
                    x: 1
                    y: 1
                    width: parent.width - 2
                    height: 1
                    color: Theme.fieldShadow
                    visible: !field.activeFocus
                }
                // Fusion focus frame: highlight border plus a light inner ring.
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: 1
                    color: "transparent"
                    border.color: Theme.focusRing
                    border.width: 1
                    visible: field.activeFocus
                }
            }
            Button {
                id: calendarButton
                objectName: "calendarButton"
                anchors.right: parent.right
                anchors.rightMargin: 4
                y: Math.floor((parent.height - height) / 2)
                width: 22
                height: 18
                // 16 px icon centred in the 22x18 button, like QLineEdit's
                // trailing action.
                leftPadding: 3
                rightPadding: 3
                topPadding: 1
                bottomPadding: 1
                focusPolicy: Qt.NoFocus
                enabled: !field.readOnly
                background: Rectangle {
                    color: "transparent"
                }
                // Same 16 px glyph UxDateField paints: an outlined page with a
                // solid header and two rings. Drawn with shapes (the 1.2 px
                // strokes of the original) so it stays sharp in a zoomed
                // window.
                contentItem: Item {
                    objectName: "calendarIcon"
                    implicitWidth: 16
                    implicitHeight: 16
                    Rectangle {
                        objectName: "calendarPage"
                        x: 1.4
                        y: 2.4
                        width: 13.2
                        height: 12.2
                        radius: 2.1
                        color: "transparent"
                        border.color: Theme.windowText
                        border.width: 1.2
                        antialiasing: true
                    }
                    Rectangle {
                        x: 1.4
                        y: 2.4
                        width: 13.2
                        height: 4.2
                        color: Theme.windowText
                        antialiasing: true
                    }
                    Rectangle {
                        x: 4.4
                        y: 1
                        width: 1.2
                        height: 3
                        color: Theme.windowText
                        antialiasing: true
                    }
                    Rectangle {
                        x: 10.4
                        y: 1
                        width: 1.2
                        height: 3
                        color: Theme.windowText
                        antialiasing: true
                    }
                }
                onClicked: root.openCalendar()
            }
            onTextEdited: root.textEdited()
        }
    }

    // Parent of the calendar popup: the window overlay, so it is painted on top
    // of the panel instead of being clipped inside the field's own item.
    Popup {
        id: calendarPopup
        objectName: "calendarPopup"
        parent: Overlay.overlay
        scale: root.zoomFactor
        transformOrigin: Popup.TopLeft
        width: 260
        height: 280
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            color: Theme.fieldBackground
            border.color: Theme.inputBorder
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
                // The interface language's month name (Qt.formatDate with a
                // format string is always English).
                Text {
                    objectName: "calendarMonth"
                    width: parent.width - 72
                    height: 32
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.windowText
                    font.bold: true
                    text: root.sentenceCase(InterfaceSettings.locale.standaloneMonthName(root.shownMonth, Locale.LongFormat)) + " " + root.shownYear
                }
                Button {
                    text: "\u203A"
                    width: 32
                    focusPolicy: Qt.NoFocus
                    onClicked: root.shiftMonth(1)
                }
            }
            // Both rows take the interface language's locale, which also starts
            // the week on Monday, never the system's Qt.locale(). Without the
            // Basic style's 6 px spacing a column is 33.7 px wide, room for the
            // widest bold name ("Dom", 31 px); the rows must keep the same
            // spacing so the names stay above their days.
            DayOfWeekRow {
                id: dayRow
                width: parent.width
                spacing: 0
                locale: InterfaceSettings.locale
                // Qt's short names are whole words in Portuguese ("segunda"):
                // three letters are the usual abbreviation in all three
                // languages.
                delegate: Text {
                    objectName: "dayName"
                    required property string shortName
                    text: root.sentenceCase(shortName.slice(0, 3))
                    font: dayRow.font
                    color: Theme.windowText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            MonthGrid {
                id: monthGrid
                objectName: "monthGrid"
                width: parent.width
                height: 180
                spacing: 0
                month: root.shownMonth
                year: root.shownYear
                locale: InterfaceSettings.locale
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
