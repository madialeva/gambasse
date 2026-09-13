#include "UxWidgets/UxTextField.h"

#include <QValidator>

namespace {

// This validator converts typed and pasted text to uppercase without moving the
// cursor. QValidator::validate() is allowed to modify the input text.
class UppercaseValidator : public QValidator
{
public:
    using QValidator::QValidator;

    State validate(QString &input, int & /*pos*/) const override
    {
        input = input.toUpper();
        return Acceptable;
    }
};

} // namespace

UxTextField::UxTextField(QWidget *parent)
    : UxField(parent)
{
}

void UxTextField::setUppercase(bool uppercase)
{
    m_uppercase = uppercase;
    applyValidator();
}

void UxTextField::applyValidator()
{
    if (m_uppercase) {
        setValidator(new UppercaseValidator(this));
        setText(text().toUpper());
    } else {
        setValidator(nullptr);
    }
}
