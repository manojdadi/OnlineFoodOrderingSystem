// MenuItem.h - menu item hierarchy
// OOP concepts: Abstraction (abstract class), Inheritance, Polymorphism (virtual functions),
//               Encapsulation (private/protected data with getters and setters)
#pragma once
#include <memory>
#include <string>
#include "Exceptions.h"
#include "Utils.h"

// Abstract base class - you cannot create a plain "MenuItem" object.
class MenuItem {
protected:
    int id;
    std::string name;
    double price;
    std::string description;
    bool available;

public:
    MenuItem(int id, const std::string& name, double price, const std::string& description, bool available = true)
        : id(id), name(name), price(price), description(description), available(available) {}

    virtual ~MenuItem() {}                              // virtual destructor

    // Pure virtual functions - every derived class MUST override these
    virtual std::string getCategory() const = 0;
    virtual int getPrepTimeMinutes() const = 0;
    virtual bool isVegetarian() const = 0;

    // Virtual functions with a default behaviour - only InstantItem overrides them
    virtual bool isInstant() const { return false; }     // sold in the 10-minute Instant Store?
    virtual int getStock() const { return -1; }          // -1 = cooked fresh, no stock limit
    virtual void changeStock(int) {}

    // Photo shown on the website, e.g. frontend/images/masala-dosa.jpg
    std::string getImage() const { return "images/" + util::slugify(name) + ".jpg"; }

    // Getters / setters (encapsulation)
    int getId() const { return id; }
    std::string getName() const { return name; }
    double getPrice() const { return price; }
    std::string getDescription() const { return description; }
    bool isAvailable() const { return available; }
    void setAvailable(bool value) { available = value; }
    void setPrice(double newPrice) {
        if (newPrice <= 0) throw InvalidInputException("Price must be greater than zero");
        price = newPrice;
    }

    // Used by the frontend
    std::string toJson() const {
        return "{\"id\":" + std::to_string(id) +
               ",\"name\":" + util::jsonStr(name) +
               ",\"category\":" + util::jsonStr(getCategory()) +      // polymorphic call
               ",\"price\":" + util::money(price) +
               ",\"description\":" + util::jsonStr(description) +
               ",\"available\":" + (available ? "true" : "false") +
               ",\"veg\":" + (isVegetarian() ? "true" : "false") +     // polymorphic call
               ",\"prepTime\":" + std::to_string(getPrepTimeMinutes()) +
               ",\"instant\":" + (isInstant() ? "true" : "false") +
               ",\"stock\":" + std::to_string(getStock()) +
               ",\"image\":" + util::jsonStr(getImage()) + "}";
    }

    // Used to save the item in data/menu.txt
    std::string toFileLine() const {
        return std::to_string(id) + "|" + getCategory() + "|" + name + "|" + util::money(price) + "|" +
               description + "|" + (available ? "1" : "0") + "|" + std::to_string(getStock());
    }
};

// ---------------- Derived classes ----------------
class VegItem : public MenuItem {
public:
    using MenuItem::MenuItem;                           // reuse the base constructor
    std::string getCategory() const override { return "Veg"; }
    int getPrepTimeMinutes() const override { return 20; }
    bool isVegetarian() const override { return true; }
};

class NonVegItem : public MenuItem {
public:
    using MenuItem::MenuItem;
    std::string getCategory() const override { return "Non-Veg"; }
    int getPrepTimeMinutes() const override { return 30; }
    bool isVegetarian() const override { return false; }
};

class Beverage : public MenuItem {
public:
    using MenuItem::MenuItem;
    std::string getCategory() const override { return "Beverage"; }
    int getPrepTimeMinutes() const override { return 5; }
    bool isVegetarian() const override { return true; }
};

class Dessert : public MenuItem {
public:
    using MenuItem::MenuItem;
    std::string getCategory() const override { return "Dessert"; }
    int getPrepTimeMinutes() const override { return 10; }
    bool isVegetarian() const override { return true; }
};

// Ready-to-eat snack kept in the Instant Store. It has a limited stock.
class InstantItem : public MenuItem {
    int stock;                                          // extra data member of the derived class
public:
    InstantItem(int id, const std::string& name, double price, const std::string& description,
                bool available = true, int stock = 25)
        : MenuItem(id, name, price, description, available), stock(stock < 0 ? 0 : stock) {}

    std::string getCategory() const override { return "Instant"; }
    int getPrepTimeMinutes() const override { return 2; }          // only needs packing
    bool isVegetarian() const override { return true; }
    bool isInstant() const override { return true; }
    int getStock() const override { return stock; }
    void changeStock(int difference) override {
        stock += difference;
        if (stock < 0) stock = 0;
    }
};

// Factory function: builds the right derived object from a category name.
// The caller only ever sees a MenuItem pointer (runtime polymorphism).
inline std::shared_ptr<MenuItem> createMenuItem(const std::string& category, int id, const std::string& name,
                                                double price, const std::string& description, bool available = true,
                                                int stock = 25) {
    if (name.empty()) throw InvalidInputException("Item name cannot be empty");
    if (price <= 0) throw InvalidInputException("Price must be greater than zero");
    if (category == "Veg") return std::make_shared<VegItem>(id, name, price, description, available);
    if (category == "Non-Veg") return std::make_shared<NonVegItem>(id, name, price, description, available);
    if (category == "Beverage") return std::make_shared<Beverage>(id, name, price, description, available);
    if (category == "Dessert") return std::make_shared<Dessert>(id, name, price, description, available);
    if (category == "Instant") return std::make_shared<InstantItem>(id, name, price, description, available, stock);
    throw InvalidInputException("Unknown category: " + category);
}
