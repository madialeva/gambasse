#pragma once

#include "UxWidgets/UxField.h"

/**
 * Free-text field. Maximum length is controlled through the QLineEdit
 * maxLength property. It can optionally force all typed and pasted text to
 * uppercase.
 */
class UxTextField : public UxField
{
    Q_OBJECT
    Q_PROPERTY(bool uppercase READ uppercase WRITE setUppercase)

public:
    explicit UxTextField(QWidget *parent = nullptr);

    bool uppercase() const { return m_uppercase; }
    void setUppercase(bool uppercase);

private:
    void applyValidator();

    bool m_uppercase = false;
};
