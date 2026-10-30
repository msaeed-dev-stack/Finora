// =====================================================================
//  MonthSwitcher.cpp
// =====================================================================
#include "MonthSwitcher.h"

#include <QHBoxLayout>

#include "core/Totals.h"
#include "ui/Theme.h"
#include "ui/UiHelpers.h"

MonthSwitcher::MonthSwitcher(QWidget *parent)
    : QWidget(parent), chosenMonth(currentMonth())
{
    previousButton = new QPushButton(this);
    previousButton->setObjectName("iconButton");
    previousButton->setAccessibleName("Previous month");
    previousButton->setToolTip("Previous month");
    previousButton->setCursor(Qt::PointingHandCursor);

    monthLabel = new QLabel(this);
    monthLabel->setObjectName("monthLabel");
    monthLabel->setAlignment(Qt::AlignCenter);

    nextButton = new QPushButton(this);
    nextButton->setObjectName("iconButton");
    nextButton->setAccessibleName("Next month");
    nextButton->setToolTip("Next month");
    nextButton->setCursor(Qt::PointingHandCursor);

    todayButton = new QPushButton("This month", this);
    todayButton->setObjectName("linkButton");
    todayButton->setCursor(Qt::PointingHandCursor);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(todayButton);
    layout->addWidget(previousButton);
    layout->addWidget(monthLabel);
    layout->addWidget(nextButton);

    connect(previousButton, &QPushButton::clicked, this, &MonthSwitcher::showPreviousMonth);
    connect(nextButton, &QPushButton::clicked, this, &MonthSwitcher::showNextMonth);
    connect(todayButton, &QPushButton::clicked, this, &MonthSwitcher::showThisMonth);

    refreshIcons();
    updateLabel();
}

QString MonthSwitcher::month() const
{
    return chosenMonth;
}

void MonthSwitcher::setMonth(const QString &month)
{
    if (!isValidMonth(month.toStdString()) || month == chosenMonth) {
        return;
    }
    chosenMonth = month;
    updateLabel();
    emit monthChanged(chosenMonth);
}

void MonthSwitcher::refreshIcons()
{
    QColor color = themeColors().text;
    previousButton->setIcon(QIcon(tintedIcon("chevron_left", color, 16)));
    nextButton->setIcon(QIcon(tintedIcon("chevron_right", color, 16)));
}

void MonthSwitcher::showPreviousMonth()
{
    moveBy(-1);
}

void MonthSwitcher::showNextMonth()
{
    moveBy(1);
}

void MonthSwitcher::showThisMonth()
{
    setMonth(currentMonth());
}

void MonthSwitcher::moveBy(int months)
{
    // shiftMonth() returns "" outside the years 1900 to 2100; setMonth()
    // ignores that, so the arrows simply stop at the ends.
    setMonth(QString::fromStdString(shiftMonth(chosenMonth.toStdString(), months)));
}

void MonthSwitcher::updateLabel()
{
    monthLabel->setText(monthTitle(chosenMonth));
    monthLabel->setAccessibleName("Showing " + monthTitle(chosenMonth));
    todayButton->setVisible(chosenMonth != currentMonth());
}
