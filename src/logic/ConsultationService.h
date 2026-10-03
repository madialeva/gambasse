#pragma once

#include <QDateTime>
#include <QList>

#include <data/common/Database.h>
#include <data/model/AdultConsultation.h>
#include <data/model/PediatricConsultation.h>
#include <data/model/PregnancyConsultation.h>

namespace gambasse {

// Consultation list/save coordination for one consultation model. The window
// only gathers/renders values and shows messages; all database access goes
// through here.
template <typename M>
class ConsultationService {
public:
    enum class SaveResult { Saved, Error };

    // Consultations of the patient, the most recent first. Returns false on
    // database error.
    bool list(qlonglong patientId, QList<M>& consultations) const {
        return Database::instance().listConsultations<M>(patientId, consultations);
    }

    // A new, unsaved consultation of the patient dated now (whole seconds:
    // the key keeps no fraction).
    M newConsultation(qlonglong patientId) const {
        M consultation;
        consultation.patientId = patientId;
        QDateTime now = QDateTime::currentDateTime();
        now.setTime(QTime(now.time().hour(), now.time().minute(), now.time().second()));
        consultation.date = now;
        return consultation;
    }

    // Inserts a new consultation (moving its timestamp a second later while
    // the key is taken, so two saves within the same second do not collide)
    // or updates a stored one, whose timestamp never changes.
    SaveResult save(M& consultation) const {
        Database& db = Database::instance();
        if (consultation.isStored())
            return db.updateConsultation<M>(consultation) ? SaveResult::Saved : SaveResult::Error;
        while (db.hasConsultation<M>(consultation.patientId,
                                     Database::timestampText(consultation.date)))
            consultation.date = consultation.date.addSecs(1);
        return db.insertConsultation<M>(consultation) ? SaveResult::Saved : SaveResult::Error;
    }

    bool remove(const M& consultation) const {
        return consultation.isStored() && Database::instance().removeConsultation<M>(consultation);
    }
};

using AdultConsultationService = ConsultationService<AdultConsultation>;
using PregnancyConsultationService = ConsultationService<PregnancyConsultation>;
using PediatricConsultationService = ConsultationService<PediatricConsultation>;

} // namespace gambasse
