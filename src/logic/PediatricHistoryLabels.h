#pragma once

#include <QString>

#include <data/model/PediatricHistory.h>

namespace gambasse {

// Display labels for the pediatric history domain lists (domestic animals and
// treatment types). English is the source language; the text shown in each
// language comes from the catalogs. Persisted values stay integers and never
// depend on the language.
QString domesticAnimalsLabel(PediatricHistory::DomesticAnimals value);
QString treatmentTypeLabel(PediatricHistory::TreatmentType value);

} // namespace gambasse
