#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace gambasse {

// Domain lists of the pediatric consultation combos (the VB.NET
// LISTA_TIPOS_* dictionaries). The persisted value is the code, so the
// language never changes the data; the label is translated to the active
// language (English source, catalogs by domain context).
struct DomainOption {
    int code = 0;
    QString label;
};

// Keys of the 28 domains ("nursingDiagnosis", "severity", "activeIngredient"...).
QStringList pediatricDomains();
// Options of a domain, in the VB.NET order, starting with the empty "-" (0).
// Unknown keys give an empty list.
QList<DomainOption> pediatricDomainOptions(const QString& domain);
// Codes of a domain. Not always contiguous: the active ingredients jump from
// 149 to 1000-1002.
QList<int> pediatricDomainCodes(const QString& domain);

} // namespace gambasse
