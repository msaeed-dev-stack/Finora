// =====================================================================
//  ProgressRing.cpp
// =====================================================================
#include "ProgressRing.h"

#include <QEasingCurve>
#include <QFont>
#include <QPainter>
#include <QPen>

#include "ui/Theme.h"

ProgressRing::ProgressRing(int ringDiameter, QWidget *parent)
    : QWidget(parent), diameter(ringDiameter)
{
    setFixedSize(diameter, diameter);

    animation = new QVariantAnimation(this);
    animation->setDuration(550);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(animation, &QVariantAnimation::valueChanged, this, &ProgressRing::onAnimationStep);

    startDelay = new QTimer(this);
    startDelay->setSingleShot(true);
    connect(startDelay, &QTimer::timeout, this, &ProgressRing::startFilling);
}

void ProgressRing::startFilling()
{
    animation->start();
}

void ProgressRing::setProgress(int percent, const QString &tone)
{
    percentValue = percent;
    toneName = tone;
    setAccessibleName(QString::number(percent) + "% used");

    double target = percent / 100.0;
    if (target > 1.0) {
        target = 1.0;   // over budget: the ring is simply full
    }
    if (target < 0.0) {
        target = 0.0;
    }

    animation->stop();
    startDelay->stop();
    if (isReducedMotion()) {
        shownFraction = target;
        update();
        return;
    }
    // The ring fills once a page fade has ended, so each step redraws only
    // the ring (0 ms: at the next turn of Qt's event loop).
    animation->setStartValue(0.0);
    animation->setEndValue(target);
    shownFraction = 0.0;
    update();
    startDelay->start(pageFadeRemaining());
}

int ProgressRing::percent() const
{
    return percentValue;
}

QString ProgressRing::tone() const
{
    return toneName;
}

QSize ProgressRing::sizeHint() const
{
    return QSize(diameter, diameter);
}

void ProgressRing::onAnimationStep(const QVariant &value)
{
    shownFraction = value.toDouble();
    update();   // asks Qt to call paintEvent again soon
}

void ProgressRing::paintEvent(QPaintEvent *)
{
    const ThemeColors &colors = themeColors();

    QColor ringColor = colors.income;
    if (toneName == "warning") {
        ringColor = colors.warning;
    } else if (toneName == "danger") {
        ringColor = colors.expense;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // The ring is a thick circle line inside the widget.
    double lineWidth = diameter / 9.0;
    QRectF circle(lineWidth / 2, lineWidth / 2, diameter - lineWidth, diameter - lineWidth);

    QPen trackPen(colors.surfaceMuted, lineWidth);
    painter.setPen(trackPen);
    painter.drawEllipse(circle);

    // drawArc measures angles in 1/16 of a degree. 90 degrees is the top
    // of the circle, and a negative span goes clockwise.
    QPen arcPen(ringColor, lineWidth, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(arcPen);
    int span = static_cast<int>(-360.0 * 16.0 * shownFraction);
    if (span != 0) {
        painter.drawArc(circle, 90 * 16, span);
    }

    // The percentage in the middle.
    QFont font("Inter");
    font.setPixelSize(qMax(10, diameter / 4));
    font.setWeight(QFont::DemiBold);
    painter.setFont(font);
    painter.setPen(colors.text);
    painter.drawText(rect(), Qt::AlignCenter, QString::number(percentValue) + "%");
}
