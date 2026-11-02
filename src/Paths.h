#pragma once

#include <QString>
#include <QCoreApplication>

namespace gambasse {

// Application base path: directory containing config.ini, the database, fotos/,
// and translations/. It is the executable's directory, unless --base sets
// another one.
inline QString& baseDirRef() {
    static QString b;
    return b;
}

inline void setBasePath(const QString& b) {
    baseDirRef() = b;
}

inline QString basePath() {
    const QString& b = baseDirRef();
    return b.isEmpty() ? QCoreApplication::applicationDirPath() : b;
}

} // namespace gambasse
