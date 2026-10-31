// =====================================================================
//  SubscriptionStore.h
//  All SQL for subscriptions and the billing history (Milestone 4,
//  Phase 4C). Every function works on the signed-in person's own rows:
//  the user id comes from the Session, never from the user interface.
// =====================================================================
#ifndef FINORA_SUBSCRIPTIONSTORE_H
#define FINORA_SUBSCRIPTIONSTORE_H

#include <QString>

#include <string>
#include <vector>

class QSqlQuery;   // a Qt SQL class; only SubscriptionStore.cpp needs its details

#include "DatabaseManager.h"
#include "core/Subscription.h"

// One payment attempt: one row of the billing history. Finora stores no
// card details at all; a payment is only a plan, an amount and a reference.
struct PaymentRecord {
    int id = 0;
    PlanId plan = PLAN_PLUS;
    std::string reason;          // "new", "upgrade" or "renewal"
    long long amountMinor = 0;   // US cents: 1299 means $12.99
    std::string currencyCode;    // "USD"
    std::string status;          // "pending", "succeeded" or "failed"
    std::string provider;        // "simulated": no real payment provider is connected
    std::string reference;       // for example "SIM-7KQ4M2TX9RHC"
    std::string failureReason;   // why it failed; "" otherwise
    std::string periodStart;     // the paid month, set when the payment succeeds
    std::string periodEnd;
    std::string createdAt;
    std::string completedAt;
};

class SubscriptionService;   // declared in src/billing/SubscriptionService.h

class SubscriptionStore
{
public:
    SubscriptionStore(DatabaseManager &sharedDatabase);

    // The signed-in person's subscription. Fails with "Please sign in
    // first." while nobody is signed in.
    bool loadSubscription(Subscription &subscription);

    // The signed-in person's billing history, newest first.
    bool loadPayments(std::vector<PaymentRecord> &payments);

    QString lastError() const;

private:
    // Every function that CHANGES a subscription or a payment is private,
    // and SubscriptionService is the only friend class: no screen can
    // change a plan or a payment by itself (the same idea as Session and
    // AuthService). Each change checks the current state in its WHERE
    // clause, so a change that does not fit touches no row and is undone.
    friend class SubscriptionService;

    bool applyTimeChange(const Subscription &before, const Subscription &after);
    bool startTrial(const std::string &startedAt, const std::string &endsAt);
    bool startPayment(PlanId plan, const std::string &reason, long long amountMinor,
                      const std::string &reference, int &paymentId);
    bool activatePaidPlan(int paymentId, PlanId plan, const std::string &periodStart,
                          const std::string &periodEnd, const std::string &accessEnd);
    bool failPayment(int paymentId, const std::string &reason);
    bool cancelAtPeriodEnd(const std::string &cancelledAt);
    bool endNow(const std::string &endedAt);
    bool resume(const std::string &renewsAt, const std::string &accessEnd);
    bool scheduleSmallerPlan(PlanId plan);
    bool clearScheduledPlan();
    bool moveDeadlines(const std::string &renewsAt, const std::string &expiresAt);
    bool failInterruptedPayments(int &count);

    bool changeOneRow(QSqlQuery &query, int userId, const std::string &eventType,
                      const std::string &details, const QString &refusal);
    bool insertEvent(int userId, const std::string &eventType, const std::string &details);
    bool rollbackAndFail(const QString &message);
    bool signedInOwner(int &userId);
    bool fail(const QString &message);

    DatabaseManager &database;   // created in main(); this store only uses it
    QString errorMessage;
};

#endif // FINORA_SUBSCRIPTIONSTORE_H
