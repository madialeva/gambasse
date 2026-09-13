#pragma once

#include "UxWidgets/UxInput.h"

class UxDateField;

/// Convenience composite control with a date field.
class UxDateInput : public UxInput
{
    Q_OBJECT

public:
    explicit UxDateInput(const QString &label = QString(), QWidget *parent = nullptr);

    UxDateField *dateField() const;
};
