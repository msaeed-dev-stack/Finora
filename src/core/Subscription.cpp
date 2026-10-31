// =====================================================================
//  Subscription.cpp
// =====================================================================
#include "Subscription.h"

// The database code and the screen name of every status, in the order
// of the SubscriptionStatus enum.
const std::string STATUS_CODES[STATUS_COUNT] = {
    "free", "trial", "active", "past_due", "cancelled", "expired"
};
const std::string STATUS_NAMES[STATUS_COUNT] = {
    "Free", "Trial", "Active", "Past due", "Cancelled", "Expired"
};

std::string statusCode(SubscriptionStatus status)
{
    return STATUS_CODES[status];
}

bool statusFromCode(const std::string &code, SubscriptionStatus &status)
{
    for (int i = 0; i < STATUS_COUNT; i++) {
        if (code == STATUS_CODES[i]) {
            status = static_cast<SubscriptionStatus>(i);
            return true;
        }
    }
    return false;
}

std::string statusName(SubscriptionStatus status)
{
    return STATUS_NAMES[status];
}

// ---- The lifecycle ------------------------------------------------------

Subscription subscriptionAt(const Subscription &stored, const std::string &nowUtc)
{
    Subscription result = stored;

    // 1. A paid period whose renewal date has passed becomes past due.
    //    "!(now < renewsAt)" means "now is at or after the renewal date".
    if (result.status == STATUS_ACTIVE && !(nowUtc < result.renewsAt)) {
        result.status = STATUS_PAST_DUE;
        if (result.hasScheduledPlan) {   // a smaller plan chosen earlier starts now
            result.plan = result.scheduledPlan;
            result.hasScheduledPlan = false;
        }
    }

    // 2. A trial, a missed renewal and a cancelled plan end at expiresAt.
    //    After that, Finora Free applies again; no record is deleted.
    bool endsAtExpiry = (result.status == STATUS_TRIAL || result.status == STATUS_PAST_DUE
                         || result.status == STATUS_CANCELLED);
    if (endsAtExpiry && !(nowUtc < result.expiresAt)) {
        result.status = STATUS_EXPIRED;
        result.plan = PLAN_FREE;
        result.renewsAt = "";
        result.hasScheduledPlan = false;
    }
    return result;
}

PlanId planAt(const Subscription &stored, const std::string &nowUtc)
{
    // Free and Expired are always on PLAN_FREE, so the plan of the
    // up-to-date subscription is the plan whose features may be used.
    return subscriptionAt(stored, nowUtc).plan;
}

PlanChange planChangeFor(const Subscription &current, PlanId wanted)
{
    SubscriptionStatus status = current.status;

    // On Free (never paid, or ended): any paid plan is a new subscription.
    if (status == STATUS_FREE || status == STATUS_EXPIRED) {
        if (wanted == PLAN_FREE) {
            return CHANGE_NONE;
        }
        return CHANGE_SUBSCRIBE;
    }

    // During the trial: pay for a plan to keep it, or go back to Free now.
    if (status == STATUS_TRIAL) {
        if (wanted == PLAN_FREE) {
            return CHANGE_CANCEL;
        }
        return CHANGE_SUBSCRIBE;
    }

    // Past due: pay to continue, choose another plan, or stop.
    if (status == STATUS_PAST_DUE) {
        if (wanted == PLAN_FREE) {
            return CHANGE_CANCEL;
        }
        if (wanted == current.plan) {
            return CHANGE_RENEW;
        }
        if (wanted > current.plan) {
            return CHANGE_UPGRADE;
        }
        return CHANGE_SUBSCRIBE;   // a smaller plan, paid from today
    }

    // Active or Cancelled: the current period is already paid.
    if (wanted == PLAN_FREE) {
        if (status == STATUS_CANCELLED) {
            return CHANGE_NONE;    // it already ends at the end of the period
        }
        return CHANGE_CANCEL;
    }
    if (wanted > current.plan) {
        return CHANGE_UPGRADE;
    }
    if (wanted < current.plan) {
        if (status == STATUS_ACTIVE && current.hasScheduledPlan && current.scheduledPlan == wanted) {
            return CHANGE_NONE;    // this smaller plan is already arranged
        }
        return CHANGE_DOWNGRADE;
    }
    // The same plan: continue it if it was cancelled or a smaller plan was arranged.
    if (status == STATUS_CANCELLED || current.hasScheduledPlan) {
        return CHANGE_RESUME;
    }
    return CHANGE_NONE;
}

bool changeNeedsPayment(PlanChange change)
{
    return change == CHANGE_SUBSCRIBE || change == CHANGE_UPGRADE || change == CHANGE_RENEW;
}

std::string paymentReasonFor(PlanChange change)
{
    if (change == CHANGE_UPGRADE) {
        return "upgrade";
    }
    if (change == CHANGE_RENEW) {
        return "renewal";
    }
    return "new";
}

bool canStartTrial(const Subscription &current)
{
    return (current.status == STATUS_FREE || current.status == STATUS_EXPIRED) && !current.trialUsed;
}
