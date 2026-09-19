#pragma once

#include <QCheckBox>

/**
 * Custom check box, so the screens do not mix standard Qt check boxes with
 * the custom controls.
 *
 * On top of QCheckBox it adds:
 *  - a focus highlight (background tint while the control has keyboard focus);
 *  - a distinct text color while checked;
 *  - a read-only mode: user toggles are ignored while the text keeps its
 *    normal (non-disabled) look, for view-only contexts.
 * Both colors adapt to the light/dark application themes.
 */
class UxCheck : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)

public:
    explicit UxCheck(QWidget *parent = nullptr);
    explicit UxCheck(const QString &text, QWidget *parent = nullptr);

    bool isReadOnly() const { return m_readOnly; }
    void setReadOnly(bool readOnly);

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void updateStyle();

    bool m_readOnly = false;
};
