// Consultation persistence: the generic SQL shared by the three consultation
// tables and the column map of each one (column names from the English schema,
// member meaning and scaling from the VB.NET ORM, ORM/Consultas/*.vb).

#include <data/common/ConsultationTable.h>
#include <data/common/Database.h>

#include <QSqlQuery>
#include <QStringList>
#include <algorithm>

namespace gambasse {

namespace {

QString keyOf(const Consultation& c) {
    return c.isStored() ? c.originalKey : Database::timestampText(c.date);
}

} // namespace

QString Database::timestampText(const QDateTime& timestamp) {
    return timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

QDateTime Database::parseTimestamp(const QString& text) {
    QString iso = text.trimmed();
    iso.replace(QLatin1Char(' '), QLatin1Char('T'));
    QDateTime timestamp = QDateTime::fromString(iso, Qt::ISODateWithMs);
    if (!timestamp.isValid())
        timestamp = QDateTime::fromString(iso, Qt::ISODate);
    if (!timestamp.isValid()) {
        const QDate date = QDate::fromString(text.trimmed().left(10), Qt::ISODate);
        if (date.isValid())
            timestamp = QDateTime(date, QTime(0, 0));
    }
    return timestamp;
}

template <typename M>
bool Database::listConsultations(qlonglong patientId, QList<M>& consultations) const {
    const ConsultationTable<M>& t = consultationTable<M>();
    QStringList names{t.dateColumn};
    for (const auto& column : t.columns)
        names.append(column.name);
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT %1 FROM %2 WHERE patient_id = ?")
                  .arg(names.join(QStringLiteral(", ")), t.table));
    q.addBindValue(patientId);
    if (!q.exec())
        return false;
    consultations.clear();
    while (q.next()) {
        M c;
        c.patientId = patientId;
        c.originalKey = q.value(0).toString();
        c.date = parseTimestamp(c.originalKey);
        for (int i = 0; i < t.columns.size(); ++i)
            t.columns.at(i).write(c, q.value(i + 1));
        consultations.append(c);
    }
    // Sorted here rather than in SQL: legacy keys may mix timestamp formats,
    // which do not sort as text.
    std::stable_sort(consultations.begin(), consultations.end(),
                     [](const M& a, const M& b) { return a.date > b.date; });
    return true;
}

template <typename M>
bool Database::hasConsultation(qlonglong patientId, const QString& key) const {
    const ConsultationTable<M>& t = consultationTable<M>();
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT 1 FROM %1 WHERE patient_id = ? AND %2 = ? LIMIT 1")
                  .arg(t.table, t.dateColumn));
    q.addBindValue(patientId);
    q.addBindValue(key);
    return q.exec() && q.next();
}

template <typename M>
bool Database::insertConsultation(M& consultation) {
    const ConsultationTable<M>& t = consultationTable<M>();
    QStringList names{QStringLiteral("patient_id"), t.dateColumn};
    for (const auto& column : t.columns)
        names.append(column.name);
    QStringList marks;
    for (int i = 0; i < names.size(); ++i)
        marks.append(QStringLiteral("?"));
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
                  .arg(t.table, names.join(QStringLiteral(", ")), marks.join(QLatin1Char(','))));
    const QString key = timestampText(consultation.date);
    q.addBindValue(consultation.patientId);
    q.addBindValue(key);
    for (const auto& column : t.columns)
        q.addBindValue(column.read(consultation));
    if (!q.exec())
        return false;
    consultation.originalKey = key;
    return true;
}

template <typename M>
bool Database::updateConsultation(const M& consultation) {
    const ConsultationTable<M>& t = consultationTable<M>();
    QStringList sets;
    for (const auto& column : t.columns)
        sets.append(column.name + QStringLiteral(" = ?"));
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE %1 SET %2 WHERE patient_id = ? AND %3 = ?")
                  .arg(t.table, sets.join(QStringLiteral(", ")), t.dateColumn));
    for (const auto& column : t.columns)
        q.addBindValue(column.read(consultation));
    q.addBindValue(consultation.patientId);
    q.addBindValue(keyOf(consultation));
    return q.exec() && q.numRowsAffected() == 1;
}

template <typename M>
bool Database::removeConsultation(const M& consultation) {
    const ConsultationTable<M>& t = consultationTable<M>();
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM %1 WHERE patient_id = ? AND %2 = ?")
                  .arg(t.table, t.dateColumn));
    q.addBindValue(consultation.patientId);
    q.addBindValue(keyOf(consultation));
    return q.exec() && q.numRowsAffected() == 1;
}

