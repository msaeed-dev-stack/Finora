// =====================================================================
//  MoneyFlowChart.h
//  A bar chart of income and spending per month for ONE currency,
//  drawn by Finora itself with QPainter.
//
//  It is interactive: pointing at a month with the mouse, or moving
//  with the Left and Right arrow keys, highlights that month and shows
//  its exact amounts. The same text is offered below the chart and to
//  screen readers, so nobody has to read values from bar heights.
// =====================================================================
#ifndef FINORA_MONEYFLOWCHART_H
#define FINORA_MONEYFLOWCHART_H

#include <QVariantAnimation>
#include <QWidget>

#include <vector>

#include "core/Totals.h"

class MoneyFlowChart : public QWidget
{
    Q_OBJECT

public:
    MoneyFlowChart(QWidget *parent = nullptr);

    // The months to draw, oldest first, all in `currencyCode`.
    void setData(const std::vector<MonthFlow> &flows, const QString &currencyCode);
    const std::vector<MonthFlow> &flows() const;

    // The highlighted month (0 = oldest), or -1 when none is highlighted.
    int highlightedIndex() const;
    void setHighlightedIndex(int index);

    // The amounts of the highlighted month, or of the newest month when
    // none is highlighted, for example
    // "October 2026: income PKR 120,000.00, spent PKR 1,580.50, net +PKR 118,419.50".
    QString summaryText() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void summaryChanged(const QString &text);

protected:
    // Virtual functions of QWidget that this class overrides (polymorphism):
    // Qt calls them when the widget must be drawn, the mouse moves, etc.
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QRectF plotArea() const;
    int monthIndexAt(double x) const;
    bool hasAnyMoney() const;
    bool hasOverflow() const;
    void drawBar(QPainter &painter, double left, double width, long long valueMinor,
                 long long scaleMaximum, const QRectF &plot, const QColor &color);
    void drawValueBubble(QPainter &painter, const QRectF &plot, double slotWidth);
    void onAnimationStep(const QVariant &value);

    std::vector<MonthFlow> monthFlows;
    QString currency;
    int highlight = -1;
    double growth = 1.0;   // 0.0 to 1.0 while the bars grow in
    QVariantAnimation *animation = nullptr;
};

#endif // FINORA_MONEYFLOWCHART_H
