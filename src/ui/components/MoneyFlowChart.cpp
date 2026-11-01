// =====================================================================
//  MoneyFlowChart.cpp
// =====================================================================
#include "MoneyFlowChart.h"

#include <QEasingCurve>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStringList>

#include "core/Money.h"
#include "ui/Theme.h"
#include "ui/UiHelpers.h"

// Space around the bars: on the left for the scale labels and at the
// bottom for the month names.
const double LEFT_MARGIN = 52.0;
const double RIGHT_MARGIN = 10.0;
const double TOP_MARGIN = 14.0;
const double BOTTOM_MARGIN = 28.0;
const int SCALE_STEPS = 4;

MoneyFlowChart::MoneyFlowChart(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("moneyFlowChart");
    setMouseTracking(true);              // mouseMoveEvent also without a pressed button
    setFocusPolicy(Qt::StrongFocus);     // reachable with the Tab key
    setAccessibleName("Money flow chart");

    animation = new QVariantAnimation(this);
    animation->setDuration(600);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(animation, &QVariantAnimation::valueChanged, this, &MoneyFlowChart::onAnimationStep);

    startDelay = new QTimer(this);
    startDelay->setSingleShot(true);
    connect(startDelay, &QTimer::timeout, this, &MoneyFlowChart::startGrowing);
}

void MoneyFlowChart::startGrowing()
{
    animation->start();
}

void MoneyFlowChart::setData(const std::vector<MonthFlow> &flows, const QString &currencyCode)
{
    monthFlows = flows;
    currency = currencyCode;
    if (highlight >= static_cast<int>(monthFlows.size())) {
        highlight = -1;
    }

    // The bars grow from the bottom, unless the user asked for less motion.
    // They start once a page fade has ended, so each step redraws only the
    // chart (0 ms: at the next turn of Qt's event loop).
    animation->stop();
    startDelay->stop();
    if (isReducedMotion()) {
        growth = 1.0;
    } else {
        growth = 0.0;
        startDelay->start(pageFadeRemaining());
    }

    setAccessibleDescription(summaryText());
    emit summaryChanged(summaryText());
    update();
}

const std::vector<MonthFlow> &MoneyFlowChart::flows() const
{
    return monthFlows;
}

int MoneyFlowChart::highlightedIndex() const
{
    return highlight;
}

void MoneyFlowChart::setHighlightedIndex(int index)
{
    int count = static_cast<int>(monthFlows.size());
    if (index < -1 || index >= count || index == highlight) {
        return;
    }
    highlight = index;
    setAccessibleDescription(summaryText());
    emit summaryChanged(summaryText());
    update();
}

QString MoneyFlowChart::summaryText() const
{
    if (monthFlows.empty()) {
        return QString();
    }
    int index = highlight;
    if (index < 0) {
        index = static_cast<int>(monthFlows.size()) - 1;   // the newest month
    }
    const MonthFlow &flow = monthFlows[index];
    QString month = monthTitle(QString::fromStdString(flow.month));
    if (flow.overflow) {
        return month + ": the totals are too large to show.";
    }

    std::string code = currency.toStdString();
    return month + ": income " + moneyText(code, flow.incomeMinor)
           + ", spent " + moneyText(code, flow.expenseMinor)
           + ", net " + QString::fromStdString(formatSignedMoney(code, flow.incomeMinor - flow.expenseMinor));
}

QSize MoneyFlowChart::sizeHint() const
{
    return QSize(560, 260);
}

QSize MoneyFlowChart::minimumSizeHint() const
{
    return QSize(320, 210);
}

// ---- Drawing -----------------------------------------------------------

