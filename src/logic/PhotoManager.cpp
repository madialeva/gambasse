#include <logic/PhotoManager.h>

#include <Paths.h>
#include <data/model/Patient.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QSaveFile>

namespace gambasse {

QString PhotoManager::photoPath(const Patient& patient) const {
    return QDir(basePath()).filePath(patient.photoFilename());
}

QString PhotoManager::legacyPhotoPath(const Patient& patient) const {
    return QDir(basePath()).filePath(QStringLiteral("fotos/%1.jpg").arg(patient.id));
}

QString PhotoManager::existingPhotoPath(const Patient& patient) const {
    for (const QString& path : {photoPath(patient), legacyPhotoPath(patient)}) {
        if (QFileInfo::exists(path))
            return path;
    }
    return QString();
}

bool PhotoManager::hasPhoto(const Patient& patient) const {
    return !existingPhotoPath(patient).isEmpty();
}

QString PhotoManager::defaultPhotoResource() const {
    return QStringLiteral(":/img/foto0.png");
}

PhotoManager::ImportResult PhotoManager::importFor(const Patient& patient,
                                                   const QString& sourceFile) const {
    const QSize frame(kPhotoWidth, kPhotoHeight);
    QImageReader reader(sourceFile);
    // Phone pictures carry their orientation in EXIF (the original application
    // ignored it and stored them sideways).
    reader.setAutoTransform(true);
    // Large pictures are decoded already reduced when the format allows it, so
    // a photo of many megapixels is never loaded whole. The reader reports the
    // stored size, before the EXIF rotation, so the frame is turned to match.
    const QSize stored = reader.size();
    if (stored.isValid()) {
        const bool rotated = reader.transformation() & QImageIOHandler::TransformationRotate90;
        const QSize bound = rotated ? frame.transposed() : frame;
        if (stored.width() > bound.width() || stored.height() > bound.height())
            reader.setScaledSize(stored.scaled(bound, Qt::KeepAspectRatio));
    }
    const QImage image = reader.read();
    if (image.isNull())
        return ImportResult::UnreadableImage;

    // Reduced to fit the frame keeping its proportions, never enlarged.
    const QImage fitted = image.width() > frame.width() || image.height() > frame.height()
                              ? image.scaled(frame, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                              : image;
    // JPEG has no alpha channel: transparent areas become white, not black.
    QImage flat(fitted.size(), QImage::Format_RGB32);
    flat.fill(Qt::white);
    {
        QPainter painter(&flat);
        painter.drawImage(0, 0, fitted);
    }

    // The whole picture has been read, so the source may be the target itself.
    const QString target = photoPath(patient);
    if (!QDir().mkpath(QFileInfo(target).absolutePath()))
        return ImportResult::WriteError;
    QSaveFile file(target);
    if (!file.open(QIODevice::WriteOnly))
        return ImportResult::WriteError;
    QImageWriter writer(&file, "jpeg");
    writer.setQuality(kJpegQuality);
    if (!writer.write(flat) || !file.commit())
        return ImportResult::WriteError;

    // A legacy file would show again once the new photo is deleted.
    const QString legacy = legacyPhotoPath(patient);
    if (QFileInfo::exists(legacy))
        QFile::remove(legacy);
    return ImportResult::Imported;
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

bool PhotoManager::removeFor(const Patient& patient) const {
    for (const QString& path : {photoPath(patient), legacyPhotoPath(patient)}) {
        if (QFileInfo::exists(path))
            QFile::remove(path);
    }
    return !hasPhoto(patient);
}

} // namespace gambasse
