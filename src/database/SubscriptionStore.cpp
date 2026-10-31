// =====================================================================
//  SubscriptionStore.cpp
// =====================================================================
#include "SubscriptionStore.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

// A text column as std::string; SQL NULL becomes "".
std::string textColumn(const QSqlQuery &query, int column)
{
    return query.value(column).toString().toStdString();
}

// The time now in UTC, as stored: "2026-10-31T09:30:00Z".
QString subscriptionTimeNow()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

// SQL NULL for "", otherwise the text.
QVariant optionalText(const std::string &text)
{
    if (text.empty()) {
        return QVariant();
    }
    return QString::fromStdString(text);
}

// The scheduled plan's code, or SQL NULL when no smaller plan is waiting.
QVariant scheduledPlanValue(const Subscription &subscription)
{
    if (!subscription.hasScheduledPlan) {
        return QVariant();
    }
    return QString::fromStdString(planCode(subscription.scheduledPlan));
}

SubscriptionStore::SubscriptionStore(DatabaseManager &sharedDatabase)
    : database(sharedDatabase)
{
}

bool SubscriptionStore::loadSubscription(Subscription &subscription)
{
    errorMessage.clear();
    subscription = Subscription();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("SELECT id, plan, status, started_at, renews_at, expires_at, cancelled_at, "
                  "scheduled_plan, trial_used, last_payment_reference, last_payment_status "
                  "FROM subscriptions WHERE user_id = :owner");
    query.bindValue(":owner", userId);
    if (!query.exec()) {
        return fail("Could not load the subscription: " + query.lastError().text());
    }
    if (!query.next()) {
        return fail("This account has no subscription record.");
    }

    // The codes are checked, so a damaged value never becomes a plan.
    if (!planFromCode(textColumn(query, 1), subscription.plan)
        || !statusFromCode(textColumn(query, 2), subscription.status)) {
        return fail("The subscription record has an unknown plan or status.");
    }
    subscription.id = query.value(0).toInt();
    subscription.startedAt = textColumn(query, 3);
    subscription.renewsAt = textColumn(query, 4);
    subscription.expiresAt = textColumn(query, 5);
    subscription.cancelledAt = textColumn(query, 6);
    if (!query.value(7).isNull()) {
        if (!planFromCode(textColumn(query, 7), subscription.scheduledPlan)) {
            return fail("The subscription record has an unknown scheduled plan.");
        }
        subscription.hasScheduledPlan = true;
    }
    subscription.trialUsed = (query.value(8).toInt() == 1);
    subscription.lastPaymentReference = textColumn(query, 9);
    subscription.lastPaymentStatus = textColumn(query, 10);
    return true;
}

bool SubscriptionStore::loadPayments(std::vector<PaymentRecord> &payments)
{
    errorMessage.clear();
    payments.clear();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("SELECT id, plan, reason, amount_minor, currency_code, status, provider, reference, "
                  "failure_reason, period_start, period_end, created_at, completed_at "
                  "FROM payments WHERE user_id = :owner ORDER BY id DESC");
    query.bindValue(":owner", userId);
    if (!query.exec()) {
        return fail("Could not load the billing history: " + query.lastError().text());
    }
    while (query.next()) {
        PaymentRecord payment;
        if (!planFromCode(textColumn(query, 1), payment.plan)) {
            return fail("A payment record has an unknown plan.");
        }
        payment.id = query.value(0).toInt();
        payment.reason = textColumn(query, 2);
        payment.amountMinor = query.value(3).toLongLong();
        payment.currencyCode = textColumn(query, 4);
        payment.status = textColumn(query, 5);
        payment.provider = textColumn(query, 6);
        payment.reference = textColumn(query, 7);
        payment.failureReason = textColumn(query, 8);
        payment.periodStart = textColumn(query, 9);
        payment.periodEnd = textColumn(query, 10);
        payment.createdAt = textColumn(query, 11);
        payment.completedAt = textColumn(query, 12);
        payments.push_back(payment);
    }
    return true;
}

QString SubscriptionStore::lastError() const
{
    return errorMessage;
}

// ---- Changes (only SubscriptionService, the friend class, calls these) ------
// Each placeholder name is used once per statement, so every value binds
// safely; where the same value is needed twice, it is bound twice.

