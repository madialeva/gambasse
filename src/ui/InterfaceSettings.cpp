#include <ui/InterfaceSettings.h>

#include <QCoreApplication>
#include <QDir>
#include <QQmlEngine>

#include <Paths.h>

namespace gambasse {

InterfaceSettings::InterfaceSettings(QQmlEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine), m_theme(m_store.theme()),
      m_language(m_store.language()) {
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

InterfaceSettings* InterfaceSettings::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine) {
    Q_UNUSED(jsEngine);
    auto* settings = new InterfaceSettings(qmlEngine);
    settings->applyLanguage();
    return settings;
}

void InterfaceSettings::setLanguage(const QString& code) {
    if (code == m_language)
        return;
    m_language = code;
    m_store.setLanguage(m_language);
    // Catalog and QML strings first, so whoever reacts to the signal (the
    // patient grid headers) already reads the new language.
    applyLanguage();
    emit languageChanged();
}

void InterfaceSettings::applyLanguage() {
    // Only the engine's singleton owns the application catalog; a detached
    // instance (settings round trips in tests) just persists.
    if (!m_engine)
        return;
    // Embedded resource first, <base>/translations as fallback; English needs
    // no catalog.
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
    m_engine->retranslate();
}

} // namespace gambasse