// --- Adult consultation (b06) ------------------------------------------------

template <>
const ConsultationTable<AdultConsultation>& consultationTable<AdultConsultation>() {
    using M = AdultConsultation;
    static const ConsultationTable<M> table = [] {
        ConsultationTable<M> t;
        t.table = QStringLiteral("b06_adult_consultation");
        t.dateColumn = QStringLiteral("date");
        t.text(QStringLiteral("reason"), [](M& m) -> QString& { return m.reason; });
        t.boolean(QStringLiteral("physical_exam_nasal_flaring"),
                  [](M& m) -> bool& { return m.nasalFlaring; });
        t.boolean(QStringLiteral("physical_exam_stridor"), [](M& m) -> bool& { return m.stridor; });
        t.boolean(QStringLiteral("physical_exam_wheezing"), [](M& m) -> bool& { return m.wheezing; });
        t.boolean(QStringLiteral("physical_exam_retraction"),
                  [](M& m) -> bool& { return m.retraction; });
        t.boolean(QStringLiteral("physical_exam_arrhythmia"),
                  [](M& m) -> bool& { return m.arrhythmia; });
        t.boolean(QStringLiteral("physical_exam_capillary_refill"),
                  [](M& m) -> bool& { return m.capillaryRefill; });
        t.boolean(QStringLiteral("physical_exam_absent_pulse"),
                  [](M& m) -> bool& { return m.absentPulse; });
        t.boolean(QStringLiteral("physical_exam_edema"), [](M& m) -> bool& { return m.edema; });
        t.boolean(QStringLiteral("physical_exam_skin_folds"),
                  [](M& m) -> bool& { return m.skinFolds; });
        t.boolean(QStringLiteral("physical_exam_pale_mucosa"),
                  [](M& m) -> bool& { return m.paleMucosa; });
        t.boolean(QStringLiteral("physical_exam_sunken_eyes"),
                  [](M& m) -> bool& { return m.sunkenEyes; });
        t.boolean(QStringLiteral("physical_exam_pale_palms"),
                  [](M& m) -> bool& { return m.palePalms; });
        t.boolean(QStringLiteral("physical_exam_organomegaly"),
                  [](M& m) -> bool& { return m.organomegaly; });
        t.text(QStringLiteral("physical_exam_organomegaly_description"),
               [](M& m) -> QString& { return m.organomegalyDescription; });
        t.boolean(QStringLiteral("physical_exam_lymphadenopathy"),
                  [](M& m) -> bool& { return m.lymphadenopathy; });
        t.text(QStringLiteral("physical_exam_lymphadenopathy_description"),
               [](M& m) -> QString& { return m.lymphadenopathyDescription; });
        t.boolean(QStringLiteral("physical_exam_secretion"),
                  [](M& m) -> bool& { return m.secretion; });
        t.boolean(QStringLiteral("physical_exam_stool_changes"),
                  [](M& m) -> bool& { return m.stoolChanges; });
        t.boolean(QStringLiteral("physical_exam_cough"), [](M& m) -> bool& { return m.cough; });
        t.text(QStringLiteral("physical_exam_cough_description"),
               [](M& m) -> QString& { return m.coughDescription; });
        t.boolean(QStringLiteral("physical_exam_urine_changes"),
                  [](M& m) -> bool& { return m.urineChanges; });
        t.text(QStringLiteral("physical_exam_urine_changes_description"),
               [](M& m) -> QString& { return m.urineChangesDescription; });
        t.boolean(QStringLiteral("physical_exam_vomiting"), [](M& m) -> bool& { return m.vomiting; });
        t.text(QStringLiteral("physical_exam_dehydration_degree"),
               [](M& m) -> QString& { return m.dehydrationDegree; });
        t.text(QStringLiteral("physical_exam_malnutrition_degree"),
               [](M& m) -> QString& { return m.malnutritionDegree; });
        t.text(QStringLiteral("physical_exam_other"), [](M& m) -> QString& { return m.other; });
        t.boolean(QStringLiteral("physical_exam_irritable"),
                  [](M& m) -> bool& { return m.irritable; });
        t.text(QStringLiteral("physical_exam_other_code"),
               [](M& m) -> QString& { return m.otherCode; });
        for (int i = 0; i < 8; ++i) {
            t.integer(QStringLiteral("lesion_region%1").arg(i + 1),
                      [i](M& m) -> int& { return m.lesionRegion[i]; });
            t.text(QStringLiteral("lesion_impairment%1").arg(i + 1),
                   [i](M& m) -> QString& { return m.lesionImpairment[i]; });
        }
        return t;
    }();
    return table;
}

