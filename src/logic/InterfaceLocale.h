#pragma once

#include <QLocale>
#include <QString>

namespace gambasse {

// The locale an interface language stands for: Portuguese is pt_GW, Spanish
// es_ES and English, like any unknown code, en_GB. The calendar and the order
// of the patient list take it instead of the system's, so both look the same on
// every machine; the three start the week on Monday.
QLocale interfaceLocale(const QString& language);

} // namespace gambasse
