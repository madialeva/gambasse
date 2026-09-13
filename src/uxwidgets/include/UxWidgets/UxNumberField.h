#pragma once

#include "UxWidgets/UxField.h"

/**
 * Numeric field with configurable precision: permitted integer and decimal
 * digits, plus an optional thousands separator. The content is right-aligned.
 * The decimal separator is a comma and the thousands separator is a period.
 */
class UxNumberField : public UxField
{
    Q_OBJECT
    Q_PROPERTY(int integerDigits READ integerDigits WRITE setIntegerDigits)
    Q_PROPERTY(int decimalDigits READ decimalDigits WRITE setDecimalDigits)
    Q_PROPERTY(bool thousandsSeparator READ thousandsSeparator WRITE setThousandsSeparator)

public:
    explicit UxNumberField(QWidget *parent = nullptr);

    int integerDigits() const { return m_integerDigits; }
    void setIntegerDigits(int digits);

    int decimalDigits() const { return m_decimalDigits; }
    void setDecimalDigits(int digits);

    bool thousandsSeparator() const { return m_thousandsSeparator; }
    void setThousandsSeparator(bool separator);

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void updateValidator();
    void formatThousands();
    void removeThousandsSeparator();

    int m_integerDigits = 9;
    int m_decimalDigits = 0;
    bool m_thousandsSeparator = false;
};
