pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Gambasse

// Labeled combo box (mirrors UxComboInput): label on the left or above the
// box, key/value items added programmatically with the persisted datum
// independent of the language, and optional in-place editing.
Item {
    id: root

    property string labelText: ""
    property string labelPosition: "left" // or "above"
    property alias currentIndex: combo.currentIndex
    property alias currentText: combo.currentText
    // Typed text in editable mode (mirrors QComboBox::currentText(), which
    // always reflects the line edit; in QML currentText follows selection).
    property alias editText: combo.editText
    property alias editable: combo.editable
    // Internal editor, for direct focus control in tests and windows.
    property alias editor: combo.contentItem
    // Emitted when the user picks an entry (never on a programmatic change).
    signal valueSelected

    function addItem(text, value) {
        comboModel.append({
            "text": text,
            "value": value
        });
    }

    function clear() {
        comboModel.clear();
    }

    function findData(value) {
        for (let i = 0; i < comboModel.count; ++i) {
            if (comboModel.get(i).value === value)
                return i;
        }
        return -1;
    }

    function currentData() {
        if (combo.currentIndex < 0 || combo.currentIndex >= comboModel.count)
            return 0;
        return comboModel.get(combo.currentIndex).value;
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
        ComboBox {
            id: combo
            objectName: "comboBox"
            textRole: "text"
            valueRole: "value"
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
            Layout.minimumWidth: Theme.fieldMinimumWidth
            Layout.preferredWidth: Theme.fieldMinimumWidth
            Layout.alignment: root.labelPosition === "above" ? Qt.AlignBottom : Qt.AlignTop
            // QBoxLayout centres with integer division: an odd spare pixel
            // goes below the field, never half above and half below.
            Layout.topMargin: root.labelPosition === "above" ? 0 : Math.max(0, Math.floor((root.height - Theme.fieldHeight) / 2))
            model: ListModel {
                id: comboModel
            }
            onActivated: root.valueSelected()

            // Height of a drop-down entry: one text line plus 4 px above and
            // below, compact like the QComboBox list.
            readonly property int entryHeight: Math.ceil(entryMetrics.height) + 8
            FontMetrics {
                id: entryMetrics
                font: combo.font
            }

            // Drop-down list: at most about ten entries (like QComboBox) and
            // never beyond the window, with a scroll bar shown whenever the
            // entries do not fit. The Basic popup only had a scroll indicator,
            // visible while scrolling, so a long list (the active
            // ingredients) gave no hint that it went on.
            popup: Popup {
                id: dropDown
                objectName: "comboPopup"
                readonly property int maxVisibleEntries: 10
                y: combo.height
                width: combo.width
                height: Math.min(Math.min(combo.count, dropDown.maxVisibleEntries) * combo.entryHeight + topPadding + bottomPadding, combo.Window.height - topMargin - bottomMargin)
                topMargin: 6
                bottomMargin: 6
                padding: 1
                contentItem: ListView {
                    id: dropDownList
                    objectName: "comboPopupList"
                    readonly property int entryHeight: combo.entryHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: dropDown.visible ? combo.delegateModel : null
                    currentIndex: combo.highlightedIndex
                    highlightMoveDuration: 0
                    ScrollBar.vertical: ScrollBar {
                        id: dropDownBar
                        objectName: "comboScrollBar"
                        policy: dropDownList.contentHeight > dropDownList.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                        // A handle of at least a sixth of the bar, even for
                        // the 153 active ingredients.
                        minimumSize: 1 / 6
                        // A faint track, so the bar reads as one on both themes.
                        background: Rectangle {
                            implicitWidth: 10
                            color: Theme.tabInactiveBottom
                        }
                        // Theme colours: the Basic handle follows palette.dark,
                        // which is the field background in the dark theme.
                        contentItem: Rectangle {
                            objectName: "comboScrollHandle"
                            implicitWidth: 6
                            radius: 3
                            color: Theme.comboArrow
                            opacity: dropDownBar.pressed ? 1.0 : 0.7
                        }
                    }
                }
                background: Rectangle {
                    color: Theme.fieldBackground
                    border.color: Theme.inputBorder
                    border.width: 1
                }
            }

            // Popup entries with the theme colours. The Basic delegate paints
            // the highlighted entry with the palette's light colour (near
            // white) and its text with highlightedText, which is white in the
            // light theme: the entry under the cursor lost its text.
            delegate: ItemDelegate {
                id: entry
                required property var model
                required property int index
                objectName: "comboEntry"
                width: ListView.view ? ListView.view.width : combo.width
                height: combo.entryHeight
                topPadding: 0
                bottomPadding: 0
                leftPadding: 6
                rightPadding: 6
                text: entry.model.text
                font: combo.font
                highlighted: combo.highlightedIndex === entry.index
                hoverEnabled: combo.hoverEnabled
                contentItem: Text {
                    objectName: "comboEntryText"
                    text: entry.text
                    font: entry.font
                    color: entry.highlighted ? Theme.highlightedText : Theme.windowText
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    objectName: "comboEntryBackground"
                    color: entry.highlighted ? Theme.highlight : Theme.fieldBackground
                }
            }

            // Fusion look of the Widgets QComboBox: a raised gradient button
            // with a small arrow; an editable one shows a line edit on the
            // left and the arrow button, split by a line, on the right.
            // The inner text field adds its own 4 px: text 6 px from the border.
            leftPadding: 1
            rightPadding: 22
            palette.text: Theme.windowText
            palette.buttonText: Theme.windowText
            palette.base: Theme.fieldBackground
            palette.window: Theme.fieldBackground
            palette.button: Theme.buttonTop
            palette.dark: Theme.inputBorder
            palette.mid: Theme.inputBorder
            palette.highlight: Theme.highlight
            palette.highlightedText: Theme.highlightedText
            background: Rectangle {
                objectName: "comboBackground"
                radius: 2
                border.color: Theme.inputBorder
                border.width: 1
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: Theme.buttonTop
                    }
                    GradientStop {
                        position: 1.0
                        color: Theme.buttonBottom
                    }
                }
                Rectangle {
                    x: 1
                    y: 1
                    width: parent.width - 2
                    height: 1
                    color: Theme.buttonTopLine
                    visible: !combo.editable
                }
                Rectangle {
                    x: 1
                    y: parent.height - 2
                    width: parent.width - 2
                    height: 1
                    color: Theme.buttonBottomLine
                }
                // Editable: the line edit part.
                Rectangle {
                    visible: combo.editable
                    x: 1
                    y: 1
                    width: parent.width - 21
                    height: parent.height - 2
                    color: combo.activeFocus ? Theme.focusBackground : Theme.fieldBackground
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: Theme.fieldShadow
                    }
                    Rectangle {
                        anchors.right: parent.right
                        width: 1
                        height: parent.height
                        color: Theme.checkBorder
                    }
                }
            }
            // Same wiring as the Basic style's text field, without its own
            // background (the line edit part is painted by the background).
            contentItem: TextField {
                objectName: "comboText"
                // The editable QComboBox line edit starts one pixel earlier.
                leftPadding: combo.editable ? 3 : 4
                rightPadding: 4
                topPadding: 0
                bottomPadding: 0
                text: combo.editable ? combo.editText : combo.displayText
                enabled: combo.editable
                autoScroll: combo.editable
                readOnly: combo.down
                inputMethodHints: combo.inputMethodHints
                validator: combo.validator
                selectByMouse: combo.selectTextByMouse
                color: Theme.windowText
                selectionColor: Theme.highlight
                selectedTextColor: Theme.highlightedText
                verticalAlignment: Text.AlignVCenter
                background: null
            }
            // 7-5-3-1 px triangle centred 11 px left of the right border.
            indicator: Canvas {
                id: arrow
                x: combo.width - 15
                y: Math.floor((combo.height - 4) / 2)
                width: 7
                height: 4
                onPaint: {
                    const ctx = getContext("2d");
                    ctx.reset();
                    ctx.fillStyle = Theme.comboArrow;
                    for (let row = 0; row < 4; ++row)
                        ctx.fillRect(row, row, 7 - 2 * row, 1);
                }
                Connections {
                    target: Theme
                    function onDarkChanged() {
                        arrow.requestPaint();
                    }
                }
            }
        }
    }
}
