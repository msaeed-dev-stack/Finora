// =====================================================================
//  SubscriptionService.cpp
// =====================================================================
#include "SubscriptionService.h"

#include <QDateTime>

// A time as the database stores it: "2026-10-31T09:30:00Z".
std::string storedTime(const QDateTime &time)
{
    return time.toUTC().toString(Qt::ISODate).toStdString();
}

QDateTime readStoredTime(const std::string &text)
{
    return QDateTime::fromString(QString::fromStdString(text), Qt::ISODate);
}

SubscriptionService::SubscriptionService(DatabaseManager &sharedDatabase, PaymentGateway &sharedGateway,
                                         bool billingDemo)
    : store(sharedDatabase), payments(sharedGateway), demoMode(billingDemo)
{
    // Payments that a crash left "pending" are closed when Finora starts.
    int interrupted = 0;
    store.failInterruptedPayments(interrupted);
}

QString SubscriptionService::lastError() const
{
    return errorMessage;
}

bool SubscriptionService::isDemoMode() const
{
    return demoMode;
}

const PaymentGateway &SubscriptionService::gateway() const
{
    return payments;
}

bool SubscriptionService::refresh(Subscription &subscription)
{
    errorMessage.clear();
    Subscription stored;
    if (!store.loadSubscription(stored)) {
        return fail(store.lastError());
    }
    subscription = subscriptionAt(stored, storedTime(QDateTime::currentDateTimeUtc()));

    // Save what time has done, so the database, the screens and the
    // audit history all agree.
    if (subscription.status != stored.status || subscription.plan != stored.plan) {
        if (!store.applyTimeChange(stored, subscription)) {
            return fail(store.lastError());
        }
    }
    return true;
}

bool SubscriptionService::changeFor(PlanId plan, PlanChange &change)
{
    Subscription current;
    if (!refresh(current)) {
        return false;
    }
    change = planChangeFor(current, plan);
    return true;
}

bool SubscriptionService::payForPlan(PlanId plan, bool simulateDecline, std::string &reference)
{
    reference.clear();
    Subscription current;
    if (!refresh(current)) {
        return false;
    }
    PlanChange change = planChangeFor(current, plan);
    if (!changeNeedsPayment(change)) {
        return fail(QString::fromStdString(planName(plan)) + " needs no payment now.");
    }

    // The price comes from Finora's own plan table, never from the screen.
    long long amount = PLANS[plan].priceMinor;
    std::string newReference = payments.newReference();

    // 1. The attempt is recorded first, as "pending".
    int paymentId = 0;
    if (!store.startPayment(plan, paymentReasonFor(change), amount, newReference, paymentId)) {
        return fail(store.lastError());
    }

    // 2. The payment provider is asked. The request has no card details.
    PaymentRequest request;
    request.reference = newReference;
    request.description = planName(plan) + ", 1 month";
    request.amountMinor = amount;
    request.currencyCode = PLAN_PRICE_CURRENCY;
    request.simulateDecline = simulateDecline;
    PaymentResult result = payments.charge(request);   // a virtual call: the real gateway's own charge()

    // 3a. Refused: the attempt is marked as failed and the plan stays as it was.
    if (!result.succeeded) {
        if (!store.failPayment(paymentId, result.message)) {
            return fail(store.lastError());
        }
        return fail(QString::fromStdString(result.message) + " Your plan has not changed.");
    }

    // 3b. Paid: a new monthly period starts now. Without a renewal, the
    //     paid features stay GRACE_DAYS after its end. QDateTime handles
    //     months of different lengths (31 January + 1 month = 28 or 29 February).
    QDateTime start = QDateTime::currentDateTimeUtc();
    QDateTime end = start.addMonths(BILLING_PERIOD_MONTHS);
    QDateTime accessEnd = end.addDays(GRACE_DAYS);
    if (!store.activatePaidPlan(paymentId, plan, storedTime(start), storedTime(end), storedTime(accessEnd))) {
        // The plan was not switched on, so the attempt must not stay "pending".
        QString reason = store.lastError();
        store.failPayment(paymentId, "The plan could not be switched on: " + reason.toStdString());
        return fail(reason);
    }
    reference = newReference;
    return true;
}

