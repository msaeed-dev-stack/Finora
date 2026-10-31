// =====================================================================
//  CheckoutDialog.h
//  The simulated checkout (Milestone 4, Phase 4C): the plan, its price
//  and the period, then two buttons that ask the SIMULATED payment
//  gateway to approve or to decline. There is no card field at all: a
//  real version would open the payment provider's own secure page.
// =====================================================================
#ifndef FINORA_CHECKOUTDIALOG_H
#define FINORA_CHECKOUTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>

#include "billing/SubscriptionService.h"
#include "core/Subscription.h"

class CheckoutDialog : public QDialog
{
    Q_OBJECT

public:
    // `change` says what kind of payment this is (subscribe, upgrade or renew).
    CheckoutDialog(SubscriptionService &sharedService, PlanId chosenPlan, PlanChange change,
                   QWidget *parent = nullptr);

    // After a successful payment: its reference, for example "SIM-7KQ4M2TX9RHC".
    QString paymentReference() const;

    // True once any payment was attempted; a declined one also enters
    // the billing history.
    bool paymentAttempted() const;

    // The message shown after a declined payment, for the tests.
    QString errorText() const;

private slots:
    void onConfirmClicked();
    void onDeclineClicked();

private:
    void pay(bool simulateDecline);

    SubscriptionService &service;   // created in main(); used, not owned
    PlanId plan;
    QLabel *errorLabel = nullptr;
    QString reference;
    bool attempted = false;
};

#endif // FINORA_CHECKOUTDIALOG_H