// --- Pregnancy consultation (b07) --------------------------------------------

template <>
const ConsultationTable<PregnancyConsultation>& consultationTable<PregnancyConsultation>() {
    using M = PregnancyConsultation;
    static const ConsultationTable<M> table = [] {
        ConsultationTable<M> t;
        t.table = QStringLiteral("b07_pregnancy_consultation");
        t.dateColumn = QStringLiteral("date");
        t.text(QStringLiteral("reason"), [](M& m) -> QString& { return m.reason; });
        t.scaled(QStringLiteral("physical_exam_weight"), [](M& m) -> double& { return m.weight; },
                 10);
        t.integer(QStringLiteral("physical_exam_height"), [](M& m) -> int& { return m.height; });
        t.scaled(QStringLiteral("physical_exam_temperature"),
                 [](M& m) -> double& { return m.temperature; }, 10);
        t.integer(QStringLiteral("physical_exam_abdominal_perimeter"),
                  [](M& m) -> int& { return m.abdominalPerimeter; });
        t.text(QStringLiteral("physical_exam_vomiting"), [](M& m) -> QString& { return m.vomiting; });
        t.text(QStringLiteral("physical_exam_fetal_position"),
               [](M& m) -> QString& { return m.fetalPosition; });
        t.text(QStringLiteral("physical_exam_fetal_auscultation"),
               [](M& m) -> QString& { return m.fetalAuscultation; });
        t.integer(QStringLiteral("physical_exam_uterine_height_weeks"),
                  [](M& m) -> int& { return m.uterineHeightWeeks; });
        t.integer(QStringLiteral("physical_exam_uterine_height_cm"),
                  [](M& m) -> int& { return m.uterineHeightCm; });
        t.boolean(QStringLiteral("physical_exam_leukocytosis"),
                  [](M& m) -> bool& { return m.leukocytosis; });
        t.boolean(QStringLiteral("physical_exam_proteinuria"),
                  [](M& m) -> bool& { return m.proteinuria; });
        t.boolean(QStringLiteral("physical_exam_mmi"), [](M& m) -> bool& { return m.edemaLegs; });
        t.boolean(QStringLiteral("physical_exam_face"), [](M& m) -> bool& { return m.edemaFace; });
        t.boolean(QStringLiteral("physical_exam_general"),
                  [](M& m) -> bool& { return m.edemaGeneral; });
        const auto supplement = [&t](const QString& prefix,
                                     PregnancyConsultation::Supplement M::*member) {
            t.text(prefix + QStringLiteral("_d"),
                   [member](M& m) -> QString& { return (m.*member).dose; });
            t.text(prefix + QStringLiteral("_m"),
                   [member](M& m) -> QString& { return (m.*member).medication; });
            t.text(prefix + QStringLiteral("_pd"),
                   [member](M& m) -> QString& { return (m.*member).nextDose; });
        };
        supplement(QStringLiteral("supplement_calcium"), &M::calcium);
        supplement(QStringLiteral("supplement_vitamin_c"), &M::vitaminC);
        supplement(QStringLiteral("supplement_other_vitamins"), &M::otherVitamins);
        supplement(QStringLiteral("supplement_malaria_prophylaxis"), &M::malariaProphylaxis);
        return t;
    }();
    return table;
}

// --- Pediatric consultation (b08) --------------------------------------------