bool SubscriptionService::switchAtRenewal(PlanId plan)
{
    Subscription current;
    if (!refresh(current)) {
        return false;
    }
    if (planChangeFor(current, plan) != CHANGE_DOWNGRADE) {
        return fail(QString::fromStdString(planName(plan)) + " cannot be chosen for the renewal date now.");
    }
    // A cancelled plan continues first; then the smaller plan waits for the renewal date.
    if (current.status == STATUS_CANCELLED) {
        std::string accessEnd = storedTime(readStoredTime(current.expiresAt).addDays(GRACE_DAYS));
        if (!store.resume(current.expiresAt, accessEnd)) {
            return fail(store.lastError());
        }
    }
    if (!store.scheduleSmallerPlan(plan)) {
        return fail(store.lastError());
    }
    return true;
}

bool SubscriptionService::cancel()
{
    Subscription current;
    if (!refresh(current)) {
        return false;
    }
    if (planChangeFor(current, PLAN_FREE) != CHANGE_CANCEL) {
        return fail("There is no paid plan or trial to cancel.");
    }
    std::string now = storedTime(QDateTime::currentDateTimeUtc());
    bool saved = false;
    if (current.status == STATUS_ACTIVE) {
        saved = store.cancelAtPeriodEnd(now);   // keeps the paid period
    } else {
        saved = store.endNow(now);              // a trial or a missed renewal
    }
    if (!saved) {
        return fail(store.lastError());
    }
    return true;
}

bool SubscriptionService::keepCurrentPlan()
{
    Subscription current;
    if (!refresh(current)) {
        return false;
    }
    if (planChangeFor(current, current.plan) != CHANGE_RESUME) {
        return fail("There is no cancellation or smaller plan to undo.");
    }
    bool saved = false;
    if (current.status == STATUS_CANCELLED) {
        // The paid period continues: its end becomes the renewal date again.
        std::string accessEnd = storedTime(readStoredTime(current.expiresAt).addDays(GRACE_DAYS));
        saved = store.resume(current.expiresAt, accessEnd);
    } else {
        saved = store.clearScheduledPlan();
    }
    if (!saved) {
        return fail(store.lastError());
    }
    return true;
}

bool SubscriptionService::startTrial()
{
    Subscription current;
    if (!refresh(current)) {
        return false;
    }
    if (!canStartTrial(current)) {
        return fail("The free Pro trial is available once per account, while on Finora Free.");
    }
    QDateTime start = QDateTime::currentDateTimeUtc();
    if (!store.startTrial(storedTime(start), storedTime(start.addDays(TRIAL_DAYS)))) {
        return fail(store.lastError());
    }
    return true;
}

bool SubscriptionService::simulatePeriodEnd()
{
    // A check in the service itself: no screen can call this outside the demonstration mode.
    if (!demoMode) {
        return fail("This works only in the billing demonstration (Finora started with --billing-demo).");
    }
    Subscription current;
    if (!refresh(current)) {
        return false;
    }
    QDateTime now = QDateTime::currentDateTimeUtc();
    bool saved = false;
    if (current.status == STATUS_ACTIVE) {
        // The renewal date is now; the grace period starts now.
        saved = store.moveDeadlines(storedTime(now), storedTime(now.addDays(GRACE_DAYS)));
    } else if (current.status == STATUS_TRIAL || current.status == STATUS_PAST_DUE
               || current.status == STATUS_CANCELLED) {
        saved = store.moveDeadlines(current.renewsAt, storedTime(now));   // the access ends now
    } else {
        return fail("There is no paid period or trial to end.");
    }
    if (!saved) {
        return fail(store.lastError());
    }
    Subscription after;
    return refresh(after);   // applies and saves what the new deadline means
}

bool SubscriptionService::fail(const QString &message)
{
    errorMessage = message;
    return false;
}