// Saves what time has done to a subscription (see subscriptionAt() in
// core/Subscription.h): Active -> Past due, or the end of access -> Expired.
bool SubscriptionStore::applyTimeChange(const Subscription &before, const Subscription &after)
{
    errorMessage.clear();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    // Time can only LOWER a plan: to Free, to the plan it had, or to the
    // smaller plan that was chosen earlier. Never to a larger one.
    query.prepare("UPDATE subscriptions SET plan = :plan, status = :status, renews_at = :renews, "
                  "expires_at = :expires, scheduled_plan = :scheduled, updated_at = :now "
                  "WHERE user_id = :owner AND status = :oldStatus AND plan = :oldPlan "
                  "AND (:planCheck1 = 'free' OR :planCheck2 = plan OR :planCheck3 = scheduled_plan)");
    QString newPlan = QString::fromStdString(planCode(after.plan));
    query.bindValue(":plan", newPlan);
    query.bindValue(":status", QString::fromStdString(statusCode(after.status)));
    query.bindValue(":renews", optionalText(after.renewsAt));
    query.bindValue(":expires", optionalText(after.expiresAt));
    query.bindValue(":scheduled", scheduledPlanValue(after));
    query.bindValue(":now", subscriptionTimeNow());
    query.bindValue(":owner", userId);
    query.bindValue(":oldStatus", QString::fromStdString(statusCode(before.status)));
    query.bindValue(":oldPlan", QString::fromStdString(planCode(before.plan)));
    query.bindValue(":planCheck1", newPlan);
    query.bindValue(":planCheck2", newPlan);
    query.bindValue(":planCheck3", newPlan);

    std::string eventType = "subscription_expired";
    std::string details = planName(before.plan) + " ended; Finora Free applies";
    if (after.status == STATUS_PAST_DUE) {
        eventType = "renewal_due";
        details = "The renewal of " + planName(after.plan) + " was due on " + after.renewsAt;
    }
    return changeOneRow(query, userId, eventType, details, "The subscription changed in the meantime.");
}

bool SubscriptionStore::startTrial(const std::string &startedAt, const std::string &endsAt)
{
    errorMessage.clear();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    // Only from Free or Expired, and only if this account never had the trial.
    query.prepare("UPDATE subscriptions SET plan = 'pro', status = 'trial', started_at = :start, "
                  "renews_at = NULL, expires_at = :end, cancelled_at = NULL, scheduled_plan = NULL, "
                  "trial_used = 1, updated_at = :now "
                  "WHERE user_id = :owner AND status IN ('free', 'expired') AND trial_used = 0");
    query.bindValue(":start", QString::fromStdString(startedAt));
    query.bindValue(":end", QString::fromStdString(endsAt));
    query.bindValue(":now", subscriptionTimeNow());
    query.bindValue(":owner", userId);
    return changeOneRow(query, userId, "trial_started", "Finora Pro trial until " + endsAt,
                        "The free Pro trial is available once per account, while on Finora Free.");
}

// Records a payment attempt as "pending" BEFORE the provider is asked,
// so even a crash in the middle leaves a trace in the billing history.
bool SubscriptionStore::startPayment(PlanId plan, const std::string &reason, long long amountMinor,
                                     const std::string &reference, int &paymentId)
{
    errorMessage.clear();
    paymentId = 0;
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("INSERT INTO payments (user_id, subscription_id, plan, reason, amount_minor, currency_code, "
                  "status, provider, reference) "
                  "SELECT :owner, id, :plan, :reason, :amount, 'USD', 'pending', 'simulated', :reference "
                  "FROM subscriptions WHERE user_id = :subscriptionOwner");
    query.bindValue(":owner", userId);
    query.bindValue(":plan", QString::fromStdString(planCode(plan)));
    query.bindValue(":reason", QString::fromStdString(reason));
    query.bindValue(":amount", amountMinor);
    query.bindValue(":reference", QString::fromStdString(reference));
    query.bindValue(":subscriptionOwner", userId);
    if (!query.exec()) {
        return fail("Could not record the payment: " + query.lastError().text());
    }
    if (query.numRowsAffected() != 1) {
        return fail("This account has no subscription record.");
    }
    paymentId = query.lastInsertId().toInt();
    return true;
}

