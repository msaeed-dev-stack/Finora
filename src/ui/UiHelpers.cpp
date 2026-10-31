// =====================================================================
//  UiHelpers.cpp
// =====================================================================
#include "UiHelpers.h"

#include <QDate>
#include <QDateTime>
#include <QLayoutItem>
#include <QMessageBox>
#include <QPushButton>
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

// ---- Subscriptions (Phase 4C) ---------------------------------------------

std::string currentUtcTime()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
}

QString dateText(const std::string &utcTime)
{
    QDateTime time = QDateTime::fromString(QString::fromStdString(utcTime), Qt::ISODate);
    if (!time.isValid()) {
        return QString();
    }
    return time.toLocalTime().toString("d MMM yyyy");
}

int daysUntil(const std::string &utcTime)
{
    QDateTime end = QDateTime::fromString(QString::fromStdString(utcTime), Qt::ISODate);
    if (!end.isValid()) {
        return 0;
    }
    long long seconds = QDateTime::currentDateTimeUtc().secsTo(end);
    if (seconds <= 0) {
        return 0;
    }
    // A part of a day counts as a whole day: 13.5 days left shows as 14.
    return static_cast<int>((seconds + 86399) / 86400);
}

QString shortPlanName(PlanId plan)
{
    QString name = QString::fromStdString(planName(plan));   // "Finora Pro"
    return name.mid(QString("Finora ").length());           // "Pro"
}

QString planChipText(const Subscription &subscription)
{
    QString name = shortPlanName(subscription.plan);
    switch (subscription.status) {
    case STATUS_TRIAL:
        return name + " trial";
    case STATUS_ACTIVE:
        return "Finora " + name;
    case STATUS_PAST_DUE:
        return QString::fromUtf8("%1 · payment due").arg(name);
    case STATUS_CANCELLED:
        return QString::fromUtf8("%1 · ends %2").arg(name, dateText(subscription.expiresAt));
    case STATUS_FREE:
    case STATUS_EXPIRED:
        break;
    }
    return "Finora Free";
}

QString planChipTone(const Subscription &subscription)
{
    if (subscription.status == STATUS_PAST_DUE) {
        return "warning";
    }
    if (subscription.status == STATUS_TRIAL || subscription.status == STATUS_ACTIVE) {
        return "info";
    }
    return "neutral";
}

bool askToSeePlans(QWidget *parent, const QString &title, const QString &explanation)
{
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(title);
    box.setText(title);
    box.setInformativeText(explanation);
    QPushButton *seePlans = box.addButton("See plans", QMessageBox::AcceptRole);
    QPushButton *notNow = box.addButton("Not now", QMessageBox::RejectRole);
    box.setDefaultButton(notNow);
    box.exec();
    return box.clickedButton() == seePlans;
}

void showLockIfNeeded(QPushButton *button, bool locked, const QString &planNeeded)
{
    if (locked) {
        button->setIcon(QIcon(tintedIcon("lock", themeColors().textMuted, 16)));
        button->setIconSize(QSize(16, 16));
        button->setToolTip("Part of " + planNeeded + ". Click to see the plans.");
    } else {
        button->setIcon(QIcon());
        button->setToolTip(QString());
    }
}
