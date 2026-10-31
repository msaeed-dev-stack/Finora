// =====================================================================
//  SimulatedPaymentGateway.h
//  The payment provider of Finora's demonstration: it approves or
//  declines a payment on the spot and moves no money. It has no network
//  connection, no secret keys and never sees card details.
// =====================================================================
#ifndef FINORA_SIMULATEDPAYMENTGATEWAY_H
#define FINORA_SIMULATEDPAYMENTGATEWAY_H

#include <string>

#include "PaymentGateway.h"

// Inherits the interface of PaymentGateway and overrides every function
// (polymorphism: SubscriptionService only knows a PaymentGateway &).
class SimulatedPaymentGateway : public PaymentGateway
{
public:
    std::string providerName() const override;
    bool isSimulated() const override;

    // "SIM-" and 12 random characters from the recovery-code alphabet
    // (no 0, O, 1 or I), for example "SIM-7KQ4M2TX9RHC".
    std::string newReference() override;

    // Approves the payment, unless request.simulateDecline asks for a
    // refusal. A request without an amount, currency or reference is refused.
    PaymentResult charge(const PaymentRequest &request) override;

    // How many payments were asked for, for the tests.
    int chargeCount() const;

private:
    int charges = 0;
};

#endif // FINORA_SIMULATEDPAYMENTGATEWAY_H
