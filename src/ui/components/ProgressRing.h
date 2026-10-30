// =====================================================================
//  ProgressRing.h
//  A ring that fills up as a budget is used, with the percentage in
//  the middle. Drawn by Finora itself with QPainter.
// =====================================================================
#ifndef FINORA_PROGRESSRING_H
#define FINORA_PROGRESSRING_H

#include <QVariantAnimation>
#include <QWidget>

class ProgressRing : public QWidget
{
    Q_OBJECT

public:
    // `diameter` is the ring's width and height in pixels.
    ProgressRing(int diameter, QWidget *parent = nullptr);

    // percent may be above 100 (over budget); the ring is then full and
    // the number shows the real percentage. tone is "success",
    // "warning" or "danger" and chooses the colour.
    void setProgress(int percent, const QString &tone);

    int percent() const;
    QString tone() const;

    QSize sizeHint() const override;

protected:
    // Qt calls paintEvent whenever the widget must be drawn. Overriding
    // this virtual function is how a widget draws itself.
    void paintEvent(QPaintEvent *event) override;

private:
    void onAnimationStep(const QVariant &value);

    int diameter = 0;
    int percentValue = 0;
    QString toneName = "success";
    double shownFraction = 0.0;   // how much of the ring is drawn now, 0.0 to 1.0
    QVariantAnimation *animation = nullptr;
};

#endif // FINORA_PROGRESSRING_H
