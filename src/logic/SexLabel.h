#pragma once

#include <QString>

#include <data/model/Patient.h>

namespace gambasse {

// Display label for a patient sex in the active interface language (English
// source strings "Male"/"Female"). The persisted value is the enum, never text.
QString sexLabel(Patient::Sex sex);

} // namespace gambasse
