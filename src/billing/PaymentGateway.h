// =====================================================================
//  PaymentGateway.h
//  What every payment provider must offer Finora: an abstract class
//  (an interface). Finora has one implementation, SimulatedPaymentGateway,
//  which moves no money. A real provider would be another class with
//  the same functions, working through a secure server; the rest of
//  Finora would not change (see docs/DESIGN.md, section 12).
// =====================================================================
#ifndef FINORA_PAYMENTGATEWAY_H
#define FINORA_PAYMENTGATEWAY_H

#include <string>

// What to charge. There is deliberately no field for card details:
// Finora never collects them. A real provider shows its own secure
// payment page instead, outside Finora.
struct PaymentRequest {
    std::string reference;          // Finora's number for this payment, for example "SIM-7KQ4M2TX9RHC"
    std::string description;        // "Finora Pro, 1 month"
    long long amountMinor = 0;      // US cents: 1299 means $12.99
    std::string currencyCode;       // "USD"
    bool simulateDecline = false;   // read only by the simulated gateway, to show how a refusal is handled
};

// The provider's answer.
struct PaymentResult {
    bool succeeded = false;
    std::string message;            // why the payment failed; "" when it succeeded
};

class PaymentGateway
{
public:
    // A virtual destructor: deleting a gateway through a PaymentGateway
    // pointer also runs the destructor of the real class.
    virtual ~PaymentGateway() = default;

    // Pure virtual functions (= 0): PaymentGateway has no code for them,
    // so no object of it can exist. Every real gateway must override all four.

    // A name for the screen, for example "Simulated payment".
    virtual std::string providerName() const = 0;

    // True when no real money can ever move.
    virtual bool isSimulated() const = 0;

    // A new, unique reference for the next payment.
    virtual std::string newReference() = 0;

    // Asks the provider to take the payment.
    virtual PaymentResult charge(const PaymentRequest &request) = 0;
};

#endif // FINORA_PAYMENTGATEWAY_H
