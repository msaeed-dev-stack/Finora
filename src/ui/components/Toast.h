// =====================================================================
//  Toast.h
//  A short message that appears at the bottom of the window, for
//  example "Budget saved.", and disappears again by itself.
// =====================================================================
#ifndef FINORA_TOAST_H
#define FINORA_TOAST_H

#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

class Toast : public QLabel
{
    Q_OBJECT

public:
    // Shows `message` at the bottom of the window that contains `anyWidget`.
    // A new message replaces the one that is showing. With "reduce
    // motion" on, the message appears and disappears without fading.
    static void showMessage(QWidget *anyWidget, const QString &message);

    Toast(QWidget *window);

private slots:
    void startHiding();
    void onFadeFinished();

private:
    void display(const QString &message);
    void placeAtBottom();

    QTimer *hideTimer = nullptr;                    // how long the message stays
    QGraphicsOpacityEffect *opacityEffect = nullptr;   // lets the label fade
    QPropertyAnimation *fade = nullptr;             // changes the opacity smoothly
    bool hiding = false;                            // true while fading out
};

#endif // FINORA_TOAST_H
