// Cart.h - shopping cart of one customer
// OOP concepts: Encapsulation, Operator Overloading, STL map
#pragma once
#include <map>
#include "Exceptions.h"

class Cart {
    std::map<int, int> items;                 // item id -> quantity
    static const int MAX_QUANTITY = 20;

public:
    void addItem(int itemId, int quantity = 1) {
        if (quantity <= 0) throw InvalidInputException("Quantity must be at least 1");
        if (items[itemId] + quantity > MAX_QUANTITY)
            throw InvalidInputException("You can order at most 20 of one item");
        items[itemId] += quantity;
    }

    void setQuantity(int itemId, int quantity) {
        if (quantity <= 0) { items.erase(itemId); return; }
        if (quantity > MAX_QUANTITY) throw InvalidInputException("You can order at most 20 of one item");
        items[itemId] = quantity;
    }

    void removeItem(int itemId) { items.erase(itemId); }
    void clear() { items.clear(); }
    bool isEmpty() const { return items.empty(); }
    const std::map<int, int>& getItems() const { return items; }

    int getTotalQuantity() const {
        int total = 0;
        for (const auto& entry : items) total += entry.second;
        return total;
    }

    // Operator overloading:  cart += itemId  adds one piece,  cart -= itemId  removes the item
    Cart& operator+=(int itemId) { addItem(itemId, 1); return *this; }
    Cart& operator-=(int itemId) { removeItem(itemId); return *this; }
};
