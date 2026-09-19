#include <logic/SexLabel.h>

#include <QCoreApplication>

namespace gambasse {

QString sexLabel(Patient::Sex sex) {
    return sex == Patient::Sex::Muller
               ? QCoreApplication::translate("Sex", "Female")
               : QCoreApplication::translate("Sex", "Male");
}

} // namespace gambasse
