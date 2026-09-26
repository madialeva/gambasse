#pragma once

#include <QObject>
#include <QString>
#include <QTranslator>

#include <logic/AppSettings.h>

class QQmlEngine;

namespace gambasse {

// Session-facing interface settings for both UI stacks: holds the current
// theme and language, persists them through AppSettings (config.ini at the
// base path) and notifies on change so QML bindings update in hot.
// QtCore only (plus a QML forward declaration): no Widgets, no Quick,
// testable without a GUI.
class InterfaceSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
public:
    explicit InterfaceSettings(QObject* parent = nullptr);

    QString theme() const { return m_theme; }
    void setTheme(const QString& mode);
    QString language() const { return m_language; }
    void setLanguage(const QString& code);

    // Installs the catalog for the current language (English needs none)
    // and refreshes every QML-bound string. Call once at startup and again
    // on every languageChanged.
    void applyLanguage(QQmlEngine* engine);

signals:
    void themeChanged();
    void languageChanged();

private:
    AppSettings m_store;
    QTranslator m_translator;
    QString m_theme;
    QString m_language;
};

} // namespace gambasse
