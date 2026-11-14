import QtQuick
import Gambasse

// A zoomed child window with combos and a date field placed so that their
// popups have room below (top) or only above (low): the forms of the
// application never need the second case.
FormWindow {
    contentHeight: 500

    GxComboInput {
        objectName: "topCombo"
        x: 20
        y: 60
        width: 200
        height: 22
        Component.onCompleted: {
            for (let i = 0; i < 10; ++i)
                addItem("Option " + i, i);
        }
    }
    GxComboInput {
        objectName: "lowCombo"
        x: 300
        y: 500
        width: 200
        height: 22
        Component.onCompleted: {
            for (let i = 0; i < 10; ++i)
                addItem("Option " + i, i);
        }
    }
    GxDateInput {
        objectName: "topDate"
        x: 600
        y: 60
        width: 200
        height: 22
    }
    GxDateInput {
        objectName: "lowDate"
        x: 600
        y: 500
        width: 200
        height: 22
    }
}
