import QtQuick
import QtQuick.Controls
import Gambasse

// Product information, opened from the Help menu of the title bar: a GxDialog
// (theme, modality, deferred opening with present()) with its own content. The
// versions come from the build, through AppInfo.
GxDialog {
    id: root

    component CentredText: Text {
        width: parent.width
        color: Theme.windowText
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
    }

    // StyledText: linkColor does not apply to RichText.
    component LinkedText: CentredText {
        id: link
        textFormat: Text.StyledText
        linkColor: Theme.linkText
        HoverHandler {
            cursorShape: link.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
    }

    // The address is shown without its scheme and is not translated.
    component RepositoryLink: LinkedText {
        text: '<a href="%1">%2</a>'.arg(AppInfo.repositoryUrl).arg(AppInfo.repositoryUrl.replace(/^https:\/\//, ""))
        onLinkActivated: url => Qt.openUrlExternally(url)
    }

    // The licence of the embedded font, shown from inside the application: no
    // file goes with the Windows executable.
    GxDialog {
        id: fontLicenseDialog
        objectName: "fontLicenseDialog"
        parent: root.parent
        width: 640
        contentItem: Column {
            spacing: 12
            width: fontLicenseDialog.availableWidth
            Flickable {
                objectName: "fontLicenseView"
                width: parent.width
                height: 420
                contentHeight: fontLicenseText.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                }
                Text {
                    id: fontLicenseText
                    objectName: "fontLicenseText"
                    width: parent.width - 16
                    color: Theme.windowText
                    wrapMode: Text.Wrap
                    textFormat: Text.PlainText
                    text: AppInfo.fontLicense
                }
            }
            GxActionButton {
                objectName: "fontLicenseOkButton"
                anchors.right: parent.right
                text: qsTr("OK")
                onClicked: fontLicenseDialog.close()
            }
        }
    }

    contentItem: Column {
        spacing: 12
        width: root.availableWidth
        Image {
            objectName: "aboutLogo"
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/img/logo.svg"
            sourceSize.width: 96
            sourceSize.height: 96
        }
        Column {
            width: parent.width
            spacing: 2
            CentredText {
                objectName: "aboutName"
                text: "Gambasse"
                font.bold: true
                font.pixelSize: 15
            }
            CentredText {
                objectName: "aboutVersion"
                text: qsTr("Version %1").arg(AppInfo.version)
            }
            CentredText {
                objectName: "aboutLicense"
                text: qsTr("MIT License")
            }
        }
        Column {
            width: parent.width
            spacing: 4
            CentredText {
                objectName: "aboutLanguage"
                text: qsTr("Developed in %1, an ISO standard.").arg(AppInfo.languageStandard)
            }
            CentredText {
                objectName: "aboutCompiler"
                text: qsTr("Compiled with GCC %1, under the GPL v3 with the GCC Runtime Library Exception.").arg(AppInfo.compilerVersion)
            }
            CentredText {
                objectName: "aboutQt"
                text: qsTr("Built with Qt %1, under the LGPL v3.").arg(AppInfo.qtVersion)
            }
            LinkedText {
                objectName: "aboutFont"
                text: qsTr("Uses the Noto Sans font, under the %1.").arg('<a href="license">SIL OFL 1.1</a>')
                onLinkActivated: fontLicenseDialog.present()
            }
        }
        RepositoryLink {
            objectName: "aboutLink"
        }
        GxActionButton {
            objectName: "aboutOkButton"
            anchors.right: parent.right
            text: qsTr("OK")
            onClicked: root.close()
        }
    }
}
