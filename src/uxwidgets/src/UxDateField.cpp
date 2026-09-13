#include "UxWidgets/UxDateField.h"

#include <QApplication>
#include <QCalendarWidget>
#include <QColor>
#include <QDate>
#include <QIcon>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPoint>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QIcon calendarIcon()
{
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QColor color = QApplication::palette().color(QPalette::Text);
    QPen pen(color);
    pen.setWidthF(1.2);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const QRect body(2, 3, 12, 11);
    p.drawRoundedRect(body, 1.5, 1.5);
    p.setBrush(color);
    p.drawRect(QRect(2, 3, 12, 3));
    p.setPen(pen);
    p.drawLine(5, 1, 5, 4);
    p.drawLine(11, 1, 11, 4);

    return QIcon(pm);
}

} // namespace

UxDateField::UxDateField(QWidget *parent)
    : UxField(parent)
{
    setPlaceholderText(QStringLiteral("dd/mm/yyyy"));

    const QString pattern =
        QStringLiteral("^(\\d{0,2})(/(\\d{0,2})(/(\\d{0,4})?)?)?$");
    setValidator(new QRegularExpressionValidator(QRegularExpression(pattern), this));

    setIcon(calendarIcon());
    setShowSelector(true);
}

void UxDateField::onActionTriggered()
{
    showCalendar();
}

void UxDateField::showCalendar()
{
    auto *popup = new QWidget(this, Qt::Popup);
    auto *layout = new QVBoxLayout(popup);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *calendar = new QCalendarWidget(popup);
    const QDate current = QDate::fromString(text(), QString::fromLatin1(kDateFormat));
    if (current.isValid())
        calendar->setSelectedDate(current);
    layout->addWidget(calendar);

    connect(calendar, &QCalendarWidget::clicked, this,
            [this, popup](const QDate &date) {
                setText(date.toString(QString::fromLatin1(kDateFormat)));
                popup->close();
                popup->deleteLater();
            });

    popup->move(mapToGlobal(QPoint(0, height())));
    popup->show();
}
