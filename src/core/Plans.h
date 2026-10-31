// =====================================================================
//  Plans.h
//  Finora's four subscription plans, the features each plan includes
//  and the limits of each plan. This is the ONE place in Finora that
//  says what a plan allows (its "entitlements"). Every check in the
//  program asks these functions; none has its own list.
//  Plain C++ (no Qt), so the tests can check every rule directly.
// =====================================================================
#ifndef FINORA_PLANS_H
#define FINORA_PLANS_H

#include <string>

// The plans, from the smallest to the largest. The order matters: every
// plan includes everything of the plans before it, so "plan >= PLAN_PRO"
// means "Pro or Business".
enum PlanId {
    PLAN_FREE,
    PLAN_PLUS,
    PLAN_PRO,
    PLAN_BUSINESS
};
const int PLAN_COUNT = 4;

// One plan. Prices are whole US cents, like every amount in Finora
// (599 means $5.99), so no rounding can ever happen. They are proposed
// prices for this project and have not been tested on the market.
struct PlanInfo {
    const char *code;       // stored in the database: "free", "plus", "pro", "business"
    const char *name;       // shown on screen: "Finora Plus"
    long long priceMinor;   // US cents per month
    const char *summary;    // one line for the plan card
};

// PLANS[PLAN_PRO] describes Finora Pro: the position in the array is the PlanId.
const PlanInfo PLANS[PLAN_COUNT] = {
    {"free",     "Finora Free",     0,    "The essentials for tracking your money."},
    {"plus",     "Finora Plus",     599,  "Unlimited budgets and exports for everyday planning."},
    {"pro",      "Finora Pro",      1299, "Deeper analysis and reports for serious planners."},
    {"business", "Finora Business", 2499, "Everything in Pro, plus tools for running a business."}
};

// Every plan price is in US dollars.
const std::string PLAN_PRICE_CURRENCY = "USD";

// ---- Budget limits ------------------------------------------------------

const int FREE_ACTIVE_BUDGET_LIMIT = 3;   // Finora Free: at most 3 active budgets
const int UNLIMITED_BUDGETS = -1;          // returned when a plan has no limit

// ---- Features -----------------------------------------------------------

// Whether a feature can be used in this version of Finora. A plan can
// include a feature that is not built yet; the screens then say "Soon",
// so Finora never pretends that it works.
enum FeatureState {
    FEATURE_AVAILABLE,      // built and working
    FEATURE_PLANNED,        // part of the plan, but not built yet
    FEATURE_NEEDS_ONLINE    // needs Finora Online: a server and shared accounts
};

// Every feature of every plan, grouped by the plan that first includes it.
enum Feature {
    // Finora Free
    FEATURE_TRANSACTIONS,
    FEATURE_DASHBOARD,
    FEATURE_CURRENCIES,
    FEATURE_BUDGETS,
    FEATURE_RECENT_HISTORY,
    FEATURE_BASIC_INSIGHTS,
    FEATURE_MANUAL_ENTRY,
    // Finora Plus
    FEATURE_UNLIMITED_BUDGETS,
    FEATURE_SAVINGS_GOALS,
    FEATURE_ADVANCED_FILTERS,
    FEATURE_REPORTS,
    FEATURE_CSV_EXPORT,
    FEATURE_RECURRING,
    FEATURE_BETTER_INSIGHTS,
    // Finora Pro
    FEATURE_MULTIPLE_ACCOUNTS,
    FEATURE_ADVANCED_ANALYTICS,
    FEATURE_PDF_REPORTS,
    FEATURE_CUSTOM_CATEGORIES,
    FEATURE_NET_WORTH,
    FEATURE_CALENDAR,
    FEATURE_PLANNING,
    // Finora Business
    FEATURE_TEAM_MEMBERS,
    FEATURE_SHARED_WORKSPACES,
    FEATURE_ROLES,
    FEATURE_BUSINESS_CATEGORIES,
    FEATURE_TEAM_REPORTS,
    FEATURE_AUDIT_HISTORY,
    FEATURE_BUSINESS_OVERVIEW
};
const int FEATURE_COUNT = 28;

struct FeatureInfo {
    const char *name;     // shown on screen
    PlanId lowestPlan;    // the smallest plan that includes it
    FeatureState state;
};

