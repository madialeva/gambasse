#pragma once

#include <ui/controller/ConsultationController.h>

#include <data/model/PregnancyConsultation.h>

namespace gambasse {

// Bridge of the pregnancy consultation screen (VB.NET FrmConsultaMullerGravida).
// The three current-treatment rows that the form shows as fixed values (iron,
// folic acid, deworming) are not persisted and therefore are not fields here:
// the form shows them as read-only text.
class PregnancyConsultationController : public ConsultationController {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PatientController when a consultation screen opens.")
public:
    explicit PregnancyConsultationController(QObject* parent = nullptr);

private:
    void buildFields();
};

} // namespace gambasse
