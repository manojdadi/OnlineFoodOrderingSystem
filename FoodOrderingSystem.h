// FoodOrderingSystem.h - the "brain" of the backend. It owns all the data and business rules.
// OOP concepts: Composition (it is built from Repository, User, MenuItem, Order objects),
//               Templates, Exception handling, File handling, STL (vector, map, shared_ptr)
#pragma once
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "Delivery.h"
#include "Exceptions.h"
#include "MenuItem.h"
#include "Order.h"
#include "Payment.h"
#include "Repository.h"
#include "User.h"
#include "Utils.h"

class FoodOrderingSystem {
    Repository<MenuItem> menu;                      // template class used with 3 different types
    Repository<User> users;
    Repository<Order> orders;
    std::map<std::string, int> sessions;            // login token -> user id
    std::string dataDir;

    // Admin password for the first run. Online, it is set secretly through the ADMIN_PASSWORD
    // variable so nobody can read it from the code. On a laptop the demo password is used.
    static std::string adminPassword() {
        const char* secret = std::getenv("ADMIN_PASSWORD");
        return (secret && *secret) ? secret : "admin123";
    }

    // ------------------------------------------------------------------ file handling
    void saveMenu() const {
        std::ofstream file(dataDir + "/menu.txt");
        for (const auto& item : menu.getAll()) file << item->toFileLine() << "\n";
    }

    void saveUsers() const {
        std::ofstream file(dataDir + "/users.txt");
        for (const auto& user : users.getAll()) file << user->toFileLine() << "\n";
    }

    void saveOrders() const {
        std::ofstream file(dataDir + "/orders.txt");
        for (const auto& order : orders.getAll()) file << order->toFileLine() << "\n";
    }

    void loadMenu() {
        std::ifstream file(dataDir + "/menu.txt");
        std::string line;
        while (std::getline(file, line)) {
            std::vector<std::string> f = util::split(line, '|');
            if (f.size() < 6) continue;
            try {
                int stock = f.size() >= 7 ? std::stoi(f[6]) : 25;
                menu.add(createMenuItem(f[1], std::stoi(f[0]), f[2], std::stod(f[3]), f[4], f[5] == "1", stock));
            } catch (const std::exception& e) {
                std::cerr << "Skipping bad menu record: " << e.what() << "\n";
            }
        }
    }

    void loadUsers() {
        std::ifstream file(dataDir + "/users.txt");
        std::string line;
        while (std::getline(file, line)) {
            std::vector<std::string> f = util::split(line, '|');
            try {
                if (f.size() >= 7 && f[1] == "customer")
                    users.add(std::make_shared<Customer>(std::stoi(f[0]), f[2], f[3], f[4], f[5], f[6]));
                else if (f.size() >= 5 && f[1] == "admin")
                    users.add(std::make_shared<Admin>(std::stoi(f[0]), f[2], f[3], f[4]));
            } catch (const std::exception& e) {
                std::cerr << "Skipping bad user record: " << e.what() << "\n";
            }
        }
    }

    void loadOrders() {
        std::ifstream file(dataDir + "/orders.txt");
        std::string line;
        while (std::getline(file, line)) {
            if (util::trim(line).empty()) continue;
            try {
                orders.add(std::make_shared<Order>(Order::fromFileLine(line)));
            } catch (const std::exception& e) {
                std::cerr << "Skipping bad order record: " << e.what() << "\n";
            }
        }
    }

