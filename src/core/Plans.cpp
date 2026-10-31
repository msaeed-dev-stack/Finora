// =====================================================================
//  Plans.cpp
// =====================================================================
#include "Plans.h"

#include "Money.h"   // formatAmount

std::string planCode(PlanId plan)
{
    return PLANS[plan].code;
}

bool planFromCode(const std::string &code, PlanId &plan)
{
    // Look at every plan in turn; the position in PLANS is the PlanId.
    for (int i = 0; i < PLAN_COUNT; i++) {
        if (code == PLANS[i].code) {
            plan = static_cast<PlanId>(i);
            return true;
        }
    }
    return false;
}

std::string planName(PlanId plan)
{
    return PLANS[plan].name;
}

std::string formatPlanPrice(PlanId plan)
{
    if (PLANS[plan].priceMinor == 0) {
        return "$0";
    }
    return "$" + formatAmount(PLANS[plan].priceMinor);   // 1299 -> "$12.99"
}

int activeBudgetLimit(PlanId plan)
{
    if (planIncludes(plan, FEATURE_UNLIMITED_BUDGETS)) {
        return UNLIMITED_BUDGETS;
    }
    return FREE_ACTIVE_BUDGET_LIMIT;
}

bool planIncludes(PlanId plan, Feature feature)
{
    // The plans are in order, so a larger plan includes every smaller one.
    return plan >= FEATURES[feature].lowestPlan;
}

bool featureAvailable(Feature feature)
{
    return FEATURES[feature].state == FEATURE_AVAILABLE;
}

std::string featureName(Feature feature)
{
    return FEATURES[feature].name;
}

std::string lockedFeatureMessage(Feature feature, PlanId plan)
{
    // The name is quoted, so "is" is right for "CSV exports" and for "Audit history".
    std::string quotedName = "\"" + featureName(feature) + "\"";
    if (FEATURES[feature].state == FEATURE_NEEDS_ONLINE) {
        return quotedName + " will need Finora Online, which is planned. "
               "It is not available in this desktop version.";
    }
    if (FEATURES[feature].state == FEATURE_PLANNED) {
        return quotedName + " is planned and not available in this version of Finora yet.";
    }
    return quotedName + " is part of " + planName(FEATURES[feature].lowestPlan)
           + ". Your plan: " + planName(plan) + ".";
}

std::string budgetLimitMessage(PlanId plan)
{
    return planName(plan) + " includes " + std::to_string(FREE_ACTIVE_BUDGET_LIMIT)
           + " active budgets. Upgrade to " + planName(FEATURES[FEATURE_UNLIMITED_BUDGETS].lowestPlan)
           + " for unlimited budgets, or delete a budget to make room.";
}
