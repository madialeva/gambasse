#pragma once

#include <ui/controller/HistoryController.h>

#include <data/model/PregnancyHistory.h>
#include <logic/PregnancyHistoryService.h>

namespace gambasse {

// Bridge of the pregnancy history screen (Widgets PregnancyHistoryWindow
// ported to QML). The three current-treatment rows that the VB.NET form shows
// as fixed values (iron, folic acid, deworming) are not persisted and therefore
// are not fields here: the form shows them as read-only text.
class PregnancyHistoryController : public HistoryController {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PatientController when a history screen opens.")
public:
    explicit PregnancyHistoryController(QObject* parent = nullptr);

protected:
    const QList<Field>& fields() const override;
    bool serviceLoad() override;
    int serviceSave() override;
    bool serviceRemove() override;

private:
    void buildFields();

    PregnancyHistory m_history;
    PregnancyHistoryService m_service;
};

} // namespace gambasse
