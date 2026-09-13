#include "UxWidgets/UxTextInput.h"
#include "UxWidgets/UxTextField.h"

UxTextInput::UxTextInput(const QString &label, QWidget *parent)
    : UxInput(new UxTextField, parent)
{
    setLabelText(label);
}

UxTextField *UxTextInput::textField() const
{
    return static_cast<UxTextField *>(field());
}
