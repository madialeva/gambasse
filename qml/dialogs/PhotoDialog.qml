import QtQuick
import QtQuick.Dialogs

// File chooser for the patient photo: the native dialog on Windows, the Qt
// Quick one on Linux.
FileDialog {
    title: qsTr("Choose the patient's photo")
    fileMode: FileDialog.OpenFile
    nameFilters: [qsTr("Images (*.jpg *.jpeg *.png *.bmp)"), qsTr("All files (*)")]
}
