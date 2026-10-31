import QtQuick
import QtQuick.Dialogs

// File chooser for the patient photo: the native dialog on Windows, the Qt
// Quick one on Linux. It lives in its own directory, with a .qmllint.ini of
// its own, because qmllint 6.6 reports the style variants of QtQuick.Dialogs
// as ambiguous types, and those reports carry no line a directive could mute.
FileDialog {
    title: qsTr("Choose the patient's photo")
    fileMode: FileDialog.OpenFile
    nameFilters: [qsTr("Images (*.jpg *.jpeg *.png *.bmp)"), qsTr("All files (*)")]
}
