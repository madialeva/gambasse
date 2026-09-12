#pragma once

#include <QString>
#include <QCoreApplication>

namespace gambasse {

// Application base path: directory containing config.ini, the database, fotos/,
// and translations/. The Windows launcher sets it with --base (the root directory
// above lib/). Otherwise, the executable directory is used for development builds.
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