// The payment succeeded: in ONE transaction, the payment is marked as
// succeeded and the plan is switched on. The plan changes only if this
// very person has a succeeded payment for exactly this plan (the guard
// in the second statement), so no plan can ever be activated without a
// matching payment record.
bool SubscriptionStore::activatePaidPlan(int paymentId, PlanId plan, const std::string &periodStart,
                                         const std::string &periodEnd, const std::string &accessEnd)
{
    errorMessage.clear();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }
    QSqlDatabase db = QSqlDatabase::database(database.sqlConnectionName(), false);
    if (!db.transaction()) {
        return fail("Could not start a database transaction: " + db.lastError().text());
    }
    QString code = QString::fromStdString(planCode(plan));
    QString now = subscriptionTimeNow();

    QSqlQuery confirm(db);
    confirm.prepare("UPDATE payments SET status = 'succeeded', period_start = :start, period_end = :end, "
                    "completed_at = :now WHERE id = :payment AND user_id = :owner AND plan = :plan "
                    "AND status = 'pending'");
    confirm.bindValue(":start", QString::fromStdString(periodStart));
    confirm.bindValue(":end", QString::fromStdString(periodEnd));
    confirm.bindValue(":now", now);
    confirm.bindValue(":payment", paymentId);
    confirm.bindValue(":owner", userId);
    confirm.bindValue(":plan", code);
    if (!confirm.exec()) {
        return rollbackAndFail("Could not confirm the payment: " + confirm.lastError().text());
    }
    if (confirm.numRowsAffected() != 1) {
        return rollbackAndFail("The payment record does not match this account and plan, "
                               "so the plan was not changed.");
    }

    QSqlQuery activate(db);
    activate.prepare("UPDATE subscriptions SET plan = :plan, status = 'active', started_at = :start, "
                     "renews_at = :renews, expires_at = :accessEnd, cancelled_at = NULL, scheduled_plan = NULL, "
                     "last_payment_reference = (SELECT reference FROM payments WHERE id = :paymentForReference), "
                     "last_payment_status = 'succeeded', updated_at = :now "
                     "WHERE user_id = :owner AND EXISTS (SELECT 1 FROM payments WHERE id = :paymentForCheck "
                     "AND user_id = :paymentOwner AND plan = :paidPlan AND status = 'succeeded')");
    activate.bindValue(":plan", code);
    activate.bindValue(":start", QString::fromStdString(periodStart));
    activate.bindValue(":renews", QString::fromStdString(periodEnd));
    activate.bindValue(":accessEnd", QString::fromStdString(accessEnd));
    activate.bindValue(":paymentForReference", paymentId);
    activate.bindValue(":now", now);
    activate.bindValue(":owner", userId);
    activate.bindValue(":paymentForCheck", paymentId);
    activate.bindValue(":paymentOwner", userId);
    activate.bindValue(":paidPlan", code);
    if (!activate.exec()) {
        return rollbackAndFail("Could not switch on the plan: " + activate.lastError().text());
    }
    if (activate.numRowsAffected() != 1) {
        return rollbackAndFail("No succeeded payment matches this plan, so the plan was not changed.");
    }

    if (!insertEvent(userId, "plan_activated", planName(plan) + " until " + periodEnd + ", payment " + std::to_string(paymentId))) {
        return rollbackAndFail(errorMessage);
    }
    if (!db.commit()) {
        return rollbackAndFail("Could not save the new plan: " + db.lastError().text());
    }
    return true;
}

// The provider refused: the payment is marked as failed and the plan
// stays exactly as it was.
bool SubscriptionStore::failPayment(int paymentId, const std::string &reason)
{
    errorMessage.clear();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }
    QSqlDatabase db = QSqlDatabase::database(database.sqlConnectionName(), false);
    if (!db.transaction()) {
        return fail("Could not start a database transaction: " + db.lastError().text());
    }
    QString now = subscriptionTimeNow();

    QSqlQuery failed(db);
    failed.prepare("UPDATE payments SET status = 'failed', failure_reason = :reason, completed_at = :now "
                   "WHERE id = :payment AND user_id = :owner AND status = 'pending'");
    failed.bindValue(":reason", QString::fromStdString(reason));
    failed.bindValue(":now", now);
    failed.bindValue(":payment", paymentId);
    failed.bindValue(":owner", userId);
    if (!failed.exec() || failed.numRowsAffected() != 1) {
        return rollbackAndFail("Could not record the declined payment: " + failed.lastError().text());
    }

    QSqlQuery remember(db);
    remember.prepare("UPDATE subscriptions SET last_payment_reference = "
                     "(SELECT reference FROM payments WHERE id = :payment), "
                     "last_payment_status = 'failed', updated_at = :now WHERE user_id = :owner");
    remember.bindValue(":payment", paymentId);
    remember.bindValue(":now", now);
    remember.bindValue(":owner", userId);
    if (!remember.exec()) {
        return rollbackAndFail("Could not record the declined payment: " + remember.lastError().text());
    }
    if (!insertEvent(userId, "payment_failed", "Payment " + std::to_string(paymentId) + ": " + reason)) {
        return rollbackAndFail(errorMessage);
    }
    if (!db.commit()) {
        return rollbackAndFail("Could not record the declined payment: " + db.lastError().text());
    }
    return true;
}

