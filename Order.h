// Order.h - one placed order
// OOP concepts: Class with static members, enum class, friend function / operator<< overloading
#pragma once
#include <ctime>
#include <iostream>
#include <string>
#include <vector>
#include "Exceptions.h"
#include "Utils.h"

enum class OrderStatus { PLACED, PREPARING, OUT_FOR_DELIVERY, DELIVERED, CANCELLED };

inline std::string statusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::PLACED: return "Placed";
        case OrderStatus::PREPARING: return "Preparing";
        case OrderStatus::OUT_FOR_DELIVERY: return "Out for Delivery";
        case OrderStatus::DELIVERED: return "Delivered";
        case OrderStatus::CANCELLED: return "Cancelled";
    }
    return "Placed";
}

inline OrderStatus statusFromString(const std::string& text) {
    if (text == "Placed") return OrderStatus::PLACED;
    if (text == "Preparing") return OrderStatus::PREPARING;
    if (text == "Out for Delivery") return OrderStatus::OUT_FOR_DELIVERY;
    if (text == "Delivered") return OrderStatus::DELIVERED;
    if (text == "Cancelled") return OrderStatus::CANCELLED;
    throw InvalidInputException("Unknown order status: " + text);
}

// One row of the bill. A copy of name and price is stored so that old orders
// do not change when the admin edits the menu later.
struct OrderLine {
    std::string itemName;
    double price;
    int quantity;
    double getLineTotal() const { return price * quantity; }
};

// Result of the bill calculation
struct Bill {
    double subtotal;
    double tax;
    double deliveryFee;
    double total;
};

class Order {
    int id;
    int customerId;
    std::string customerName;
    std::string address;
    std::vector<OrderLine> lines;
    Bill bill;
    std::string paymentMethod;
    std::string paymentRef;
    OrderStatus status;
    std::string placedAt;
    std::string deliveryMode;                  // "Standard Delivery" or "Instant Delivery"
    bool instant;
    int etaMinutes;                            // promised delivery time
    long long placedEpoch;                     // time of ordering in seconds, used for the countdown

    static int nextId;                         // shared by all Order objects

public:
    static constexpr double GST_RATE = 0.05;               // 5% GST

    // Bill = subtotal + 5% GST + delivery fee (the fee is decided by the DeliveryMode object)
    static Bill calculateBill(double subtotal, double deliveryFee) {
        Bill b;
        b.subtotal = subtotal;
        b.tax = subtotal * GST_RATE;
        b.deliveryFee = deliveryFee;
        b.total = b.subtotal + b.tax + b.deliveryFee;
        return b;
    }

    // Constructor for a NEW order (gets the next id automatically)
    Order(int customerId, const std::string& customerName, const std::string& address,
          const std::vector<OrderLine>& lines, const Bill& bill,
          const std::string& paymentMethod, const std::string& paymentRef,
          const std::string& deliveryMode = "Standard Delivery", bool instant = false, int etaMinutes = 45)
        : id(nextId++), customerId(customerId), customerName(customerName), address(address), lines(lines),
          bill(bill), paymentMethod(paymentMethod), paymentRef(paymentRef),
          status(OrderStatus::PLACED), placedAt(util::nowString()),
          deliveryMode(deliveryMode), instant(instant), etaMinutes(etaMinutes),
          placedEpoch(static_cast<long long>(std::time(nullptr))) {}

    int getId() const { return id; }
    int getCustomerId() const { return customerId; }
    OrderStatus getStatus() const { return status; }
    double getTotal() const { return bill.total; }
    bool isInstant() const { return instant; }

    // Seconds until the promised delivery time (0 when the time is over)
    long long getSecondsLeft() const {
        long long left = placedEpoch + etaMinutes * 60LL - static_cast<long long>(std::time(nullptr));
        return left > 0 ? left : 0;
    }

    // Instant orders move forward on their own: packed in 1 minute, on the road after
    // 3 minutes, delivered at the promised time. Returns true when the status changed.
    bool refreshInstantStatus() {
        if (!instant || status == OrderStatus::DELIVERED || status == OrderStatus::CANCELLED) return false;
        long long elapsed = static_cast<long long>(std::time(nullptr)) - placedEpoch;
        OrderStatus expected = OrderStatus::PLACED;
        if (elapsed >= etaMinutes * 60LL) expected = OrderStatus::DELIVERED;
        else if (elapsed >= 180) expected = OrderStatus::OUT_FOR_DELIVERY;
        else if (elapsed >= 60) expected = OrderStatus::PREPARING;
        if (static_cast<int>(expected) <= static_cast<int>(status)) return false;   // never move backwards
        status = expected;
        return true;
    }

    void setStatus(OrderStatus newStatus) {
        if (status == OrderStatus::DELIVERED || status == OrderStatus::CANCELLED)
            throw InvalidInputException("This order is already " + statusToString(status));
        status = newStatus;
    }

