#pragma once

#include <QColor>
#include <QLineEdit>
#include <QIcon>

class QAction;

/**
 * Base field layer: a QLineEdit specialized with the behavior shared by all
 * variants (text, number, date):
 *  - A trailing integrated action button (addAction) with a configurable icon
 *    (ellipsis by default) that emits selectionRequested().
 *  - A "required" property with visual feedback when the field is empty.
 *  - Background highlighting while the field has focus, so the active control is
 *    easy to identify. Its color adapts to the theme and updates dynamically.
 *
 * Pasted text needs no custom extension point: QLineEdit already applies its
 * validator and maxLength to pasted text, so each variant filters paste input
 * through its validator.
 */
class UxField : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(bool required READ required WRITE setRequired)
    Q_PROPERTY(bool showSelector READ showSelector WRITE setShowSelector)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
    Q_PROPERTY(QColor lightFocusColor READ lightFocusColor WRITE setLightFocusColor)
    Q_PROPERTY(QColor darkFocusColor READ darkFocusColor WRITE setDarkFocusColor)

public:
    explicit UxField(QWidget *parent = nullptr);
    ~UxField() override;

    bool required() const { return m_required; }
    void setRequired(bool required);

    bool showSelector() const { return m_showSelector; }
    void setShowSelector(bool show);

    QIcon icon() const { return m_icon; }
    void setIcon(const QIcon &icon);

    /// Background color when focused in a light theme.
    QColor lightFocusColor() const { return m_lightFocusColor; }
    void setLightFocusColor(const QColor &color);

    /// Background color when focused in a dark theme.
    QColor darkFocusColor() const { return m_darkFocusColor; }
    void setDarkFocusColor(const QColor &color);

signals:
    /// Requests opening a selector, usually a modal selection dialog.
    void selectionRequested();

protected:
    /// Extension point for activation of the field's integrated button.
    virtual void onActionTriggered();

    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;

    QAction *trailingAction() const { return m_action; }
    void ensureAction();

private:
    void updateBackground();
    bool isDarkTheme() const;

    bool m_required = false;
    bool m_showSelector = false;
    bool m_updating = false;
    QIcon m_icon;
    QColor m_lightFocusColor{0xCC, 0xE8, 0xFF};
    QColor m_darkFocusColor{0x14, 0x37, 0x5A};
    QAction *m_action = nullptr;
};
