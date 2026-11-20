import QtQuick
import Gambasse

// Application entry point loaded by main.cpp: shows the splash and, once it
// finishes, creates the main shell. The splash is centred on its screen; the
// shell applies its own persisted geometry and state.
QtObject {
    id: app

    // The main shell, created when the splash is over.
    property Root shell: null

    property Component shellComponent: Component {
        Root {}
    }

    property SplashWindow splash: SplashWindow {
        id: splashWindow
        Component.onCompleted: {
            splashWindow.x = Screen.virtualX + Math.round((Screen.width - splashWindow.width) / 2);
            splashWindow.y = Screen.virtualY + Math.round((Screen.height - splashWindow.height) / 2);
        }
        onFinished: app.shell = app.shellComponent.createObject(app) as Root
    }
}
