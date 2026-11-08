#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace gambasse {

// What the views say about the product. The versions come from the build (CMake
// and the running Qt), so they cannot go out of date. Registered as the QML
// singleton `AppInfo`; it only holds constants, so unlike InterfaceSettings it
// needs no engine and the default constructor is enough.
class AppInfo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString languageStandard READ languageStandard CONSTANT)
    Q_PROPERTY(QString compilerVersion READ compilerVersion CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString repositoryUrl READ repositoryUrl CONSTANT)
public:
    explicit AppInfo(QObject* parent = nullptr);

    QString version() const;
    QString languageStandard() const;
    QString compilerVersion() const;
    QString qtVersion() const;
    QString repositoryUrl() const;
};

} // namespace gambasse