void MoneyFlowChart::paintEvent(QPaintEvent *)
{
    const ThemeColors &colors = themeColors();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QFont smallFont("Inter");
    smallFont.setPixelSize(11);
    painter.setFont(smallFont);

    QRectF plot = plotArea();
    int count = static_cast<int>(monthFlows.size());

    if (count > 0 && hasOverflow()) {
        painter.setPen(colors.textMuted);
        painter.drawText(rect(), Qt::AlignCenter, "These totals are too large to show.");
        return;
    }

    // The largest bar decides the scale.
    long long largest = 0;
    for (int i = 0; i < count; i++) {
        largest = qMax(largest, qMax(monthFlows[i].incomeMinor, monthFlows[i].expenseMinor));
    }
    long long scaleMaximum = chartScaleMaximum(largest);

    // Grid lines with their values on the left: 0, 1/4, 2/4, 3/4 and the top.
    for (int step = 0; step <= SCALE_STEPS; step++) {
        double y = plot.bottom() - plot.height() * step / SCALE_STEPS;
        painter.setPen(QPen(colors.chartGrid, 1));
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));

        long long value = scaleMaximum / SCALE_STEPS * step;
        painter.setPen(colors.textMuted);
        painter.drawText(QRectF(0, y - 8, plot.left() - 10, 16), Qt::AlignRight | Qt::AlignVCenter,
                         QString::fromStdString(formatCompactAmount(value)));
    }

    if (count == 0) {
        return;
    }

    // Each month gets an equal slot: the income bar on the left half,
    // the spending bar on the right half.
    double slotWidth = plot.width() / count;
    double barWidth = qMin(slotWidth * 0.28, 20.0);

    for (int i = 0; i < count; i++) {
        double slotLeft = plot.left() + slotWidth * i;
        double centre = slotLeft + slotWidth / 2;

        if (i == highlight) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(colors.primarySoft);
            painter.drawRoundedRect(QRectF(slotLeft + 3, plot.top() - 6, slotWidth - 6, plot.height() + 6), 8, 8);
        }

        drawBar(painter, centre - barWidth - 2, barWidth, monthFlows[i].incomeMinor, scaleMaximum, plot,
                colors.income);
        drawBar(painter, centre + 2, barWidth, monthFlows[i].expenseMinor, scaleMaximum, plot, colors.expense);

        QColor labelColor = colors.textMuted;
        if (i == highlight) {
            labelColor = colors.text;
        }
        painter.setPen(labelColor);
        painter.drawText(QRectF(slotLeft, plot.bottom() + 7, slotWidth, 18), Qt::AlignHCenter | Qt::AlignTop,
                         shortMonthName(QString::fromStdString(monthFlows[i].month)));
    }

    if (!hasAnyMoney()) {
        painter.setPen(colors.textMuted);
        painter.drawText(plot, Qt::AlignCenter, "No income or spending in these months yet");
    }

    if (highlight >= 0 && hasAnyMoney()) {
        drawValueBubble(painter, plot, slotWidth);
    }

    // A visible ring while the chart has the keyboard focus.
    if (hasFocus()) {
        painter.setPen(QPen(colors.focus, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 10, 10);
    }
}

