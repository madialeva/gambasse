import QtQuick

// Test fixture only (never shipped, never scanned by lupdate): probes hot
// QML retranslation through a production catalog entry, so the catalogs
// themselves stay untouched.
Item {
    property string probed: qsTranslate("DomesticAnimals", "In stable")
}
