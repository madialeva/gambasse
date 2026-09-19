#include <UxWidgets/UxInput.h>
#include <UxWidgets/UxField.h>

#include <QBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

UxInput::UxInput(UxField *field, QWidget *parent)
    : QWidget(parent)
    , m_label(new QLabel(this))
    , m_field(field)
{
    m_field->setParent(this);
    connect(m_field, &UxField::selectionRequested,
            this, &UxInput::selectionRequested);
    rebuildLayout();
}

QString UxInput::labelText() const
{
    return m_label->text();
}

void UxInput::setLabelText(const QString &text)
{
    m_label->setText(text);
}

void UxInput::setLabelPosition(LabelPosition position)
{
    if (m_labelPosition == position)
        return;
    m_labelPosition = position;
    rebuildLayout();
}

QString UxInput::text() const
{
    return m_field->text();
}

void UxInput::setText(const QString &text)
{
    m_field->setText(text);
}

int UxInput::maxLength() const
{
    return m_field->maxLength();
}

void UxInput::setMaxLength(int length)
{
    m_field->setMaxLength(length);
}

bool UxInput::required() const
{
    return m_field->required();
}

void UxInput::setRequired(bool required)
{
    m_field->setRequired(required);
}

void UxInput::rebuildLayout()
{
    if (QLayout *previousLayout = layout()) {
        previousLayout->removeWidget(m_label);
        previousLayout->removeWidget(m_field);
        delete previousLayout;
    }

    QBoxLayout *layout = (m_labelPosition == Above)
        ? static_cast<QBoxLayout *>(new QVBoxLayout)
        : static_cast<QBoxLayout *>(new QHBoxLayout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_label);
    layout->addWidget(m_field);
    setLayout(layout);
}
