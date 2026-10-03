import QtQuick
import QtQuick.Controls
import Gambasse

// Accent button of the form windows (Delete / Save / Exit of the histories,
// the four consultation buttons): bold caption on the theme's blue, with
// pressed, hover and disabled states.
Button {
    id: actionButton
    font.bold: true
    background: Rectangle {
        color: !actionButton.enabled ? Theme.accentDisabledBackground : (actionButton.down ? Theme.accentPressed : (actionButton.hovered ? Theme.accentHover : Theme.accentBackground))
        border.color: !actionButton.enabled ? Theme.accentDisabledBorder : Theme.accentBorder
        radius: 4
    }
    contentItem: Text {
        text: actionButton.text
        color: !actionButton.enabled ? Theme.accentDisabledText : Theme.accentText
        font: actionButton.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