    // First run: fill the menu and create the admin account
    void seedDefaults() {
        if (menu.isEmpty()) {
            struct Seed { const char* category; const char* name; double price; const char* description; };
            const Seed seeds[] = {
                {"Veg", "Paneer Butter Masala", 220, "Soft paneer cubes in a rich tomato-butter gravy"},
                {"Veg", "Veg Biryani", 180, "Basmati rice cooked with garden vegetables and spices"},
                {"Veg", "Masala Dosa", 90, "Crispy dosa with potato masala, sambar and chutney"},
                {"Veg", "Chole Bhature", 130, "Spicy chickpea curry with two fluffy bhature"},
                {"Veg", "Margherita Pizza", 249, "Classic cheese and tomato pizza with basil"},
                {"Non-Veg", "Chicken Biryani", 240, "Hyderabadi dum biryani with raita and salan"},
                {"Non-Veg", "Butter Chicken", 280, "Tandoori chicken in creamy makhani gravy"},
                {"Non-Veg", "Chicken 65", 210, "Spicy deep-fried chicken starter"},
                {"Non-Veg", "Egg Fried Rice", 150, "Wok-tossed rice with egg and spring onion"},
                {"Non-Veg", "Fish Fry", 260, "Andhra style masala fish fry"},
                {"Beverage", "Mango Lassi", 80, "Thick yoghurt drink with Alphonso mango"},
                {"Beverage", "Masala Chai", 30, "Hot tea brewed with ginger and cardamom"},
                {"Beverage", "Cold Coffee", 110, "Chilled coffee blended with ice cream"},
                {"Beverage", "Fresh Lime Soda", 60, "Sweet or salted lime soda"},
                {"Dessert", "Gulab Jamun", 70, "Two warm jamuns soaked in sugar syrup"},
                {"Dessert", "Chocolate Brownie", 120, "Fudgy brownie with chocolate sauce"},
                {"Dessert", "Rasmalai", 90, "Soft paneer discs in saffron milk"},
            };
            int id = 1;
            for (const Seed& s : seeds) menu.add(createMenuItem(s.category, id++, s.name, s.price, s.description));
            saveMenu();
        }
        // The 10-minute Instant Store (also added to an older menu file that does not have it yet)
        bool hasInstant = false;
        for (const auto& item : menu.getAll()) if (item->isInstant()) hasInstant = true;
        if (!hasInstant) {
            struct Snack { const char* name; double price; const char* description; };
            const Snack snacks[] = {
                {"Samosa", 40, "Two crispy potato samosas with chutney"},
                {"Vada Pav", 35, "Mumbai style vada pav with dry garlic chutney"},
                {"Veg Sandwich", 80, "Grilled vegetable sandwich with green chutney"},
                {"French Fries", 90, "Hot salted fries with ketchup"},
                {"Instant Noodles", 60, "Masala noodles, ready in two minutes"},
                {"Potato Chips", 30, "Classic salted potato chips"},
                {"Cold Drink", 40, "Chilled 300 ml can"},
                {"Chocolate Bar", 50, "Milk chocolate bar"},
            };
            for (const Snack& s : snacks)
                menu.add(createMenuItem("Instant", menu.getNextId(), s.name, s.price, s.description, true, 25));
            saveMenu();
        }
        bool hasAdmin = false;
        for (const auto& user : users.getAll()) if (user->getRole() == "admin") hasAdmin = true;
        if (!hasAdmin) {
            users.add(std::make_shared<Admin>(users.getNextId(), "Restaurant Admin", "admin@foodexpress.com",
                                              util::hashPassword(adminPassword())));
            saveUsers();
        }
    }

    std::shared_ptr<User> findUserByEmail(const std::string& email) const {
        for (const auto& user : users.getAll())
            if (util::toLower(user->getEmail()) == util::toLower(email)) return user;
        return nullptr;
    }

public:
    explicit FoodOrderingSystem(const std::string& dataDirectory = "data") : dataDir(dataDirectory) {
        std::filesystem::create_directories(dataDir);
        loadMenu();
        loadUsers();
        loadOrders();
        seedDefaults();
        std::cout << "Loaded " << menu.size() << " menu items, " << users.size() << " users, "
                  << orders.size() << " orders from '" << dataDir << "/'\n";
    }