void MoneyFlowChart::drawBar(QPainter &painter, double left, double width, long long valueMinor,
                             long long scaleMaximum, const QRectF &plot, const QColor &color)
{
    if (valueMinor <= 0 || scaleMaximum <= 0) {
        return;
    }
    double height = plot.height() * static_cast<double>(valueMinor) / static_cast<double>(scaleMaximum) * growth;
    if (height < 2.0) {
        height = 2.0;   // a tiny amount still shows as a thin line
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    // Rounded at the top; the bottom corners are squared again by a
    // small rectangle, so the bar stands flat on the base line.
    painter.drawRoundedRect(QRectF(left, plot.bottom() - height, width, height), 4, 4);
    double square = qMin(height, 4.0);
    painter.drawRect(QRectF(left, plot.bottom() - square, width, square));
}

// The dark bubble with the exact amounts of the highlighted month.
void MoneyFlowChart::drawValueBubble(QPainter &painter, const QRectF &plot, double slotWidth)
{
    const ThemeColors &colors = themeColors();
    const MonthFlow &flow = monthFlows[highlight];
    std::string code = currency.toStdString();

    // A heading line, then three rows with a name on the left and the
    // amount on the right.
    const int ROW_COUNT = 3;
    QString names[ROW_COUNT] = {"Income", "Spent", "Net"};
    QString amounts[ROW_COUNT] = {
        moneyText(code, flow.incomeMinor),
        moneyText(code, flow.expenseMinor),
        QString::fromStdString(formatSignedMoney(code, flow.incomeMinor - flow.expenseMinor))
    };
    QString heading = monthTitle(QString::fromStdString(flow.month));

    QFont bubbleFont("Inter");
    bubbleFont.setPixelSize(12);
    QFont headingFont = bubbleFont;
    headingFont.setWeight(QFont::DemiBold);
    QFontMetrics metrics(bubbleFont);
    int lineHeight = metrics.height() + 3;

    int nameWidth = 0;
    int amountWidth = 0;
    for (int i = 0; i < ROW_COUNT; i++) {
        nameWidth = qMax(nameWidth, metrics.horizontalAdvance(names[i]));
        amountWidth = qMax(amountWidth, metrics.horizontalAdvance(amounts[i]));
    }
    int innerWidth = qMax(nameWidth + 18 + amountWidth, QFontMetrics(headingFont).horizontalAdvance(heading));

    double bubbleWidth = innerWidth + 44;   // padding and room for the coloured dots
    double bubbleHeight = lineHeight * (ROW_COUNT + 1) + 14;
    double centre = plot.left() + slotWidth * highlight + slotWidth / 2;
    double left = centre - bubbleWidth / 2;
    // Keep the whole bubble inside the widget.
    left = qMax(4.0, qMin(left, width() - bubbleWidth - 4.0));
    QRectF bubble(left, plot.top(), bubbleWidth, bubbleHeight);

    painter.setPen(Qt::NoPen);
    painter.setBrush(colors.tooltip);
    painter.drawRoundedRect(bubble, 10, 10);

    double textLeft = bubble.left() + 28;
    double textRight = bubble.right() - 14;
    painter.setFont(headingFont);
    painter.setPen(colors.tooltipText);
    painter.drawText(QRectF(bubble.left() + 14, bubble.top() + 7, bubbleWidth - 28, lineHeight),
                     Qt::AlignLeft | Qt::AlignVCenter, heading);

    painter.setFont(bubbleFont);
    for (int i = 0; i < ROW_COUNT; i++) {
        double y = bubble.top() + 7 + lineHeight * (i + 1);
        if (i < 2) {
            // Coloured dots match the bars: teal income, rose spending.
            QColor dot = colors.income;
            if (i == 1) {
                dot = colors.expense;
            }
            painter.setPen(Qt::NoPen);
            painter.setBrush(dot);
            painter.drawEllipse(QPointF(bubble.left() + 18, y + lineHeight / 2.0), 3.5, 3.5);
        }
        painter.setPen(colors.tooltipText);
        painter.drawText(QRectF(textLeft, y, nameWidth + 4, lineHeight), Qt::AlignLeft | Qt::AlignVCenter, names[i]);
        painter.drawText(QRectF(textLeft, y, textRight - textLeft, lineHeight), Qt::AlignRight | Qt::AlignVCenter,
                         amounts[i]);
    }
}

// ---- Interaction ---------------------------------------------------------

void MoneyFlowChart::mouseMoveEvent(QMouseEvent *event)
{
    setHighlightedIndex(monthIndexAt(event->position().x()));
}

void MoneyFlowChart::leaveEvent(QEvent *)
{
    if (!hasFocus()) {
        setHighlightedIndex(-1);
    }
}

void MoneyFlowChart::keyPressEvent(QKeyEvent *event)
{
    int count = static_cast<int>(monthFlows.size());
    if (count == 0) {
        QWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Left:
        setHighlightedIndex(qMax(0, highlight - 1));
        break;
    case Qt::Key_Right:
        setHighlightedIndex(qMin(count - 1, highlight + 1));
        break;
    case Qt::Key_Home:
        setHighlightedIndex(0);
        break;
    case Qt::Key_End:
        setHighlightedIndex(count - 1);
        break;
    default:
        QWidget::keyPressEvent(event);   // other keys, such as Tab, work as usual
        break;
    }
}

void MoneyFlowChart::focusInEvent(QFocusEvent *event)
{
    if (highlight < 0 && !monthFlows.empty()) {
        setHighlightedIndex(static_cast<int>(monthFlows.size()) - 1);
    }
    QWidget::focusInEvent(event);
    update();
}

void MoneyFlowChart::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    update();
}

void MoneyFlowChart::onAnimationStep(const QVariant &value)
{
    growth = value.toDouble();
    update();
}

// ---- Helpers ---------------------------------------------------------------

QRectF MoneyFlowChart::plotArea() const
{
    return QRectF(LEFT_MARGIN, TOP_MARGIN, width() - LEFT_MARGIN - RIGHT_MARGIN,
                  height() - TOP_MARGIN - BOTTOM_MARGIN);
}

// The month under the mouse at position x, or -1 outside the bars.
int MoneyFlowChart::monthIndexAt(double x) const
{
    QRectF plot = plotArea();
    int count = static_cast<int>(monthFlows.size());
    if (count == 0 || x < plot.left() || x >= plot.right()) {
        return -1;
    }
    int index = static_cast<int>((x - plot.left()) / (plot.width() / count));
    return qMin(index, count - 1);
}

bool MoneyFlowChart::hasAnyMoney() const
{
    int count = static_cast<int>(monthFlows.size());
    for (int i = 0; i < count; i++) {
        if (monthFlows[i].incomeMinor > 0 || monthFlows[i].expenseMinor > 0) {
            return true;
        }
    }
    return false;
}

bool MoneyFlowChart::hasOverflow() const
{
    int count = static_cast<int>(monthFlows.size());
    for (int i = 0; i < count; i++) {
        if (monthFlows[i].overflow) {
            return true;
        }
    }
    return false;
}
