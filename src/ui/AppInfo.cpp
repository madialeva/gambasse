#include <ui/AppInfo.h>

#include <QCoreApplication>
#include <QFile>
#include <QtVersion>

namespace gambasse {

AppInfo::AppInfo(QObject* parent) : QObject(parent) {}

QString AppInfo::version() const {
    return QCoreApplication::applicationVersion();
}

QString AppInfo::languageStandard() const {
    return QStringLiteral("C++" GAMBASSE_CXX_STANDARD);
}

QString AppInfo::compilerVersion() const {
    return QStringLiteral(GAMBASSE_COMPILER_VERSION);
}

QString AppInfo::qtVersion() const {
    return QString::fromLatin1(qVersion());
}

QString AppInfo::repositoryUrl() const {
    return QStringLiteral("https://github.com/madialeva/gambasse");
}

QString AppInfo::fontLicense() const {
    QFile file(QStringLiteral(":/fonts/OFL.txt"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}

} // namespace gambasse