    // ------------------------------------------------------------------ accounts
    std::shared_ptr<Customer> registerCustomer(const std::string& rawName, const std::string& rawEmail,
                                               const std::string& password, const std::string& rawPhone,
                                               const std::string& rawAddress) {
        std::string name = util::sanitize(rawName), email = util::sanitize(rawEmail);
        std::string phone = util::sanitize(rawPhone), address = util::sanitize(rawAddress);

        if (name.size() < 2) throw InvalidInputException("Please enter your name");
        size_t at = email.find('@');
        if (at == std::string::npos || at == 0 || email.find('.', at) == std::string::npos || email.find(' ') != std::string::npos)
            throw InvalidInputException("Please enter a valid email address");
        if (password.size() < 6) throw InvalidInputException("Password must have at least 6 characters");
        if (phone.size() != 10) throw InvalidInputException("Phone number must have 10 digits");
        for (char c : phone)
            if (!std::isdigit(static_cast<unsigned char>(c))) throw InvalidInputException("Phone number must have 10 digits");
        if (address.size() < 5) throw InvalidInputException("Please enter your delivery address");
        if (findUserByEmail(email)) throw InvalidInputException("An account with this email already exists");

        auto customer = std::make_shared<Customer>(users.getNextId(), name, email, util::hashPassword(password), phone, address);
        users.add(customer);
        saveUsers();
        return customer;
    }

    // Returns a session token that the browser sends with every later request
    std::string login(const std::string& email, const std::string& password) {
        std::shared_ptr<User> user = findUserByEmail(util::trim(email));
        if (!user || !user->checkPassword(password)) throw AuthException("Wrong email or password");
        std::string token = util::randomToken();
        sessions[token] = user->getId();
        return token;
    }

    void logout(const std::string& token) { sessions.erase(token); }

    std::shared_ptr<User> getUser(const std::string& token) const {
        auto it = sessions.find(token);
        if (it == sessions.end()) throw AuthException("Please log in first");
        std::shared_ptr<User> user = users.findById(it->second);
        if (!user) throw AuthException("Please log in first");
        return user;
    }

    // dynamic_pointer_cast = safe downcast from User to Customer (RTTI)
    std::shared_ptr<Customer> getCustomer(const std::string& token) const {
        std::shared_ptr<Customer> customer = std::dynamic_pointer_cast<Customer>(getUser(token));
        if (!customer) throw FoodException("Only customers can do this", 403);
        return customer;
    }

    void requireAdmin(const std::string& token) const {
        if (getUser(token)->getRole() != "admin") throw FoodException("Only the admin can do this", 403);
    }

    // ------------------------------------------------------------------ menu
    const std::vector<std::shared_ptr<MenuItem>>& getMenu() const { return menu.getAll(); }

    std::shared_ptr<MenuItem> getMenuItem(int itemId) const {
        std::shared_ptr<MenuItem> item = menu.findById(itemId);
        if (!item) throw NotFoundException("Menu item not found");
        return item;
    }

    std::shared_ptr<MenuItem> addMenuItem(const std::string& category, const std::string& name, double price,
                                          const std::string& description) {
        auto item = createMenuItem(category, menu.getNextId(), util::sanitize(name), price, util::sanitize(description));
        menu.add(item);
        saveMenu();
        return item;
    }

    void removeMenuItem(int itemId) {
        if (!menu.removeById(itemId)) throw NotFoundException("Menu item not found");
        saveMenu();
    }

    void setItemAvailability(int itemId, bool available) {
        getMenuItem(itemId)->setAvailable(available);
        saveMenu();
    }

    // ------------------------------------------------------------------ cart
    void addToCart(Customer& customer, int itemId, int quantity) {
        std::shared_ptr<MenuItem> item = getMenuItem(itemId);
        if (!item->isAvailable()) throw InvalidInputException(item->getName() + " is not available right now");
        if (item->getStock() == 0) throw InvalidInputException(item->getName() + " is out of stock");
        customer.getCart().addItem(itemId, quantity);
    }

    void updateCart(Customer& customer, int itemId, int quantity) {
        customer.getCart().setQuantity(itemId, quantity);
    }

    // Turns the cart (ids + quantities) into bill lines. Items that the admin removed
    // or marked unavailable are dropped from the cart.
    std::vector<OrderLine> buildCartLines(Customer& customer, std::vector<std::shared_ptr<MenuItem>>* items = nullptr) {
        std::vector<OrderLine> lines;
        std::vector<int> stale;
        for (const auto& entry : customer.getCart().getItems()) {
            std::shared_ptr<MenuItem> item = menu.findById(entry.first);
            if (!item || !item->isAvailable()) { stale.push_back(entry.first); continue; }
            lines.push_back({item->getName(), item->getPrice(), entry.second});
            if (items) items->push_back(item);
        }
        for (int id : stale) customer.getCart() -= id;          // overloaded operator
        return lines;
    }

