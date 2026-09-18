#include "UxWidgets/UxComboInput.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QVBoxLayout>

UxComboInput::UxComboInput(const QString &label, QWidget *parent)
    : QWidget(parent)
    , m_label(new QLabel(label, this))
    , m_combo(new QComboBox(this))
{
    m_combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rebuildLayout();
}

QString UxComboInput::labelText() const
{
    return m_label->text();
}

void UxComboInput::setLabelText(const QString &text)
{
    m_label->setText(text);
}

void UxComboInput::setLabelPosition(LabelPosition position)
{
    if (m_labelPosition == position)
        return;
    m_labelPosition = position;
    rebuildLayout();
}

void UxComboInput::rebuildLayout()
{
    if (QLayout *previousLayout = layout()) {
        previousLayout->removeWidget(m_label);
        previousLayout->removeWidget(m_combo);
        delete previousLayout;
    }

    QBoxLayout *layout = (m_labelPosition == Above)
        ? static_cast<QBoxLayout *>(new QVBoxLayout)
        : static_cast<QBoxLayout *>(new QHBoxLayout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_label);
    layout->addWidget(m_combo, 1);
    setLayout(layout);
}
