#include "UxWidgets/UxDateInput.h"
#include "UxWidgets/UxDateField.h"

UxDateInput::UxDateInput(const QString &label, QWidget *parent)
    : UxInput(new UxDateField, parent)
{
    setLabelText(label);
}

UxDateField *UxDateInput::dateField() const
{
    return static_cast<UxDateField *>(field());
}