// Active -> Cancelled: the plan stays until the end of the paid period
// (the renewal date becomes the end of access), then Free applies.
bool SubscriptionStore::cancelAtPeriodEnd(const std::string &cancelledAt)
{
    errorMessage.clear();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    // In SQL, every value on the right of SET is read from the row as it
    // was before, so expires_at receives the old renewal date.
    query.prepare("UPDATE subscriptions SET status = 'cancelled', cancelled_at = :cancelled, "
                  "expires_at = renews_at, renews_at = NULL, scheduled_plan = NULL, updated_at = :now "
                  "WHERE user_id = :owner AND status = 'active'");
    query.bindValue(":cancelled", QString::fromStdString(cancelledAt));
    query.bindValue(":now", subscriptionTimeNow());
    query.bindValue(":owner", userId);
    return changeOneRow(query, userId, "subscription_cancelled", "Cancelled; the paid period is kept until its end",
                        "Only an active subscription can be cancelled.");
}

// A trial or a missed renewal ends at once: Free applies again now.
bool SubscriptionStore::endNow(const std::string &endedAt)
{
    errorMessage.clear();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE subscriptions SET status = 'expired', plan = 'free', expires_at = :ended, "
                  "renews_at = NULL, cancelled_at = :cancelled, scheduled_plan = NULL, updated_at = :now "
                  "WHERE user_id = :owner AND status IN ('trial', 'past_due')");
    query.bindValue(":ended", QString::fromStdString(endedAt));
    query.bindValue(":cancelled", QString::fromStdString(endedAt));
    query.bindValue(":now", subscriptionTimeNow());
    query.bindValue(":owner", userId);
    return changeOneRow(query, userId, "subscription_ended", "Ended by the person; Finora Free applies",
                        "Only a trial or a missed renewal can be ended at once.");
}

// Cancelled -> Active again, before the paid period ends. Nothing is
// paid: that period was paid already.
bool SubscriptionStore::resume(const std::string &renewsAt, const std::string &accessEnd)
{
    errorMessage.clear();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE subscriptions SET status = 'active', renews_at = :renews, expires_at = :accessEnd, "
                  "cancelled_at = NULL, updated_at = :now "
                  "WHERE user_id = :owner AND status = 'cancelled' AND expires_at > :checkNow");
    QString now = subscriptionTimeNow();
    query.bindValue(":renews", QString::fromStdString(renewsAt));
    query.bindValue(":accessEnd", QString::fromStdString(accessEnd));
    query.bindValue(":now", now);
    query.bindValue(":owner", userId);
    query.bindValue(":checkNow", now);
    return changeOneRow(query, userId, "subscription_resumed", "The cancellation was withdrawn",
                        "Only a cancelled plan whose paid period has not ended can be resumed.");
}

// A smaller paid plan from the renewal date. Nothing is paid now.
bool SubscriptionStore::scheduleSmallerPlan(PlanId plan)
{
    errorMessage.clear();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    // The waiting plan must be smaller than the plan in use: Plus below
    // Pro or Business, Pro below Business.
    query.prepare("UPDATE subscriptions SET scheduled_plan = :scheduled, updated_at = :now "
                  "WHERE user_id = :owner AND status = 'active' "
                  "AND ((:smaller1 = 'plus' AND plan IN ('pro', 'business')) OR (:smaller2 = 'pro' AND plan = 'business'))");
    QString code = QString::fromStdString(planCode(plan));
    query.bindValue(":scheduled", code);
    query.bindValue(":now", subscriptionTimeNow());
    query.bindValue(":owner", userId);
    query.bindValue(":smaller1", code);
    query.bindValue(":smaller2", code);
    return changeOneRow(query, userId, "smaller_plan_scheduled", planName(plan) + " starts at the renewal date",
                        "Only a smaller paid plan can start at the renewal date.");
}