// FEATURES[FEATURE_CSV_EXPORT] describes CSV exports: the position in the
// array is the Feature.
const FeatureInfo FEATURES[FEATURE_COUNT] = {
    {"Basic transactions",                   PLAN_FREE,     FEATURE_AVAILABLE},
    {"Dashboard and financial summaries",    PLAN_FREE,     FEATURE_AVAILABLE},
    {"PKR, USD and EUR",                     PLAN_FREE,     FEATURE_AVAILABLE},
    {"Monthly budgets (up to 3 active)",     PLAN_FREE,     FEATURE_AVAILABLE},
    {"Recent transaction history",           PLAN_FREE,     FEATURE_AVAILABLE},
    {"Basic insights and budget alerts",     PLAN_FREE,     FEATURE_AVAILABLE},
    {"Manual transaction entry",             PLAN_FREE,     FEATURE_AVAILABLE},
    {"Unlimited active budgets",             PLAN_PLUS,     FEATURE_AVAILABLE},
    {"Savings goals",                        PLAN_PLUS,     FEATURE_AVAILABLE},
    {"Advanced filters",                     PLAN_PLUS,     FEATURE_PLANNED},
    {"Monthly and yearly reports",           PLAN_PLUS,     FEATURE_PLANNED},
    {"CSV exports",                          PLAN_PLUS,     FEATURE_AVAILABLE},
    {"Recurring transactions",               PLAN_PLUS,     FEATURE_PLANNED},
    {"Better financial insights",            PLAN_PLUS,     FEATURE_PLANNED},
    {"Multiple accounts",                    PLAN_PRO,      FEATURE_PLANNED},
    {"Advanced analytics",                   PLAN_PRO,      FEATURE_PLANNED},
    {"PDF reports (monthly summary)",        PLAN_PRO,      FEATURE_AVAILABLE},
    {"Custom category lists",                PLAN_PRO,      FEATURE_PLANNED},
    {"Net worth tracking",                   PLAN_PRO,      FEATURE_PLANNED},
    {"Financial calendar",                   PLAN_PRO,      FEATURE_PLANNED},
    {"Advanced financial planning",          PLAN_PRO,      FEATURE_PLANNED},
    {"Team members",                         PLAN_BUSINESS, FEATURE_NEEDS_ONLINE},
    {"Shared workspaces",                    PLAN_BUSINESS, FEATURE_NEEDS_ONLINE},
    {"Role-based access",                    PLAN_BUSINESS, FEATURE_NEEDS_ONLINE},
    {"Business expense categories",          PLAN_BUSINESS, FEATURE_PLANNED},
    {"Team reports",                         PLAN_BUSINESS, FEATURE_NEEDS_ONLINE},
    {"Audit history",                        PLAN_BUSINESS, FEATURE_AVAILABLE},
    {"Business overview",                    PLAN_BUSINESS, FEATURE_PLANNED}
};

// ---- Functions ----------------------------------------------------------

// The database code of a plan: "pro" for PLAN_PRO.
std::string planCode(PlanId plan);

// Finds the plan with this code. Returns false for an unknown code, so a
// damaged database value is never mistaken for a real plan.
bool planFromCode(const std::string &code, PlanId &plan);

// "Finora Pro"
std::string planName(PlanId plan);

// The monthly price as shown on screen: "$12.99", or "$0" for Free.
std::string formatPlanPrice(PlanId plan);

// How many budgets may be active on a plan: FREE_ACTIVE_BUDGET_LIMIT on
// Free, UNLIMITED_BUDGETS on every paid plan.
int activeBudgetLimit(PlanId plan);

// True if `plan` includes `feature`: the plan is the feature's lowest
// plan or a larger one.
bool planIncludes(PlanId plan, Feature feature);

// True if the feature is built and working in this version of Finora.
bool featureAvailable(Feature feature);

// The feature's name: "CSV exports".
std::string featureName(Feature feature);

// Why a feature cannot be used on `plan`, in words for the screen, for
// example "\"CSV exports\" is part of Finora Plus. Your plan: Finora Free."
std::string lockedFeatureMessage(Feature feature, PlanId plan);

// Why no more budgets can be created, for example "Finora Free includes
// 3 active budgets. Upgrade to Finora Plus for unlimited budgets, or
// delete a budget to make room."
std::string budgetLimitMessage(PlanId plan);

#endif // FINORA_PLANS_H
