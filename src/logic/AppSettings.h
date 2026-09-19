#pragma once

#include <QString>

namespace gambasse {

// Persists application settings (interface language and visual theme) in the
// config.ini file located at the base path.
class AppSettings {
public:
    QString language() const;
    void setLanguage(const QString& code) const;
    QString theme() const;
    void setTheme(const QString& mode) const;
};

} // namespace gambasse
