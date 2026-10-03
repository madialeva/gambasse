#pragma once

#include <ui/controller/ConsultationController.h>

#include <data/model/AdultConsultation.h>

namespace gambasse {

// Bridge of the adult consultation screen (VB.NET FrmConsultaAdulto). It only
// declares the field table over the edited consultation; the list, the service
// calls and the rules live in ConsultationController, logic/ and data/.
class AdultConsultationController : public ConsultationController {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PatientController when a consultation screen opens.")
public:
    explicit AdultConsultationController(QObject* parent = nullptr);

private:
    void buildFields();
};

} // namespace gambasse
