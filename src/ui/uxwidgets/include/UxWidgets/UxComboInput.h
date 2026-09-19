#pragma once

#include <QString>
#include <QWidget>

class QLabel;
class QComboBox;

/**
 * Composite control: label (QLabel) + combo box (QComboBox) as a single unit.
 * The label can be placed to the left or above the combo box, mirroring the
 * UxInput composites used by the field controls. The combo box is exposed
 * through comboBox() so callers add items and read/write the selection.
 */
class UxComboInput : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText)
    Q_PROPERTY(LabelPosition labelPosition READ labelPosition WRITE setLabelPosition)

public:
    enum LabelPosition { Left, Above };
    Q_ENUM(LabelPosition)

    explicit UxComboInput(const QString &label = QString(), QWidget *parent = nullptr);

    QComboBox *comboBox() const { return m_combo; }

    QString labelText() const;
    void setLabelText(const QString &text);

    LabelPosition labelPosition() const { return m_labelPosition; }
    void setLabelPosition(LabelPosition position);

private:
    void rebuildLayout();

    QLabel *m_label = nullptr;
    QComboBox *m_combo = nullptr;
    LabelPosition m_labelPosition = Left;
};
