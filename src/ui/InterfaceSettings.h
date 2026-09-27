#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QTranslator>
#include <QtQml/qqmlregistration.h>

#include <logic/AppSettings.h>

class QJSEngine;
class QQmlEngine;

namespace gambasse {

// Interface settings: the current theme and language, persisted through
// AppSettings (config.ini at the base path). Registered as the QML singleton
// `InterfaceSettings`, created by the engine: that instance installs the
// language catalog and retranslates the engine on every change before
// announcing it, so views and controllers only react to languageChanged.
// QtCore only (plus QML forward declarations): no Quick, testable without a
// GUI.
class InterfaceSettings : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
public:
    // Bound to `engine`, the instance owns the application catalog and
    // retranslates that engine; with a null engine it only reads and persists
    // the settings. There is deliberately no default constructor: QML would
    // prefer it over create() and the singleton would lose its engine.
    explicit InterfaceSettings(QQmlEngine* engine, QObject* parent = nullptr);

    // Singleton factory: binds the instance to its engine and installs the
    // catalog of the persisted language.
    static InterfaceSettings* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    QString theme() const { return m_theme; }
    void setTheme(const QString& mode);
    QString language() const { return m_language; }
    void setLanguage(const QString& code);

signals:
    void themeChanged();
    void languageChanged();

private:
    // Installs the catalog for the current language (English needs none) and
    // refreshes every QML-bound string of the bound engine.
    void applyLanguage();

    AppSettings m_store;
    QTranslator m_translator;
    QPointer<QQmlEngine> m_engine;
    QString m_theme;
    QString m_language;
};

} // namespace gambasse
