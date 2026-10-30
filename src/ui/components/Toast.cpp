// =====================================================================
//  Toast.cpp
// =====================================================================
#include "Toast.h"

#include <QAccessible>

#include "ui/Theme.h"

void Toast::showMessage(QWidget *anyWidget, const QString &message)
{
    QWidget *window = anyWidget->window();

    // Reuse the window's toast if it already has one.
    Toast *toast = window->findChild<Toast *>(QString(), Qt::FindDirectChildrenOnly);
    if (toast == nullptr) {
        toast = new Toast(window);
    }
    toast->display(message);
}

Toast::Toast(QWidget *window)
    : QLabel(window)
{
    setObjectName("toast");
    setAlignment(Qt::AlignCenter);
    setAttribute(Qt::WA_TransparentForMouseEvents);   // clicks go to the widgets underneath
    hide();

    opacityEffect = new QGraphicsOpacityEffect(this);
    opacityEffect->setOpacity(1.0);
    setGraphicsEffect(opacityEffect);

    fade = new QPropertyAnimation(opacityEffect, "opacity", this);
    connect(fade, &QPropertyAnimation::finished, this, &Toast::onFadeFinished);

    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    connect(hideTimer, &QTimer::timeout, this, &Toast::startHiding);
}

void Toast::display(const QString &message)
{
    setText(message);
    setAccessibleName(message);
    adjustSize();
    placeAtBottom();
    raise();   // above every other widget of the window
    show();

    fade->stop();
    hiding = false;
    if (isReducedMotion()) {
        opacityEffect->setOpacity(1.0);
    } else {
        fade->setDuration(160);
        fade->setStartValue(0.0);
        fade->setEndValue(1.0);
        fade->start();
    }
    hideTimer->start(3200);

    // Screen readers announce the message, so users who cannot see it hear it.
    QAccessibleAnnouncementEvent announcement(this, message);
    QAccessible::updateAccessibility(&announcement);
}

void Toast::startHiding()
{
    if (isReducedMotion()) {
        hide();
        return;
    }
    hiding = true;
    fade->setDuration(220);
    fade->setStartValue(1.0);
    fade->setEndValue(0.0);
    fade->start();
}

void Toast::onFadeFinished()
{
    if (hiding) {
        hide();
        opacityEffect->setOpacity(1.0);
        hiding = false;
    }
}

// Centred near the bottom edge of the window.
void Toast::placeAtBottom()
{
    QWidget *window = parentWidget();
    int left = (window->width() - width()) / 2;
    int top = window->height() - height() - 32;
    move(left, top);
}
