#pragma once

#include <QDateTime>
#include <QString>

namespace gambasse {

// Fields shared by the three consultation models. A consultation is keyed by
// patient and timestamp (the date column of its table), so a patient has any
// number of them.
struct Consultation {
    qlonglong patientId = 0;
    // When the consultation took place: set on creation and never edited.
    QDateTime date;
    // The timestamp text exactly as stored, empty for a consultation not yet
    // saved. Updates and deletes locate the row by it, so rows written by the
    // legacy application in another timestamp format are still found.
    QString originalKey;
    QString reason;

    bool isStored() const { return !originalKey.isEmpty(); }
};

} // namespace gambasse
