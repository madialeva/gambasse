#pragma once

#include <ui/controller/HistoryController.h>

#include <data/model/PediatricHistory.h>
#include <logic/PediatricHistoryService.h>

namespace gambasse {

// Bridge of the pediatric history screen (Widgets PediatricHistoryWindow ported
// to QML). Same contract as the other history controllers: the model, the
// service calls and the field table the form renders.
class PediatricHistoryController : public HistoryController {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PatientController when a history screen opens.")
public:
    explicit PediatricHistoryController(QObject* parent = nullptr);

protected:
    const QList<Field>& fields() const override;
    bool serviceLoad() override;
    int serviceSave() override;
    bool serviceRemove() override;

private:
    void buildFields();

    PediatricHistory m_history;
    PediatricHistoryService m_service;
};

} // namespace gambasse
