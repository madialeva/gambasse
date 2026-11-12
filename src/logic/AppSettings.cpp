#include <logic/AppSettings.h>

#include <Paths.h>
#include <logic/ZoomLevels.h>

#include <QDir>
#include <QSettings>

namespace gambasse {
namespace {

QString configFilePath() {
    return QDir(basePath()).filePath(QStringLiteral("config.ini"));
}

} // namespace

QString AppSettings::language() const {
    QSettings config(configFilePath(), QSettings::IniFormat);
    return config.value(QStringLiteral("idioma/codigo"), QStringLiteral("pt")).toString();
}

void AppSettings::setLanguage(const QString& code) const {
    QSettings config(configFilePath(), QSettings::IniFormat);
    config.setValue(QStringLiteral("idioma/codigo"), code);
}

QString AppSettings::theme() const {
    QSettings config(configFilePath(), QSettings::IniFormat);
    return config.value(QStringLiteral("tema/modo"), QStringLiteral("claro")).toString();
}

void AppSettings::setTheme(const QString& mode) const {
    QSettings config(configFilePath(), QSettings::IniFormat);
    config.setValue(QStringLiteral("tema/modo"), mode);
}

int AppSettings::letterSize() const {
    QSettings config(configFilePath(), QSettings::IniFormat);
    bool ok = false;
    const int pixels = config.value(QStringLiteral("letra/pixeles")).toString().trimmed().toInt(&ok);
    return ok ? zoom::clampLetterSize(pixels) : zoom::baseLetterSize;
}

void AppSettings::setLetterSize(int pixels) const {
    QSettings config(configFilePath(), QSettings::IniFormat);
    config.setValue(QStringLiteral("letra/pixeles"), zoom::clampLetterSize(pixels));
}

} // namespace gambasse
