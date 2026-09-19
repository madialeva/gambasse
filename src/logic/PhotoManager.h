#pragma once

#include <QString>

namespace gambasse {

class Patient;

// Resolves and maintains the patient photo files on disk. It only computes
// paths and performs file operations; loading the image is the UI's job.
class PhotoManager {
public:
    QString photoPath(const Patient& patient) const;
    QString legacyPhotoPath(const Patient& patient) const;
    QString defaultPhotoResource() const;
    void renameFor(const Patient& previous, const Patient& current) const;
    void removeFor(const Patient& patient) const;
};

} // namespace gambasse
