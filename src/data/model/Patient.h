#pragma once

#include <QString>
#include <QDate>
#include <QDateTime>
#include <QTime>

namespace gambasse {

// Patient domain model (table b01_patient).
// Mirrors the fields used by the main window.
struct Patient {

    enum class Sex { Home = 0, Muller = 1 };

    qlonglong id = 0;
    QString   name;
    QDate     birthDate;
    Sex      sex = Sex::Home;
    int       ageRange = 0;        // approximate_age
    QString   address;
    QString   cohabitants;
    QString   contactPerson;
    int       siblingCount = 0;

    // Whether the birth date is real (the database default is 1900-01-01).
    bool isValidDate() const {
        return birthDate.isValid() && birthDate.year() > 1900;
    }

    // Age in years from the birth date (0 if it is not valid).
    int ageInYears() const {
        if (!isValidDate())
            return 0;
        const QDate today = QDate::currentDate();
        int age = today.year() - birthDate.year();
        if (birthDate.addYears(age) > today)
            --age;
        return age;
    }

    QString sexToString() const {
        return sex == Sex::Muller ? QStringLiteral("Muller") : QStringLiteral("Home");
    }

    // Patient photo filename in the persisted fotos/ directory.
    // The directory and filename format are a compatibility contract.
    QString photoFilename() const {
        QString normalizedName = name;
        normalizedName.replace(QLatin1Char(' '), QLatin1Char('_'));
        const QString timestamp =
            QDateTime(birthDate, QTime(0, 0, 0)).toString(QStringLiteral("yyyyMMddHHmmss"));
        const QString sexValue =
            sex == Sex::Muller ? QStringLiteral("MULLER") : QStringLiteral("HOME");
        return QStringLiteral("fotos/") + timestamp + QLatin1Char('-') + normalizedName + QLatin1Char('-')
               + QString::number(ageRange) + QLatin1Char('-') + sexValue + QStringLiteral(".jpg");
    }
};

} // namespace gambasse
