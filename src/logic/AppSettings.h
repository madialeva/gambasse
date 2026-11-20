#pragma once

#include <QString>

#include <logic/MainWindowGeometry.h>

namespace gambasse {

// Persists application settings (interface language, visual theme, shared
// letter size and main window geometry) in the config.ini file located at the
// base path.
class AppSettings {
public:
    QString language() const;
    void setLanguage(const QString& code) const;
    QString theme() const;
    void setTheme(const QString& mode) const;
    // Shared letter size chosen for the zoom of the main, history and
    // consultation windows (letra/pixeles): 13 when missing or not a whole
    // number, and brought into the zoom range otherwise.
    int letterSize() const;
    void setLetterSize(int pixels) const;
    // Main window state and preferred normal size (ventana/maximizada,
    // ventana/ancho, ventana/alto): maximized and 1008x561 when missing or
    // invalid, never below the 1008x561 base size.
    MainWindowGeometry mainWindowGeometry() const;
    void setMainWindowGeometry(const MainWindowGeometry& geometry) const;
};

} // namespace gambasse
