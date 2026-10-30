// =====================================================================
//  UiHelpers.cpp
// =====================================================================
#include "UiHelpers.h"

#include <QDate>
#include <QLayoutItem>
#include <QWidget>

#include "Theme.h"
#include "core/Money.h"

QString currentMonth()
{
    return QDate::currentDate().toString("yyyy-MM");
}

QString monthTitle(const QString &month)
{
    QDate firstDay = QDate::fromString(month + "-01", "yyyy-MM-dd");
    if (!firstDay.isValid()) {
        return month;
    }
    return firstDay.toString("MMMM yyyy");
}

QString shortMonthName(const QString &month)
{
    QDate firstDay = QDate::fromString(month + "-01", "yyyy-MM-dd");
    if (!firstDay.isValid()) {
        return month;
    }
    return firstDay.toString("MMM");
}

QString countText(int count, const QString &singular, const QString &plural)
{
    if (count == 1) {
        return "1 " + singular;
    }
    return QString::number(count) + " " + plural;
}

QString moneyText(const std::string &currencyCode, long long amountMinor)
{
    return QString::fromStdString(formatMoney(currencyCode, amountMinor));
}

QLabel *createChip(const QString &text, const QString &tone, QWidget *parent)
{
    QLabel *chip = new QLabel(text, parent);
    chip->setObjectName("statusChip");
    setTone(chip, tone);
    return chip;
}

void clearLayout(QLayout *layout)
{
    // takeAt(0) removes the first item until the layout is empty.
    // Widgets are deleted with deleteLater(), which waits until Qt has
    // finished handling the current event, so a button may safely clear
    // the list it belongs to.
    QLayoutItem *item = layout->takeAt(0);
    while (item != nullptr) {
        if (item->widget() != nullptr) {
            item->widget()->hide();
            item->widget()->deleteLater();
        }
        if (item->layout() != nullptr) {
            clearLayout(item->layout());   // a layout inside the layout: empty it too
        }
        delete item;
        item = layout->takeAt(0);
    }
}
