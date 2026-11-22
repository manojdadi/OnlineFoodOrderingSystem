// Payment.h - payment methods
// OOP concepts: Abstract class + runtime polymorphism. The system calls payment->process()
//               without knowing whether it is Cash, UPI or Card.
#pragma once
#include <memory>
#include <string>
#include "Exceptions.h"
#include "Utils.h"

class Payment {
protected:
    double amount;

public:
    explicit Payment(double amount) : amount(amount) {}
    virtual ~Payment() {}

    virtual std::string getMethod() const = 0;
    // Validates the details, "pays" and returns a reference number. Throws PaymentException on failure.
    virtual std::string process() = 0;
};

class CashOnDelivery : public Payment {
public:
    using Payment::Payment;
    std::string getMethod() const override { return "Cash on Delivery"; }
    std::string process() override {
        if (amount > 2000) throw PaymentException("Cash on Delivery is allowed only up to Rs. 2000");
        return "COD-PAY-ON-DELIVERY";
    }
};

class UpiPayment : public Payment {
    std::string upiId;
public:
    UpiPayment(double amount, const std::string& upiId) : Payment(amount), upiId(upiId) {}
    std::string getMethod() const override { return "UPI"; }
    std::string process() override {
        size_t at = upiId.find('@');
        if (at == std::string::npos || at == 0 || at == upiId.size() - 1)
            throw PaymentException("Enter a valid UPI ID (example: name@bank)");
        return "UPI-" + util::randomToken().substr(0, 10);
    }
};

class CardPayment : public Payment {
    std::string cardNumber;
public:
    CardPayment(double amount, const std::string& cardNumber) : Payment(amount) {
        for (char c : cardNumber) if (c != ' ' && c != '-') this->cardNumber += c;   // ignore spaces
    }
    std::string getMethod() const override { return "Card"; }
    std::string process() override {
        if (cardNumber.size() != 16) throw PaymentException("Card number must have 16 digits");
        for (char c : cardNumber)
            if (!std::isdigit(static_cast<unsigned char>(c))) throw PaymentException("Card number must contain only digits");
        // Only the last 4 digits are kept - the full number is never saved
        return "CARD-XXXX-" + cardNumber.substr(12);
    }
};

// Factory: picks the right payment class from the option chosen on the checkout page
inline std::unique_ptr<Payment> createPayment(const std::string& method, double amount, const std::string& detail) {
    if (method == "cod") return std::unique_ptr<Payment>(new CashOnDelivery(amount));
    if (method == "upi") return std::unique_ptr<Payment>(new UpiPayment(amount, detail));
    if (method == "card") return std::unique_ptr<Payment>(new CardPayment(amount, detail));
    throw PaymentException("Please choose a payment method");
}
