#include "UxWidgets/UxNumberField.h"

#include <QFocusEvent>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>

UxNumberField::UxNumberField(QWidget *parent)
    : UxField(parent)
{
    setAlignment(Qt::AlignRight);
    updateValidator();
}

void UxNumberField::setIntegerDigits(int digits)
{
    m_integerDigits = qMax(1, digits);
    updateValidator();
}

void UxNumberField::setDecimalDigits(int digits)
{
    m_decimalDigits = qMax(0, digits);
    updateValidator();
}

void UxNumberField::setThousandsSeparator(bool separator)
{
    m_thousandsSeparator = separator;
    if (!separator)
        removeThousandsSeparator();
    else if (!hasFocus())
        formatThousands();
}

void UxNumberField::updateValidator()
{
    QString pattern;
    if (m_decimalDigits > 0)
        pattern = QStringLiteral("^\\d{0,%1}([,]\\d{0,%2})?$")
                     .arg(m_integerDigits)
                     .arg(m_decimalDigits);
    else
        pattern = QStringLiteral("^\\d{0,%1}$").arg(m_integerDigits);

    setValidator(new QRegularExpressionValidator(QRegularExpression(pattern), this));
}

void UxNumberField::removeThousandsSeparator()
{
    QString value = text();
    if (!value.contains(QLatin1Char('.')))
        return;
    value.remove(QLatin1Char('.'));
    QSignalBlocker blocker(this);
    setText(value);
}

void UxNumberField::formatThousands()
{
    if (!m_thousandsSeparator)
        return;

    const QString value = text().trimmed();
    if (value.isEmpty())
        return;

    const int comma = value.indexOf(QLatin1Char(','));
    QString integerPart = (comma >= 0) ? value.left(comma) : value;
    const QString decimalPart = (comma >= 0) ? value.mid(comma + 1) : QString();
    integerPart.remove(QLatin1Char('.'));

    QString grouped;
    int digitCount = 0;
    for (int index = integerPart.size() - 1; index >= 0; --index) {
        grouped.prepend(integerPart.at(index));
        if (++digitCount % 3 == 0 && index > 0)
            grouped.prepend(QLatin1Char('.'));
    }

    QString result = grouped;
    if (comma >= 0)
        result += QLatin1Char(',') + decimalPart;

    QSignalBlocker blocker(this);
    setText(result);
}

void UxNumberField::focusInEvent(QFocusEvent *event)
{
    if (m_thousandsSeparator)
        removeThousandsSeparator();
    UxField::focusInEvent(event);
}

void UxNumberField::focusOutEvent(QFocusEvent *event)
{
    formatThousands();
    UxField::focusOutEvent(event);
}