bool SubscriptionStore::clearScheduledPlan()
{
    errorMessage.clear();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE subscriptions SET scheduled_plan = NULL, updated_at = :now "
                  "WHERE user_id = :owner AND status = 'active' AND scheduled_plan IS NOT NULL");
    query.bindValue(":now", subscriptionTimeNow());
    query.bindValue(":owner", userId);
    return changeOneRow(query, userId, "smaller_plan_withdrawn", "The current plan continues",
                        "No smaller plan was waiting.");
}

// For the --billing-demo mode only: moves the deadlines of the current
// period, so that a renewal or an end can be shown during a demonstration.
bool SubscriptionStore::moveDeadlines(const std::string &renewsAt, const std::string &expiresAt)
{
    errorMessage.clear();
    int userId = 0;
    if (!signedInOwner(userId)) {
        return false;
    }
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE subscriptions SET renews_at = :renews, expires_at = :expires, updated_at = :now "
                  "WHERE user_id = :owner AND status IN ('trial', 'active', 'past_due', 'cancelled')");
    query.bindValue(":renews", optionalText(renewsAt));
    query.bindValue(":expires", QString::fromStdString(expiresAt));
    query.bindValue(":now", subscriptionTimeNow());
    query.bindValue(":owner", userId);
    return changeOneRow(query, userId, "billing_demo_period_end",
                        "Demonstration: the current period was made to end now", "There is no paid period to end.");
}

// Payments left "pending" by a crash are marked as failed. Only one copy
// of Finora can run at a time (InstanceLock), so at start-up a pending
// payment can only be one that was interrupted.
bool SubscriptionStore::failInterruptedPayments(int &count)
{
    errorMessage.clear();
    count = 0;
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("UPDATE payments SET status = 'failed', completed_at = :now, "
                  "failure_reason = 'Finora closed before this payment finished, so it was not completed.' "
                  "WHERE status = 'pending'");
    query.bindValue(":now", subscriptionTimeNow());
    if (!query.exec()) {
        return fail("Could not close interrupted payments: " + query.lastError().text());
    }
    count = query.numRowsAffected();
    if (count > 0) {
        insertEvent(0, "payments_interrupted", std::to_string(count) + " interrupted payment(s) marked as failed");
    }
    return true;
}

// Runs one prepared change, which must touch exactly one row, together
// with its audit event in ONE transaction. If the row did not fit the
// WHERE clause, `refusal` explains why and nothing is saved.
bool SubscriptionStore::changeOneRow(QSqlQuery &query, int userId, const std::string &eventType,
                                     const std::string &details, const QString &refusal)
{
    QSqlDatabase db = QSqlDatabase::database(database.sqlConnectionName(), false);
    if (!db.transaction()) {
        return fail("Could not start a database transaction: " + db.lastError().text());
    }
    if (!query.exec()) {
        return rollbackAndFail("Could not save the subscription: " + query.lastError().text());
    }
    if (query.numRowsAffected() != 1) {
        return rollbackAndFail(refusal);
    }
    if (!insertEvent(userId, eventType, details)) {
        return rollbackAndFail(errorMessage);
    }
    if (!db.commit()) {
        return rollbackAndFail("Could not save the subscription: " + db.lastError().text());
    }
    return true;
}

// One line of the audit history. userId 0 means "no account".
bool SubscriptionStore::insertEvent(int userId, const std::string &eventType, const std::string &details)
{
    QSqlQuery query(QSqlDatabase::database(database.sqlConnectionName(), false));
    query.prepare("INSERT INTO audit_events (user_id, event_type, details) VALUES (:user, :type, :details)");
    if (userId > 0) {
        query.bindValue(":user", userId);
    } else {
        query.bindValue(":user", QVariant());
    }
    query.bindValue(":type", QString::fromStdString(eventType));
    query.bindValue(":details", QString::fromStdString(details));
    if (!query.exec()) {
        return fail("Could not record an event: " + query.lastError().text());
    }
    return true;
}

bool SubscriptionStore::rollbackAndFail(const QString &message)
{
    QSqlDatabase::database(database.sqlConnectionName(), false).rollback();
    return fail(message);
}

// Subscriptions belong to accounts. The owner is the signed-in person
// of the Session that DatabaseManager is attached to.
bool SubscriptionStore::signedInOwner(int &userId)
{
    if (!database.isOpen()) {
        return fail("The database is not open.");
    }
    if (!database.signedInUserId(userId)) {
        return fail("Please sign in first.");
    }
    return true;
}

bool SubscriptionStore::fail(const QString &message)
{
    errorMessage = message;
    return false;
}
