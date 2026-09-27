#pragma once

#include <ui/controller/HistoryController.h>

#include <data/model/AdultHistory.h>
#include <logic/AdultHistoryService.h>

namespace gambasse {

// Bridge of the adult history screen (Widgets AdultHistoryWindow ported to
// QML). It owns the AdultHistory model, the AdultHistoryService calls and the
// field table the form renders; every rule (defaults, bounds, date parsing and
// persistence) stays here, in logic/ and data/.
class AdultHistoryController : public HistoryController {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PatientController when a history screen opens.")
public:
    explicit AdultHistoryController(QObject* parent = nullptr);

protected:
    const QList<Field>& fields() const override;
    bool serviceLoad() override;
    int serviceSave() override;
    bool serviceRemove() override;

private:
    void buildFields();

    AdultHistory m_history;
    AdultHistoryService m_service;
};

} // namespace gambasse
