// main.cpp - program entry point. Connects the web addresses (API routes) to the C++ classes.
//
// Compile : g++ -std=c++17 -pthread backend/main.cpp -o server
// Run     : ./server (then open http://localhost:8080)

#include <iostream>
#include <string>
#include "FoodOrderingSystem.h"
#include "HttpServer.h"



// Creates an object of HttpResponse class to store the HTTP response.
static HttpResponse ok(const std::string& dataJson = "null") { //cls
    HttpResponse response;
    response.body = "{\"success\":true,\"data\":" + dataJson + "}";
    return response;
}

// FUNCTION TEMPLATE: listToJson
// T represents any class whose objects have a toJson() function.
// Uses shared_ptr to access objects dynamically.
template <class T>
static std::string listToJson(const std::vector<std::shared_ptr<T>>& list, bool newestFirst = false) {
    std::string json = "[";
    for (size_t i = 0; i < list.size(); i++) {
        if (i > 0) json += ",";
        json += list[newestFirst ? list.size() - 1 - i : i]->toJson();
    }
    return json + "]";
}


// Receives the HTTP request and extracts the price parameter.
static double priceParam(const HttpRequest& request) { //cls
    try { return std::stod(request.param("price")); }
    catch (const std::exception&) { throw InvalidInputException("Price must be a number"); }
}

// MAIN FUNCTION
int main(int argc, char* argv[]) {
    int port = 8080;
    bool publicAccess = false;

    // Hosting sites (like Render) tell the program which port to use through the PORT variable
    if (const char* hostPort = std::getenv("PORT")) {
        try { port = std::stoi(hostPort); publicAccess = true; }
        catch (const std::exception&) {}
    }

    if (argc > 1) {
        try { port = std::stoi(argv[1]); }
        catch (const std::exception&) { std::cerr << "Usage: ./server [port]\n"; return 1; }
    }

    try {
        
        // Creates the main application object and initializes the food ordering system.
        FoodOrderingSystem app("data"); //cls //cons

    
        // CONSTRUCTOR: HttpServer()
        // Creates the HTTP server object.
        HttpServer server; //cls 

        server.serveStaticFiles("frontend");

        // LAMBDA OBJECT: token
        // Receives an HttpRequest object and extracts the user's authentication token.
        auto token = [](const HttpRequest& r) { return r.header("x-token"); };

        // ---------------- public ----------------

        server.get("/api/menu", [&](const HttpRequest&) {
            return ok(listToJson(app.getMenu()));
        });

        server.post("/api/register", [&](const HttpRequest& r) {
            app.registerCustomer(r.param("name"), r.param("email"), r.param("password"), r.param("phone"), r.param("address"));
            std::string t = app.login(r.param("email"), r.param("password"));
            return ok("{\"token\":" + util::jsonStr(t) + ",\"user\":" + app.getUser(t)->toJson() + "}");
        });

        server.post("/api/login", [&](const HttpRequest& r) {
            std::string t = app.login(r.param("email"), r.param("password"));
            return ok("{\"token\":" + util::jsonStr(t) + ",\"user\":" + app.getUser(t)->toJson() + "}");
        });

        server.post("/api/logout", [&](const HttpRequest& r) {
            app.logout(token(r));
            return ok();
        });

        server.get("/api/me", [&](const HttpRequest& r) {
            return ok(app.getUser(token(r))->toJson());
        });

        // ---------------- customer: cart ----------------
        // "delivery" is the option picked on the cart page: standard or instant

        server.get("/api/cart", [&](const HttpRequest& r) {
            return ok(app.cartToJson(*app.getCustomer(token(r)), r.param("delivery")));
        });

        server.post("/api/cart/add", [&](const HttpRequest& r) {
            // OBJECT: customer
            // TYPE: shared_ptr<Customer>
            // Gets the customer object associated with the login token.
            auto customer = app.getCustomer(token(r));

            app.addToCart(*customer, r.intParam("itemId"), r.param("quantity").empty() ? 1 : r.intParam("quantity"));
            return ok(app.cartToJson(*customer, r.param("delivery")));
        });

        server.post("/api/cart/update", [&](const HttpRequest& r) {
            // OBJECT: customer
            // Gets the logged-in customer object.
            auto customer = app.getCustomer(token(r));

            app.updateCart(*customer, r.intParam("itemId"), r.intParam("quantity"));
            return ok(app.cartToJson(*customer, r.param("delivery")));
        });

        server.post("/api/cart/clear", [&](const HttpRequest& r) {
            // OBJECT: customer
            // Gets the logged-in customer object.
            auto customer = app.getCustomer(token(r));

            customer->getCart().clear();
            return ok(app.cartToJson(*customer));
        });

        // ---------------- customer: orders ----------------

        server.post("/api/orders/place", [&](const HttpRequest& r) {
            // OBJECT: customer
            // Gets the logged-in customer object.
            auto customer = app.getCustomer(token(r));

            return ok(app.placeOrder(*customer, r.param("method"), r.param("detail"), r.param("delivery"))->toJson());
        });

        server.get("/api/orders", [&](const HttpRequest& r) {
            return ok(listToJson(app.getOrdersOf(app.getCustomer(token(r))->getId()), true));
        });

        server.post("/api/orders/cancel", [&](const HttpRequest& r) {
            // OBJECT: customer
            // Gets the logged-in customer object.
            app.cancelOrder(*app.getCustomer(token(r)), r.intParam("orderId"));
            return ok();
        });

        // ---------------- admin ----------------

        server.get("/api/admin/orders", [&](const HttpRequest& r) {
            app.requireAdmin(token(r));
            return ok(listToJson(app.getAllOrders(), true));
        });

        server.get("/api/admin/stats", [&](const HttpRequest& r) {
            app.requireAdmin(token(r));
            return ok(app.statsToJson());
        });

        server.post("/api/admin/orders/status", [&](const HttpRequest& r) {
            app.requireAdmin(token(r));
            app.updateOrderStatus(r.intParam("orderId"), statusFromString(r.param("status")));
            return ok();
        });

        server.post("/api/admin/menu/add", [&](const HttpRequest& r) {
            app.requireAdmin(token(r));
            return ok(app.addMenuItem(r.param("category"), r.param("name"), priceParam(r), r.param("description"))->toJson());
        });

        server.post("/api/admin/menu/restock", [&](const HttpRequest& r) {
            app.requireAdmin(token(r));
            app.restockItem(r.intParam("itemId"), r.intParam("quantity"));
            return ok();
        });

        server.post("/api/admin/menu/remove", [&](const HttpRequest& r) {
            app.requireAdmin(token(r));
            app.removeMenuItem(r.intParam("itemId"));
            return ok();
        });

        server.post("/api/admin/menu/availability", [&](const HttpRequest& r) {
            app.requireAdmin(token(r));
            app.setItemAvailability(r.intParam("itemId"), r.param("available") == "true");
            return ok();
        });

        server.listen(port, publicAccess);

    } catch (const std::exception& e) {
        std::cerr << "Could not start: " << e.what() << "\n";
        return 1;
    }

    return 0;
}