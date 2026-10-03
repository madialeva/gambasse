#pragma once

#include <ui/controller/ConsultationController.h>

#include <data/model/PediatricConsultation.h>

namespace gambasse {

// Bridge of the pediatric consultation screen (VB.NET FrmConsultaPediatrica).
// Besides the field table it publishes the domain lists of the 64 combos,
// translated to the active language; the persisted value is the VB.NET code.
class PediatricConsultationController : public ConsultationController {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PatientController when a consultation screen opens.")
    // { domain: [{ "value": int, "label": string }] } for every domain of
    // PediatricConsultationLabels.
    Q_PROPERTY(QVariantMap domainOptions READ domainOptions NOTIFY domainOptionsChanged)
public:
    explicit PediatricConsultationController(QObject* parent = nullptr);

    QVariantMap domainOptions() const;

    // Rebuilds the translated labels after a language change; the combos keep
    // their selection because it is the code.
    Q_INVOKABLE void refreshLanguage();

signals:
    void domainOptionsChanged();

private:
    void buildFields();

    QVariantMap m_domainOptions;
};

} // namespace gambasse
