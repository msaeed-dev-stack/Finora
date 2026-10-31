// =====================================================================
//  SubscriptionService.h
//  The subscription rules in action (Milestone 4, Phase 4C): the free
//  Pro trial, paying for a plan (subscribe, upgrade, renew), choosing a
//  smaller plan, cancelling, resuming, and what time does to a plan.
//
//  It is the only class that can change a subscription (the friend of
//  SubscriptionStore). The person always comes from the Session, through
//  DatabaseManager; the screen only says which plan was clicked, and the
//  price always comes from the plan table in core/Plans.h.
// =====================================================================
#ifndef FINORA_SUBSCRIPTIONSERVICE_H
#define FINORA_SUBSCRIPTIONSERVICE_H

#include <QString>

#include <string>

#include "PaymentGateway.h"
#include "core/Subscription.h"
#include "database/DatabaseManager.h"
#include "database/SubscriptionStore.h"

class SubscriptionService
{
public:
    // `sharedGateway` takes the payments; Finora passes a
    // SimulatedPaymentGateway. `billingDemo` is true only when Finora was
    // started with --billing-demo, which allows simulatePeriodEnd().
    // Payments that a crash left "pending" are marked as failed here.
    SubscriptionService(DatabaseManager &sharedDatabase, PaymentGateway &sharedGateway, bool billingDemo = false);

    QString lastError() const;
    bool isDemoMode() const;
    const PaymentGateway &gateway() const;

    // The signed-in person's subscription, with every deadline that has
    // passed applied AND saved (for example Active -> Past due), each
    // change with an audit event.
    bool refresh(Subscription &subscription);

    // What choosing `plan` would mean now (see planChangeFor() in core/Subscription.h).
    bool changeFor(PlanId plan, PlanChange &change);

    // Subscribes, upgrades or renews. The payment is recorded first, then
    // the gateway is asked; the plan is switched on ONLY if the payment
    // succeeded. On success `reference` is the payment's reference.
    // `simulateDecline` lets the demonstration show a refused payment.
    bool payForPlan(PlanId plan, bool simulateDecline, std::string &reference);

    // A smaller paid plan from the renewal date; nothing is paid now.
    bool switchAtRenewal(PlanId plan);

    // Back to Free at the end of the paid period. A trial or a missed
    // renewal ends at once. Nothing is ever deleted.
    bool cancel();

    // Keeps the current plan after a cancellation or after choosing a
    // smaller plan for the renewal date.
    bool keepCurrentPlan();

    // The free 14-day trial of Finora Pro, once per account.
    bool startTrial();

    // Only with --billing-demo: makes the current period end now, so that
    // a renewal, a missed payment or an expiry can be shown live.
    bool simulatePeriodEnd();

private:
    bool fail(const QString &message);

    SubscriptionStore store;     // the subscription SQL (composition), on main()'s database
    PaymentGateway &payments;    // created in main(); used, not owned
    bool demoMode = false;
    QString errorMessage;
};

#endif // FINORA_SUBSCRIPTIONSERVICE_H
