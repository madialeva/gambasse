#include <logic/PhotoManager.h>

#include <Paths.h>
#include <data/model/Patient.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace gambasse {

QString PhotoManager::photoPath(const Patient& patient) const {
    return QDir(basePath()).filePath(patient.photoFilename());
}

QString PhotoManager::legacyPhotoPath(const Patient& patient) const {
    return QDir(basePath()).filePath(QStringLiteral("fotos/%1.jpg").arg(patient.id));
}

QString PhotoManager::defaultPhotoResource() const {
    return QStringLiteral(":/img/foto0.png");
}

void PhotoManager::renameFor(const Patient& previous, const Patient& current) const {
    const QString oldName = previous.photoFilename();
    const QString newName = current.photoFilename();
    if (oldName == newName)
        return;
    const QString oldPath = QDir(basePath()).filePath(oldName);
    const QString newPath = QDir(basePath()).filePath(newName);
    if (QFileInfo::exists(oldPath))
        QFile::rename(oldPath, newPath);
}

void PhotoManager::removeFor(const Patient& patient) const {
    const QString path = photoPath(patient);
    if (QFileInfo::exists(path))
        QFile::remove(path);
}

} // namespace gambasse
