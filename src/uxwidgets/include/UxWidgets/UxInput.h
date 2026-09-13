#pragma once

#include <QWidget>
#include <QString>

class QLabel;
class UxField;

/**
 * Composite control: label (QLabel) + a field-layer control (UxField) as a
 * single unit. The label can be placed to the left or above the field. Relevant
 * properties and the selection signal are forwarded from the internal field.
 */
class UxInput : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText)
    Q_PROPERTY(LabelPosition labelPosition READ labelPosition WRITE setLabelPosition)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength)
    Q_PROPERTY(bool required READ required WRITE setRequired)

public:
    enum LabelPosition { Left, Above };
    Q_ENUM(LabelPosition)

    /// Takes ownership of field by reparenting it into the composite control.
    explicit UxInput(UxField *field, QWidget *parent = nullptr);

    UxField *field() const { return m_field; }

    QString labelText() const;
    void setLabelText(const QString &text);

    LabelPosition labelPosition() const { return m_labelPosition; }
    void setLabelPosition(LabelPosition position);

    QString text() const;
    void setText(const QString &text);
    int maxLength() const;
    void setMaxLength(int length);
    bool required() const;
    void setRequired(bool required);

signals:
    void selectionRequested();

private:
    void rebuildLayout();

    QLabel *m_label = nullptr;
    UxField *m_field = nullptr;
    LabelPosition m_labelPosition = Left;
};
