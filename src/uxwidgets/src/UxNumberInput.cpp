#include <UxWidgets/UxNumberInput.h>
#include <UxWidgets/UxNumberField.h>

UxNumberInput::UxNumberInput(const QString &label, QWidget *parent)
    : UxInput(new UxNumberField, parent)
{
    setLabelText(label);
}

UxNumberField *UxNumberInput::numberField() const
{
    return static_cast<UxNumberField *>(field());
}