template <>
const ConsultationTable<PediatricConsultation>& consultationTable<PediatricConsultation>() {
    using M = PediatricConsultation;
    static const ConsultationTable<M> table = [] {
        ConsultationTable<M> t;
        t.table = QStringLiteral("b08_pediatric_consultation");
        t.dateColumn = QStringLiteral("consultation_date");
        const auto text = [&t](const char* column, QString M::*member) {
            t.text(QLatin1String(column), [member](M& m) -> QString& { return m.*member; });
        };
        const auto integer = [&t](const char* column, int M::*member) {
            t.integer(QLatin1String(column), [member](M& m) -> int& { return m.*member; });
        };
        const auto boolean = [&t](const char* column, bool M::*member) {
            t.boolean(QLatin1String(column), [member](M& m) -> bool& { return m.*member; });
        };

        text("reason", &M::reason);
        integer("nursing_diagnosis", &M::nursingDiagnosis);
        text("other_diagnosis", &M::otherDiagnosis);

        // Clinical signs.
        t.scaled(QStringLiteral("physical_exam_weight"), [](M& m) -> double& { return m.weight; },
                 100);
        integer("physical_exam_length", &M::length);
        integer("physical_exam_weight_ratio", &M::weightRatio);
        integer("physical_exam_height_ratio", &M::heightRatio);
        integer("physical_exam_consciousness_combo", &M::consciousness);
        t.scaled(QStringLiteral("physical_exam_temperature"),
                 [](M& m) -> double& { return m.temperature; }, 10);
        integer("physical_exam_blood_glucose", &M::bloodGlucose);
        integer("physical_exam_mental_status", &M::mentalStatus);
        integer("physical_exam_arm_color", &M::armColor);
        integer("physical_exam_appetite_test", &M::appetiteTest);
        integer("physical_exam_malaria_test", &M::malariaTest);
        boolean("physical_exam_convulsion", &M::convulsion);
        boolean("physical_exam_bradypnea", &M::bradypnea);
        boolean("physical_exam_drinking_eating", &M::drinkingEating);

        // Respiratory. The cough check had no DataField in the VB.NET form:
        // it is bound to the cough column its ORM already declared.
        integer("physical_exam_respiratory_retraction_type", &M::retractionType);
        integer("physical_exam_respiratory_retraction_severity", &M::retractionSeverity);
        integer("physical_exam_respiratory_tachypnea", &M::tachypnea);
        boolean("physical_exam_respiratory_burning", &M::burning);
        boolean("physical_exam_respiratory_grunting", &M::grunting);
        boolean("physical_exam_respiratory_runny_nose", &M::runnyNose);
        integer("physical_exam_respiratory_wheeze", &M::wheeze);
        integer("physical_exam_respiratory_secretions", &M::secretions);
        integer("physical_exam_respiratory_stridor", &M::stridor);
        boolean("physical_exam_respiratory_congestion", &M::congestion);
        boolean("physical_exam_respiratory_tearing", &M::tearing);
        boolean("physical_exam_respiratory_sore_throat", &M::soreThroat);
        boolean("physical_exam_cough", &M::cough);
        integer("physical_exam_respiratory_duration", &M::coughDays);
        integer("physical_exam_respiratory_oxygen_saturation", &M::oxygenSaturation);

        // Elimination.
        integer("physical_exam_elimination_stool", &M::stool);
        text("physical_exam_elimination_bleeding", &M::bleeding);
        integer("physical_exam_elimination_urine", &M::urine);
        integer("physical_exam_elimination_vomiting", &M::vomiting);

        // Circulatory.
        integer("physical_exam_circulatory_capillary_refill", &M::capillaryRefill);
        boolean("physical_exam_absent_pulse", &M::absentPulse);
        integer("physical_exam_circulatory_cyanosis", &M::cyanosis);
        text("physical_exam_circulatory_edema", &M::edema);
        integer("physical_exam_circulatory_heart_rate", &M::heartRate);
        text("physical_exam_circulatory_arrhythmia", &M::arrhythmia);

        // Skin.
        boolean("physical_exam_skin_folds", &M::skinFolds);
        boolean("physical_exam_pale_palms", &M::palePalms);
        boolean("physical_exam_skin_dry_skin", &M::drySkin);
        text("physical_exam_skin_body_lesions", &M::bodyLesions);
        boolean("physical_exam_skin_petechiae", &M::petechiae);
        boolean("physical_exam_skin_pustules", &M::pustules);
        boolean("physical_exam_skin_jaundice", &M::jaundice);
        integer("physical_exam_skin_exanthema", &M::exanthema);
        integer("physical_exam_skin_infection", &M::infection);
        integer("physical_exam_skin_edema", &M::skinEdema);

        // Eye.
        integer("physical_exam_eye_discharge", &M::eyeDischarge);
        boolean("physical_exam_eye_eyelid_edema", &M::eyelidEdema);
        boolean("physical_exam_eye_stuck_eyelids", &M::stuckEyelids);
        boolean("physical_exam_eye_red_conjunctiva", &M::redConjunctiva);
        boolean("physical_exam_eye_pale_conjunctiva", &M::paleConjunctiva);
        boolean("physical_exam_sunken_eyes", &M::sunkenEyes);

        // Ear.
        boolean("physical_exam_ear_swelling_behind_ear", &M::swellingBehindEar);
        integer("physical_exam_ear_pain", &M::earPain);
        integer("physical_exam_ear_discharge", &M::earDischarge);
        boolean("physical_exam_ear_itchy_ear_canal", &M::itchyEarCanal);
        integer("physical_exam_ear_otoscopy_tympanum", &M::otoscopy);

        // Mouth.
        integer("physical_exam_mouth_erythematous_tonsils", &M::tonsils);
        integer("physical_exam_mouth_pain", &M::mouthPain);
        text("physical_exam_mouth_lesion_type", &M::lesionType);
        boolean("physical_exam_mouth_koplik_spots", &M::koplikSpots);
        boolean("physical_exam_mouth_white_patches", &M::whitePatches);
        boolean("physical_exam_mouth_red_tongue", &M::redTongue);
        boolean("physical_exam_mouth_palatal_petechiae", &M::palatalPetechiae);
        boolean("physical_exam_mouth_cervical_nodes", &M::cervicalNodes);

        // Abdomen / pelvis.
        integer("physical_exam_abdominal_pain_signs", &M::abdominalPainSigns);

        // Neurological.
        boolean("physical_exam_neuro_severe_headache", &M::severeHeadache);
        boolean("physical_exam_neuro_photophobia", &M::photophobia);
        boolean("physical_exam_neuro_brudzinski_sign", &M::brudzinskiSign);
        boolean("physical_exam_neuro_kernig_sign", &M::kernigSign);
        boolean("physical_exam_neuro_neck_stiffness", &M::neckStiffness);
        boolean("physical_exam_neuro_bulging_fontanelle", &M::bulgingFontanelle);
        boolean("physical_exam_neuro_developmental_problems", &M::developmentalProblems);
        text("physical_exam_neuro_developmental_problems_description",
             &M::developmentalProblemsDescription);

        // Endocrine. The VB.NET ORM stores the lymphadenopathy text of this
        // tab in the generic description column.
        boolean("physical_exam_general_excessive_hunger", &M::excessiveHunger);
        boolean("physical_exam_general_excessive_thirst", &M::excessiveThirst);
        boolean("physical_exam_general_asthenia", &M::asthenia);
        text("physical_exam_lymphadenopathy_description", &M::lymphadenopathy);
        text("physical_exam_general_rapid_weight_loss", &M::rapidWeightLoss);
        text("physical_exam_general_breath_odor", &M::breathOdor);

        // Indicated treatments and recommendations.
        for (int i = 0; i < 10; ++i) {
            const QString n = QString::number(i + 1);
            t.integer(QStringLiteral("treatment_medication") + n,
                      [i](M& m) -> int& { return m.treatments[i].medication; });
            t.text(QStringLiteral("treatment_description") + n,
                   [i](M& m) -> QString& { return m.treatments[i].description; });
            t.integer(QStringLiteral("treatment_route") + n,
                      [i](M& m) -> int& { return m.treatments[i].route; });
            t.integer(QStringLiteral("treatment_days") + n,
                      [i](M& m) -> int& { return m.treatments[i].days; });
            t.text(QStringLiteral("treatment_dose") + n,
                   [i](M& m) -> QString& { return m.treatments[i].dose; });
            t.text(QStringLiteral("treatment_frequency") + n,
                   [i](M& m) -> QString& { return m.treatments[i].frequency; });
            t.integer(QStringLiteral("treatment_perfusion") + n,
                      [i](M& m) -> int& { return m.treatments[i].perfusion; });
        }
        for (int i = 0; i < 5; ++i) {
            t.integer(QStringLiteral("recommendation%1").arg(i + 1),
                      [i](M& m) -> int& { return m.recommendations[i]; });
        }
        return t;
    }();
    return table;
}

// The three consultation models are the only instantiations.
#define GAMBASSE_CONSULTATION_INSTANCES(M)                                                         \
    template bool Database::listConsultations<M>(qlonglong, QList<M>&) const;                      \
    template bool Database::hasConsultation<M>(qlonglong, const QString&) const;                   \
    template bool Database::insertConsultation<M>(M&);                                             \
    template bool Database::updateConsultation<M>(const M&);                                       \
    template bool Database::removeConsultation<M>(const M&);

GAMBASSE_CONSULTATION_INSTANCES(AdultConsultation)
GAMBASSE_CONSULTATION_INSTANCES(PregnancyConsultation)
GAMBASSE_CONSULTATION_INSTANCES(PediatricConsultation)

#undef GAMBASSE_CONSULTATION_INSTANCES

} // namespace gambasse
