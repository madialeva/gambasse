#include <logic/InterfaceLocale.h>

namespace gambasse {

QLocale interfaceLocale(const QString& language) {
    if (language == QLatin1String("pt"))
        return QLocale(QLocale::Portuguese, QLocale::GuineaBissau);
    if (language == QLatin1String("es"))
        return QLocale(QLocale::Spanish, QLocale::Spain);
    // The interface shows any other code in English (no catalog, English flag).
    return QLocale(QLocale::English, QLocale::UnitedKingdom);
}

} // namespace gambasse
