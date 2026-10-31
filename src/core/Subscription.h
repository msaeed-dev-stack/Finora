// =====================================================================
//  Subscription.h
//  One person's subscription: the plan, its status and its dates, and
//  the rules of its lifecycle (a small state machine).
//  Plain C++ (no Qt), so the tests can check every rule directly.
// =====================================================================
#ifndef FINORA_SUBSCRIPTION_H
#define FINORA_SUBSCRIPTION_H

#include <string>

#include "Plans.h"

// Where a subscription stands. An enum gives each state a name, so the
// code says STATUS_PAST_DUE instead of a number such as 3.
enum SubscriptionStatus {
    STATUS_FREE,        // on Finora Free
    STATUS_TRIAL,       // a free 14-day trial of Finora Pro
    STATUS_ACTIVE,      // paid until the renewal date
    STATUS_PAST_DUE,    // the renewal date has passed and the renewal is not paid yet
    STATUS_CANCELLED,   // cancelled, but paid until the end of the current period
    STATUS_EXPIRED      // a trial or a paid plan ended without payment: back on Free
};
const int STATUS_COUNT = 6;

const int TRIAL_DAYS = 14;            // the free trial of Finora Pro, once per account
const int GRACE_DAYS = 7;             // paid features stay this long after a missed renewal
const int BILLING_PERIOD_MONTHS = 1;  // every paid plan is billed monthly

// One row of the subscriptions table. Every time is UTC text such as
// "2026-10-31T09:30:00Z". Written this way, text order is time order, so
// two times can be compared with < like two numbers.
struct Subscription {
    int id = 0;                           // the subscription id
    PlanId plan = PLAN_FREE;
    SubscriptionStatus status = STATUS_FREE;
    std::string startedAt;                // when the current plan or trial began
    std::string renewsAt;                 // the renewal date; "" if none
    std::string expiresAt;                // when paid features end without further payment; "" on Free
    std::string cancelledAt;              // when the person cancelled; "" if not cancelled
    bool hasScheduledPlan = false;        // true when a smaller plan starts at the renewal date
    PlanId scheduledPlan = PLAN_FREE;     // that smaller plan
    bool trialUsed = false;               // every account may try Pro once
    std::string lastPaymentReference;     // for example "SIM-7KQ4M2TX9RHC"; "" before any payment
    std::string lastPaymentStatus;        // "succeeded", "failed" or ""
};

// The database code of a status: "past_due" for STATUS_PAST_DUE.
std::string statusCode(SubscriptionStatus status);

// Finds the status with this code. Returns false for an unknown code.
bool statusFromCode(const std::string &code, SubscriptionStatus &status);

// Words for the screen: "Past due".
std::string statusName(SubscriptionStatus status);

// ---- The lifecycle ------------------------------------------------------

// What a stored subscription has become at `nowUtc`, after every deadline
// that has passed in the meantime:
//   Active     its renewal date passed      -> Past due (a smaller plan
//                                              chosen earlier starts now)
//   Trial, Past due or Cancelled: its end of access passed
//                                           -> Expired, back on Free
// Nothing else changes with time. The result is what the database row
// should say now.
Subscription subscriptionAt(const Subscription &stored, const std::string &nowUtc);

// The plan whose features may be used at `nowUtc`. Every entitlement
// check in Finora starts here.
PlanId planAt(const Subscription &stored, const std::string &nowUtc);

// What choosing the plan `wanted` would mean for the person's current
// (already up-to-date) subscription.
enum PlanChange {
    CHANGE_NONE,        // nothing to do: it is already the plan, or already arranged
    CHANGE_SUBSCRIBE,   // pay now; a new monthly period starts today
    CHANGE_UPGRADE,     // pay the larger plan's full price now; a new period starts today
    CHANGE_RENEW,       // past due on this plan: pay now to continue it
    CHANGE_DOWNGRADE,   // a smaller paid plan starts at the renewal date; nothing to pay now
    CHANGE_CANCEL,      // back to Free: at the end of the paid period (a trial or a
                        // missed renewal ends at once)
    CHANGE_RESUME       // keep the current plan after a cancellation or a scheduled
                        // smaller plan; it is already paid
};
PlanChange planChangeFor(const Subscription &current, PlanId wanted);

// True if this change needs a payment before anything else happens.
bool changeNeedsPayment(PlanChange change);

// The payments.reason value of a paid change: "new", "upgrade" or "renewal".
std::string paymentReasonFor(PlanChange change);

// True if the free Pro trial may start: on Free or Expired, and never
// used by this account before.
bool canStartTrial(const Subscription &current);

#endif // FINORA_SUBSCRIPTION_H
