// Delivery.h - delivery options: normal restaurant delivery and 10-minute Instant delivery
// OOP concepts: Abstract class + runtime polymorphism. The system calls delivery->calculateFee()
//               and delivery->validate() without knowing which kind of delivery was chosen.
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Exceptions.h"
#include "MenuItem.h"

class DeliveryMode {
public:
    virtual ~DeliveryMode() {}

    virtual std::string getCode() const = 0;                       // short name used by the frontend
    virtual std::string getName() const = 0;                       // name shown on the bill
    virtual bool isInstant() const = 0;
    virtual double calculateFee(double subtotal) const = 0;
    virtual int getEtaMinutes(int longestPrepTime) const = 0;      // estimated delivery time

    // Checks whether this delivery option can carry the given cart.
    // Throws InvalidInputException with the reason when it cannot.
    virtual void validate(const std::vector<std::shared_ptr<MenuItem>>& items, int totalQuantity) const = 0;
};

// Food cooked in the kitchen and delivered by a rider
class StandardDelivery : public DeliveryMode {
public:
    static constexpr double FEE = 40.0;
    static constexpr double FREE_ABOVE = 499.0;

    std::string getCode() const override { return "standard"; }
    std::string getName() const override { return "Standard Delivery"; }
    bool isInstant() const override { return false; }
    double calculateFee(double subtotal) const override {
        return (subtotal == 0 || subtotal >= FREE_ABOVE) ? 0 : FEE;
    }
    int getEtaMinutes(int longestPrepTime) const override { return longestPrepTime + 20; }   // cooking + travel
    void validate(const std::vector<std::shared_ptr<MenuItem>>&, int) const override {}        // carries anything
};

// Ready-to-eat snacks from the Instant Store, delivered in 10 minutes
class InstantDelivery : public DeliveryMode {
public:
    static constexpr double FEE = 25.0;
    static constexpr double FREE_ABOVE = 199.0;
    static constexpr int ETA_MINUTES = 10;
    static constexpr int MAX_ITEMS = 12;                           // what fits in one delivery bag

    std::string getCode() const override { return "instant"; }
    std::string getName() const override { return "Instant Delivery"; }
    bool isInstant() const override { return true; }
    double calculateFee(double subtotal) const override {
        return (subtotal == 0 || subtotal >= FREE_ABOVE) ? 0 : FEE;
    }
    int getEtaMinutes(int) const override { return ETA_MINUTES; }
    void validate(const std::vector<std::shared_ptr<MenuItem>>& items, int totalQuantity) const override {
        for (const auto& item : items)
            if (!item->isInstant())
                throw InvalidInputException("10-minute delivery is only for Instant Store items. Remove " +
                                            item->getName() + " or choose Standard Delivery.");
        if (totalQuantity > MAX_ITEMS)
            throw InvalidInputException("Instant delivery can carry at most 12 items in one order.");
    }
};

// Factory: picks the right delivery class from the option chosen on the cart page
inline std::unique_ptr<DeliveryMode> createDeliveryMode(const std::string& code) {
    if (code == "instant") return std::unique_ptr<DeliveryMode>(new InstantDelivery());
    if (code == "standard" || code.empty()) return std::unique_ptr<DeliveryMode>(new StandardDelivery());
    throw InvalidInputException("Unknown delivery option: " + code);
}