    // Cart contents + the bill for the chosen delivery option + a quote for every delivery option
    std::string cartToJson(Customer& customer, std::string deliveryCode = "standard") {
        std::vector<std::shared_ptr<MenuItem>> items;
        std::vector<OrderLine> lines = buildCartLines(customer, &items);
        double subtotal = 0;
        int longestPrep = 0;
        std::string json = "{\"items\":[";
        for (size_t i = 0; i < lines.size(); i++) {
            subtotal += lines[i].getLineTotal();
            if (items[i]->getPrepTimeMinutes() > longestPrep) longestPrep = items[i]->getPrepTimeMinutes();
            if (i > 0) json += ",";
            json += "{\"itemId\":" + std::to_string(items[i]->getId()) + ",\"name\":" + util::jsonStr(lines[i].itemName) +
                    ",\"price\":" + util::money(lines[i].price) + ",\"quantity\":" + std::to_string(lines[i].quantity) +
                    ",\"lineTotal\":" + util::money(lines[i].getLineTotal()) +
                    ",\"instant\":" + (items[i]->isInstant() ? "true" : "false") +
                    ",\"image\":" + util::jsonStr(items[i]->getImage()) + "}";
        }
        int totalQuantity = customer.getCart().getTotalQuantity();

        // Ask every delivery class for its fee, time and whether it can carry this cart (polymorphism)
        std::string modesJson = "[";
        bool chosenIsPossible = false;
        const char* codes[] = {"standard", "instant"};
        for (int i = 0; i < 2; i++) {
            std::unique_ptr<DeliveryMode> mode = createDeliveryMode(codes[i]);
            bool possible = !lines.empty();
            std::string reason = lines.empty() ? "Your cart is empty" : "";
            try { mode->validate(items, totalQuantity); }
            catch (const FoodException& e) { possible = false; reason = e.what(); }
            if (possible && deliveryCode == codes[i]) chosenIsPossible = true;
            if (i > 0) modesJson += ",";
            modesJson += "{\"code\":" + util::jsonStr(mode->getCode()) + ",\"name\":" + util::jsonStr(mode->getName()) +
                         ",\"fee\":" + util::money(mode->calculateFee(subtotal)) +
                         ",\"eta\":" + std::to_string(mode->getEtaMinutes(longestPrep)) +
                         ",\"available\":" + (possible ? "true" : "false") + ",\"reason\":" + util::jsonStr(reason) + "}";
        }
        modesJson += "]";
        if (!chosenIsPossible) deliveryCode = "standard";

        std::unique_ptr<DeliveryMode> chosen = createDeliveryMode(deliveryCode);
        Bill bill = Order::calculateBill(subtotal, chosen->calculateFee(subtotal));
        json += "],\"count\":" + std::to_string(totalQuantity) +
                ",\"delivery\":" + util::jsonStr(chosen->getCode()) + ",\"modes\":" + modesJson +
                ",\"subtotal\":" + util::money(bill.subtotal) + ",\"tax\":" + util::money(bill.tax) +
                ",\"deliveryFee\":" + util::money(bill.deliveryFee) + ",\"total\":" + util::money(bill.total) + "}";
        return json;
    }

