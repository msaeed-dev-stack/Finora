// =====================================================================
//  SimulatedPaymentGateway.cpp
// =====================================================================
#include "SimulatedPaymentGateway.h"

#include <QRandomGenerator>

#include "core/AccountRules.h"   // RECOVERY_CODE_ALPHABET, RECOVERY_CODE_LENGTH

std::string SimulatedPaymentGateway::providerName() const
{
    return "Simulated payment (no real money)";
}

bool SimulatedPaymentGateway::isSimulated() const
{
    return true;
}

std::string SimulatedPaymentGateway::newReference()
{
    // QRandomGenerator::system() uses Windows' own secure random numbers.
    // A reference only has to be unique, not secret; the database's
    // UNIQUE rule refuses a repeat anyway.
    std::string reference = "SIM-";
    int alphabetSize = static_cast<int>(RECOVERY_CODE_ALPHABET.size());
    for (int i = 0; i < RECOVERY_CODE_LENGTH; i++) {
        int position = static_cast<int>(QRandomGenerator::system()->bounded(alphabetSize));
        reference += RECOVERY_CODE_ALPHABET[position];
    }
    return reference;
}

PaymentResult SimulatedPaymentGateway::charge(const PaymentRequest &request)
{
    charges++;
    PaymentResult result;
    if (request.amountMinor <= 0 || request.currencyCode.empty() || request.reference.empty()) {
        result.message = "The payment request was incomplete, so it was refused.";
        return result;
    }
    if (request.simulateDecline) {
        result.message = "The simulated payment was declined. No money was moved.";
        return result;
    }
    result.succeeded = true;   // simulated: nothing was charged anywhere
    return result;
}

int SimulatedPaymentGateway::chargeCount() const
{
    return charges;
}
