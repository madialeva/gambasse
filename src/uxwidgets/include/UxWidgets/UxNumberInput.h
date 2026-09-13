#pragma once

#include "UxWidgets/UxInput.h"

class UxNumberField;

/// Convenience composite control with a number field.
class UxNumberInput : public UxInput
{
    Q_OBJECT

public:
    explicit UxNumberInput(const QString &label = QString(), QWidget *parent = nullptr);

    UxNumberField *numberField() const;
};
