#pragma once

#include <UxWidgets/UxInput.h>

class UxTextField;

/// Convenience composite control with a text field.
class UxTextInput : public UxInput
{
    Q_OBJECT

public:
    explicit UxTextInput(const QString &label = QString(), QWidget *parent = nullptr);

    UxTextField *textField() const;
};
