import QtQuick

// Frameless centered splash with the logo image and a credits strip at the
// bottom (mirrors the Widgets SplashWindow). Closes after three seconds and
// emits finished(). Centering on the primary screen is done by the loader
// (see main.cpp), like the Widgets splash.
Window {
    id: root
    flags: Qt.FramelessWindowHint | Qt.SplashScreen
    visible: true
    width: 560
    height: 470
    title: "Gambasse"
    color: "white"

    signal finished

    Image {
        objectName: "splashImage"
        width: 560
        height: 430
        source: "qrc:/img/logo-historias.jpg"
        fillMode: Image.Stretch
    }
    Rectangle {
        y: 430
        width: parent.width
        height: 40
        color: "#24292E"
        Row {
            anchors.centerIn: parent
            spacing: 6
            Image {
                source: "qrc:/img/github.svg"
                sourceSize.width: 16
                sourceSize.height: 16
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: qsTr('Project by Juan Franco, available on <a href="%1" style="color:#58A6FF; text-decoration:none;">GitHub</a>.').arg("https://github.com/madialeva/gambasse")
                textFormat: Text.RichText
                color: "white"
                font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
                onLinkActivated: link => Qt.openUrlExternally(link)
            }
        }
    }
    Timer {
        interval: 3000
        running: true
        repeat: false
        onTriggered: {
            root.finished();
            root.close();
        }
    }
}
