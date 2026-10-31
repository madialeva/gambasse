#pragma once

#include <QString>

namespace gambasse {

class Patient;

// Resolves and maintains the patient photo files on disk: their paths, the
// import of a new picture (decoded, oriented, reduced and stored as JPEG) and
// their removal. Showing the image is the UI's job.
class PhotoManager {
public:
    // Stored photos fit in the 240x204 frame of the original application.
    static constexpr int kPhotoWidth = 240;
    static constexpr int kPhotoHeight = 204;
    static constexpr int kJpegQuality = 100;

    enum class ImportResult { Imported, UnreadableImage, WriteError };

    QString photoPath(const Patient& patient) const;
    QString legacyPhotoPath(const Patient& patient) const;
    // The file that holds the patient's photo: the current format, else the
    // legacy fotos/<id>.jpg, else an empty string.
    QString existingPhotoPath(const Patient& patient) const;
    bool hasPhoto(const Patient& patient) const;
    QString defaultPhotoResource() const;
    // Stores sourceFile as the patient's photo. The previous photo is only
    // replaced once the new one is fully written; a legacy file is removed.
    ImportResult importFor(const Patient& patient, const QString& sourceFile) const;
    void renameFor(const Patient& previous, const Patient& current) const;
    // Removes the current and the legacy files; true when no photo remains.
    bool removeFor(const Patient& patient) const;
};

} // namespace gambasse
