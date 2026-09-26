import QtQuick
import Gambasse

// Non-editable label (mirrors UxLabel): text with alignment and optional
// word wrap, an optional image on the left, an optional fill-with-dots mode
// and an optional hover highlight (underline + color + hand cursor) that
// emits clicked() on a left click.
Item {
    id: root

    property alias text: label.text
    property int horizontalAlignment: Text.AlignLeft
    property int verticalAlignment: Text.AlignVCenter
    property bool multiline: false
    property bool highlight: false
    property color highlightColor: Theme.labelHighlight
    property bool fillWithDots: false
    property string imageSource: ""
    readonly property alias lineCount: label.lineCount
    signal clicked

    Image {
        id: icon
        objectName: "labelIcon"
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        source: root.imageSource
        visible: root.imageSource !== ""
    }
    Text {
        id: label
        objectName: "labelText"
        anchors.fill: parent
        anchors.leftMargin: icon.visible ? icon.width + 4 : 0
        color: hoverArea.containsMouse && root.highlight ? root.highlightColor : Theme.windowText
        font.underline: hoverArea.containsMouse && root.highlight
        horizontalAlignment: root.horizontalAlignment
        verticalAlignment: root.verticalAlignment
        wrapMode: root.multiline ? Text.Wrap : Text.NoWrap
    }
    TextMetrics {
        id: metrics
        font: label.font
    }
    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: root.highlight && containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (root.highlight)
                root.clicked();
        }
    }

    function refreshDots() {
        if (!root.fillWithDots || root.multiline) {
            if (label.text !== root.text)
                label.text = root.text;
            return;
        }
        let padded = root.text;
        metrics.text = padded;
        let guard = 0;
        while (metrics.width < root.width && guard++ < 500) {
            padded += ".";
            metrics.text = padded;
        }
        if (label.text !== padded)
            label.text = padded;
    }

    onTextChanged: refreshDots()
    onWidthChanged: refreshDots()
    Component.onCompleted: refreshDots()
}
