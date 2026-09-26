#include <ui/InterfaceSettings.h>

#include <QCoreApplication>
#include <QDir>
#include <QQmlEngine>

#include <Paths.h>

namespace gambasse {

InterfaceSettings::InterfaceSettings(QObject* parent)
    : QObject(parent), m_theme(m_store.theme()), m_language(m_store.language()) {
    if (m_theme != QStringLiteral("oscuro"))
        m_theme = QStringLiteral("claro");
}

void InterfaceSettings::setTheme(const QString& mode) {
    const QString normalized =
        (mode == QStringLiteral("oscuro")) ? QStringLiteral("oscuro") : QStringLiteral("claro");
    if (normalized == m_theme)
        return;
    m_theme = normalized;
    m_store.setTheme(m_theme);
    emit themeChanged();
}

void InterfaceSettings::setLanguage(const QString& code) {
    if (code == m_language)
        return;
    m_language = code;
    m_store.setLanguage(m_language);
    emit languageChanged();
}

void InterfaceSettings::applyLanguage(QQmlEngine* engine) {
    // Same catalog lookup as the Widgets MainWindow: embedded resource
    // first, <base>/translations as fallback; English needs no catalog.
    qApp->removeTranslator(&m_translator);
    if (m_language != QLatin1String("en")) {
        bool ok = m_translator.load(QStringLiteral("gambasse_") + m_language,
                                    QStringLiteral(":/i18n"));
        if (!ok)
            ok = m_translator.load(QStringLiteral("gambasse_") + m_language,
                                   QDir(basePath()).filePath(QStringLiteral("translations")));
        if (ok)
            qApp->installTranslator(&m_translator);
    }
    if (engine != nullptr)
        engine->retranslate();
}

} // namespace gambasse
