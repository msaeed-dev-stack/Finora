// =====================================================================
//  MonthSwitcher.h
//  "<  October 2026  >": chooses the month a page shows.
// =====================================================================
#ifndef FINORA_MONTHSWITCHER_H
#define FINORA_MONTHSWITCHER_H

#include <QLabel>
#include <QPushButton>
#include <QWidget>

class MonthSwitcher : public QWidget
{
    Q_OBJECT

public:
    MonthSwitcher(QWidget *parent = nullptr);

    // The chosen month as "YYYY-MM". It starts with the current month.
    QString month() const;
    void setMonth(const QString &month);

    // Colours the arrow icons again after the theme changed.
    void refreshIcons();

signals:
    void monthChanged(const QString &month);

private slots:
    void showPreviousMonth();
    void showNextMonth();
    void showThisMonth();

private:
    void moveBy(int months);
    void updateLabel();

    QString chosenMonth;
    QPushButton *previousButton = nullptr;
    QPushButton *nextButton = nullptr;
    QPushButton *todayButton = nullptr;   // "This month", shown only for other months
    QLabel *monthLabel = nullptr;
};

#endif // FINORA_MONTHSWITCHER_H
