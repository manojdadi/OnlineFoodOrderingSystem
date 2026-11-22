// User.h - people who use the system
// OOP concepts: Abstract base class, Inheritance (Customer and Admin are Users),
//               Polymorphism (getRole), Composition (a Customer HAS-A Cart)
#pragma once
#include <string>
#include "Cart.h"
#include "Utils.h"

class User {
protected:
    int id;
    std::string name;
    std::string email;
    std::string passwordHash;                 // the real password is never stored

public:
    User(int id, const std::string& name, const std::string& email, const std::string& passwordHash)
        : id(id), name(name), email(email), passwordHash(passwordHash) {}
    virtual ~User() {}

    virtual std::string getRole() const = 0;  // pure virtual -> User is abstract

    int getId() const { return id; }
    std::string getName() const { return name; }
    std::string getEmail() const { return email; }
    bool checkPassword(const std::string& password) const {
        return passwordHash == util::hashPassword(password);
    }

    virtual std::string toJson() const {
        return "{\"id\":" + std::to_string(id) + ",\"name\":" + util::jsonStr(name) +
               ",\"email\":" + util::jsonStr(email) + ",\"role\":" + util::jsonStr(getRole()) + "}";
    }

    virtual std::string toFileLine() const {
        return std::to_string(id) + "|" + getRole() + "|" + name + "|" + email + "|" + passwordHash;
    }
};

class Customer : public User {
    std::string phone;
    std::string address;
    Cart cart;                                // composition

public:
    Customer(int id, const std::string& name, const std::string& email, const std::string& passwordHash,
             const std::string& phone, const std::string& address)
        : User(id, name, email, passwordHash), phone(phone), address(address) {}

    std::string getRole() const override { return "customer"; }
    std::string getPhone() const { return phone; }
    std::string getAddress() const { return address; }
    Cart& getCart() { return cart; }

    std::string toJson() const override {
        return "{\"id\":" + std::to_string(id) + ",\"name\":" + util::jsonStr(name) +
               ",\"email\":" + util::jsonStr(email) + ",\"role\":\"customer\"" +
               ",\"phone\":" + util::jsonStr(phone) + ",\"address\":" + util::jsonStr(address) + "}";
    }

    std::string toFileLine() const override {
        return User::toFileLine() + "|" + phone + "|" + address;
    }
};

class Admin : public User {
public:
    using User::User;
    std::string getRole() const override { return "admin"; }
};
