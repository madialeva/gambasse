#pragma once

#include <QString>

namespace gambasse {

// Persists application settings (interface language, visual theme and the
// letter size of the history and consultation windows) in the config.ini file
// located at the base path.
class AppSettings {
public:
    QString language() const;
    void setLanguage(const QString& code) const;
    QString theme() const;
    void setTheme(const QString& mode) const;
    // Letter size chosen for the zoom of the history and consultation
    // windows (letra/pixeles): 13 when missing or not a whole number, and
    // brought into the zoom range otherwise.
    int letterSize() const;
    void setLetterSize(int pixels) const;
};

} // namespace gambasse