    std::string toJson() const {
        std::string json = "{\"id\":" + std::to_string(id) +
            ",\"customerId\":" + std::to_string(customerId) +
            ",\"customerName\":" + util::jsonStr(customerName) +
            ",\"address\":" + util::jsonStr(address) +
            ",\"status\":" + util::jsonStr(statusToString(status)) +
            ",\"paymentMethod\":" + util::jsonStr(paymentMethod) +
            ",\"paymentRef\":" + util::jsonStr(paymentRef) +
            ",\"placedAt\":" + util::jsonStr(placedAt) +
            ",\"deliveryMode\":" + util::jsonStr(deliveryMode) +
            ",\"instant\":" + (instant ? "true" : "false") +
            ",\"etaMinutes\":" + std::to_string(etaMinutes) +
            ",\"secondsLeft\":" + std::to_string(getSecondsLeft()) +
            ",\"subtotal\":" + util::money(bill.subtotal) +
            ",\"tax\":" + util::money(bill.tax) +
            ",\"deliveryFee\":" + util::money(bill.deliveryFee) +
            ",\"total\":" + util::money(bill.total) + ",\"items\":[";
        for (size_t i = 0; i < lines.size(); i++) {
            if (i > 0) json += ",";
            json += "{\"name\":" + util::jsonStr(lines[i].itemName) +
                    ",\"price\":" + util::money(lines[i].price) +
                    ",\"quantity\":" + std::to_string(lines[i].quantity) +
                    ",\"lineTotal\":" + util::money(lines[i].getLineTotal()) + "}";
        }
        return json + "]}";
    }

    // ---- File handling: one order = one line in data/orders.txt ----
    std::string toFileLine() const {
        std::string line = std::to_string(id) + "|" + std::to_string(customerId) + "|" + customerName + "|" +
            address + "|" + statusToString(status) + "|" + paymentMethod + "|" + paymentRef + "|" + placedAt + "|" +
            util::money(bill.subtotal) + "|" + util::money(bill.tax) + "|" + util::money(bill.deliveryFee) + "|" +
            util::money(bill.total) + "|";
        for (size_t i = 0; i < lines.size(); i++) {
            if (i > 0) line += ";";
            line += lines[i].itemName + "~" + util::money(lines[i].price) + "~" + std::to_string(lines[i].quantity);
        }
        return line + "|" + deliveryMode + "|" + (instant ? "1" : "0") + "|" + std::to_string(etaMinutes) + "|" +
               std::to_string(placedEpoch);
    }

    // Rebuild an order that was saved earlier
    static Order fromFileLine(const std::string& line) {
        std::vector<std::string> f = util::split(line, '|');
        if (f.size() < 13) throw InvalidInputException("Corrupt order record");
        std::vector<OrderLine> lines;
        for (const std::string& part : util::split(f[12], ';')) {
            std::vector<std::string> p = util::split(part, '~');
            if (p.size() == 3) lines.push_back({p[0], std::stod(p[1]), std::stoi(p[2])});
        }
        Bill bill{std::stod(f[8]), std::stod(f[9]), std::stod(f[10]), std::stod(f[11])};
        Order order(std::stoi(f[1]), f[2], f[3], lines, bill, f[5], f[6]);
        nextId--;                                   // the constructor consumed an id we do not need
        order.id = std::stoi(f[0]);
        order.status = statusFromString(f[4]);
        order.placedAt = f[7];
        if (f.size() >= 17) {                       // fields added with the Instant Delivery feature
            order.deliveryMode = f[13];
            order.instant = f[14] == "1";
            order.etaMinutes = std::stoi(f[15]);
            order.placedEpoch = std::stoll(f[16]);
        } else {
            order.placedEpoch = 0;                  // old record: delivery time is already over
        }
        if (order.id >= nextId) nextId = order.id + 1;
        return order;
    }

    // Friend function: prints a receipt on the server console with  cout << order
    friend std::ostream& operator<<(std::ostream& out, const Order& order) {
        out << "\n========== ORDER #" << order.id << " ==========\n"
            << "Customer : " << order.customerName << "\n"
            << "Address  : " << order.address << "\n"
            << "Time     : " << order.placedAt << "\n";
        for (const OrderLine& line : order.lines)
            out << "  " << line.quantity << " x " << line.itemName << "  = Rs. " << util::money(line.getLineTotal()) << "\n";
        out << "Subtotal : Rs. " << util::money(order.bill.subtotal) << "\n"
            << "GST (5%) : Rs. " << util::money(order.bill.tax) << "\n"
            << "Del. fee : Rs. " << util::money(order.bill.deliveryFee) << "\n"
            << "TOTAL    : Rs. " << util::money(order.bill.total) << "\n"
            << "Delivery : " << order.deliveryMode << " (about " << order.etaMinutes << " min)\n"
            << "Payment  : " << order.paymentMethod << " (" << order.paymentRef << ")\n"
            << "Status   : " << statusToString(order.status) << "\n"
            << "================================\n";
        return out;
    }
};

inline int Order::nextId = 1001;               // definition of the static member
