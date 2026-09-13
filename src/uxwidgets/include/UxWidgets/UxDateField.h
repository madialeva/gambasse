#pragma once

#include "UxWidgets/UxField.h"

/**
 * Date field built on QLineEdit (not QDateEdit), with dd/MM/yyyy format. It
 * accepts an empty value without error when it is not required and always shows
 * a calendar icon that opens a QCalendarWidget and writes the selected date.
 */
class UxDateField : public UxField
{
    Q_OBJECT

public:
    /// Date format used by the control.
    static constexpr const char *kDateFormat = "dd/MM/yyyy";

    explicit UxDateField(QWidget *parent = nullptr);

protected:
    void onActionTriggered() override;

private:
    void showCalendar();
};
