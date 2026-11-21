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

MainWindowGeometry AppSettings::mainWindowGeometry() const {
    QSettings config(configFilePath(), QSettings::IniFormat);
    MainWindowGeometry geometry;
    const QString rawMaximized =
        config.value(QStringLiteral("ventana/maximizada")).toString().trimmed().toLower();
    if (rawMaximized == QLatin1String("false"))
        geometry.maximized = false;
    else if (rawMaximized == QLatin1String("true"))
        geometry.maximized = true;

    bool widthOk = false;
    const int rawWidth =
        config.value(QStringLiteral("ventana/ancho")).toString().trimmed().toInt(&widthOk);
    bool heightOk = false;
    const int rawHeight =
        config.value(QStringLiteral("ventana/alto")).toString().trimmed().toInt(&heightOk);
    const QSize base = mainwindow::baseSize();
    geometry.normalSize = mainwindow::clampNormalSize(
        {widthOk ? rawWidth : base.width(), heightOk ? rawHeight : base.height()});
    return geometry;
}

void AppSettings::setMainWindowGeometry(const MainWindowGeometry& geometry) const {
    QSettings config(configFilePath(), QSettings::IniFormat);
    const QSize clamped = mainwindow::clampNormalSize(geometry.normalSize);
    config.setValue(QStringLiteral("ventana/maximizada"), geometry.maximized);
    config.setValue(QStringLiteral("ventana/ancho"), clamped.width());
    config.setValue(QStringLiteral("ventana/alto"), clamped.height());
}

} // namespace gambasse