    // ------------------------------------------------------------------ orders
    std::shared_ptr<Order> placeOrder(Customer& customer, const std::string& paymentMethod,
                                      const std::string& paymentDetail, const std::string& deliveryCode = "standard") {
        std::vector<std::shared_ptr<MenuItem>> items;
        std::vector<OrderLine> lines = buildCartLines(customer, &items);
        if (lines.empty()) throw InvalidInputException("Your cart is empty");

        // Polymorphism: StandardDelivery and InstantDelivery have different rules, fees and times
        std::unique_ptr<DeliveryMode> delivery = createDeliveryMode(deliveryCode);
        delivery->validate(items, customer.getCart().getTotalQuantity());

        double subtotal = 0;
        int longestPrep = 0;
        for (size_t i = 0; i < lines.size(); i++) {
            int stock = items[i]->getStock();
            if (stock >= 0 && lines[i].quantity > stock)
                throw InvalidInputException("Only " + std::to_string(stock) + " x " + items[i]->getName() + " left in stock");
            subtotal += lines[i].getLineTotal();
            if (items[i]->getPrepTimeMinutes() > longestPrep) longestPrep = items[i]->getPrepTimeMinutes();
        }
        Bill bill = Order::calculateBill(subtotal, delivery->calculateFee(subtotal));

        // Polymorphism: we do not know (or care) which payment class this is
        std::unique_ptr<Payment> payment = createPayment(paymentMethod, bill.total, paymentDetail);
        std::string reference = payment->process();            // throws PaymentException if it fails

        auto order = std::make_shared<Order>(customer.getId(), customer.getName(), customer.getAddress(),
                                             lines, bill, payment->getMethod(), reference,
                                             delivery->getName(), delivery->isInstant(),
                                             delivery->getEtaMinutes(longestPrep));
        orders.add(order);
        saveOrders();
        for (size_t i = 0; i < lines.size(); i++) items[i]->changeStock(-lines[i].quantity);   // snacks leave the shelf
        saveMenu();
        customer.getCart().clear();
        std::cout << *order << std::flush;                      // overloaded operator<<
        return order;
    }

    // Moves Instant orders forward according to the time that has passed
    void refreshOrders() {
        bool changed = false;
        for (const auto& order : orders.getAll())
            if (order->refreshInstantStatus()) changed = true;
        if (changed) saveOrders();
    }

    void restockItem(int itemId, int quantity) {
        std::shared_ptr<MenuItem> item = getMenuItem(itemId);
        if (!item->isInstant()) throw InvalidInputException("Only Instant Store items have stock");
        if (quantity <= 0 || quantity > 500) throw InvalidInputException("Restock quantity must be between 1 and 500");
        item->changeStock(quantity);
        saveMenu();
    }

    std::vector<std::shared_ptr<Order>> getOrdersOf(int customerId) {
        refreshOrders();
        std::vector<std::shared_ptr<Order>> result;
        for (const auto& order : orders.getAll())
            if (order->getCustomerId() == customerId) result.push_back(order);
        return result;
    }

    const std::vector<std::shared_ptr<Order>>& getAllOrders() {
        refreshOrders();
        return orders.getAll();
    }

    void updateOrderStatus(int orderId, OrderStatus status) {
        std::shared_ptr<Order> order = orders.findById(orderId);
        if (!order) throw NotFoundException("Order not found");
        order->setStatus(status);
        saveOrders();
    }

    // A customer may cancel only their own order, and only before the kitchen starts
    void cancelOrder(const Customer& customer, int orderId) {
        std::shared_ptr<Order> order = orders.findById(orderId);
        if (!order || order->getCustomerId() != customer.getId()) throw NotFoundException("Order not found");
        order->refreshInstantStatus();
        if (order->getStatus() != OrderStatus::PLACED)
            throw InvalidInputException("The order can no longer be cancelled");
        order->setStatus(OrderStatus::CANCELLED);
        saveOrders();
    }

    // Numbers shown on the admin dashboard
    std::string statsToJson() const {
        double revenue = 0;
        int active = 0, instantOrders = 0;
        for (const auto& order : orders.getAll()) {
            if (order->isInstant()) instantOrders++;
            if (order->getStatus() != OrderStatus::CANCELLED) revenue += order->getTotal();
            if (order->getStatus() != OrderStatus::CANCELLED && order->getStatus() != OrderStatus::DELIVERED) active++;
        }
        int customers = 0;
        for (const auto& user : users.getAll()) if (user->getRole() == "customer") customers++;
        return "{\"orders\":" + std::to_string(orders.size()) + ",\"activeOrders\":" + std::to_string(active) +
               ",\"instantOrders\":" + std::to_string(instantOrders) +
               ",\"revenue\":" + util::money(revenue) + ",\"customers\":" + std::to_string(customers) +
               ",\"menuItems\":" + std::to_string(menu.size()) + "}";
    }
};
