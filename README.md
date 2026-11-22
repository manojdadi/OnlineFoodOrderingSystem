# Online Food Ordering System (FoodExpress)

A mini project for **Object Oriented Programming with C++**.

- **Backend:** C++17, written fully with classes (no external libraries). It contains a small web server built with sockets.
- **Frontend:** HTML + CSS + JavaScript, shown in the browser.
- **Storage:** plain text files in the `data/` folder (created automatically on first run).

## What it can do

- **Kitchen menu** with photos, category filter and search.
- **Instant Store**: famous snacks (samosa, vada pav, sandwich, fries, noodles, chips) delivered in **10 minutes**, with limited stock.
- **Two delivery options** chosen in the cart: Standard Delivery, or Instant Delivery (only when every item in the cart is from the Instant Store, at most 12 items).
- **Live tracking**: an Instant order shows a countdown and moves from Placed to Packing to Out for Delivery to Delivered on its own.
- Cart, bill with 5% GST, payment by Cash / UPI / Card, order history and cancel.
- **Admin panel**: orders, revenue, status changes, add / hide / remove items, restock Instant items.

## How to run

You need a C++ compiler (`g++` or `clang++`).

**macOS / Linux**

```bash
./run.sh
```

or do the two steps yourself:

```bash
g++ -std=c++17 -pthread backend/main.cpp -o server
./server
```

**Windows (MinGW-w64 g++)** – the socket code has a Windows branch, but it has only been tested on macOS:

```bash
g++ -std=c++17 backend/main.cpp -o server.exe -lws2_32
server.exe
```

Then open **http://localhost:8080** in your browser. Stop the server with `Ctrl + C`.
If port 8080 is busy, pass another one: `./server 9090`.

## Accounts

| Role | How to log in |
|---|---|
| Customer | Click **Sign up** and create an account |
| Admin | Email `admin@foodexpress.com`, password `admin123` when running on your own computer (demo account created on first run). The online version uses a secret password set in Render instead. |

To start again with empty data, stop the server and delete the `data/` folder.

## Running it online (Render)

The project includes a `Dockerfile`, so it can be hosted for free on [Render](https://render.com) as a **Web Service** (runtime: Docker).

- Render passes the port in the `PORT` variable; the server then accepts connections from any device.
- Set an `ADMIN_PASSWORD` environment variable in Render so the online admin account does not use the demo password.
- Free plan: the site sleeps after 15 minutes without visitors (the next visit takes about a minute), and new accounts and orders are wiped whenever it restarts because data is kept in text files.
- Passwords use a simple hash meant for a classroom demo, so do not sign up with a real password.

## Demo script for the presentation (about 3 minutes)

1. Start the server and open the site. Show the menu, the category chips and the search box.
2. Sign up as a customer. In **Kitchen**, add 2–3 dishes, then switch to **Instant Store** and add a samosa.
3. Open **Cart**: Instant Delivery is greyed out with the reason (a kitchen dish is in the cart). This is `InstantDelivery::validate()` throwing an exception.
4. Choose **UPI**, type a wrong id (for example `abc`) to show the error, then a valid one (`name@bank`) and place the order with Standard Delivery.
5. Go back to **Instant Store**, add only snacks, open the cart: Instant Delivery (10 min) is now selected. Place the order.
6. **My Orders** shows the live countdown. Show the receipt printed in the terminal (`operator<<`) and the new line in `data/orders.txt` (file handling).
7. Log out, log in as admin. Show the dashboard, change the status of the first order, press **+10** to restock a snack.

## Folder structure

```
OnlineFoodOrderingSystem/
├── backend/
│   ├── main.cpp               entry point, connects web addresses (routes) to the classes
│   ├── FoodOrderingSystem.h   main controller: accounts, menu, cart, orders, file handling
│   ├── HttpServer.h           small socket-based web server
│   ├── MenuItem.h             abstract MenuItem + VegItem, NonVegItem, Beverage, Dessert, InstantItem
│   ├── User.h                 abstract User + Customer, Admin
│   ├── Cart.h                 shopping cart (operator += and -=)
│   ├── Order.h                Order, OrderLine, Bill, OrderStatus (operator <<)
│   ├── Payment.h              abstract Payment + CashOnDelivery, UpiPayment, CardPayment
│   ├── Delivery.h             abstract DeliveryMode + StandardDelivery, InstantDelivery
│   ├── Repository.h           template class Repository<T>
│   ├── Exceptions.h           FoodException and derived exception classes
│   └── Utils.h                helper functions (split, hash, JSON text)
├── frontend/
│   ├── index.html
│   ├── style.css
│   ├── app.js
│   └── images/                dish photos + CREDITS.md (photo sources and licences)
├── run.sh
└── README.md
```

## OOP concepts and where to find them (useful for the viva)

| Concept | Where |
|---|---|
| Class and object | every header file; objects are created in `FoodOrderingSystem.h` |
| Encapsulation | private/protected data with public getters and setters (`MenuItem`, `User`, `Cart`, `Order`) |
| Abstraction | abstract classes `MenuItem`, `User`, `Payment`, `DeliveryMode` (pure virtual functions) |
| Inheritance | `VegItem`/`NonVegItem`/`Beverage`/`Dessert` : `MenuItem`; `InstantItem` : `MenuItem`; `Customer`/`Admin` : `User`; payment classes : `Payment`; `StandardDelivery`/`InstantDelivery` : `DeliveryMode`; exception classes |
| Runtime polymorphism | `delivery->validate()` / `delivery->calculateFee()` and `payment->process()` in `placeOrder()`, `isInstant()` / `getStock()`, `getCategory()` inside `MenuItem::toJson()`, `getRole()` |
| Function overriding | every `override` function in the derived classes |
| Operator overloading | `Cart::operator+=`, `Cart::operator-=`, `operator<<` for `Order` |
| Friend function | `operator<<` in `Order.h` |
| Static members | `Order::nextId`, `Order::calculateBill()` |
| Constructors, initializer lists, virtual destructor | `MenuItem.h`, `User.h`, `Order.h` |
| Composition | `Customer` has a `Cart`; `Order` has `OrderLine`s; `FoodOrderingSystem` has repositories |
| Templates | `Repository<T>` and the function template `listToJson<T>` in `main.cpp` |
| Exception handling | `Exceptions.h`; `throw` in the classes, `try`/`catch` in `HttpServer::dispatch()` |
| File handling | `saveMenu/loadMenu`, `saveUsers/loadUsers`, `saveOrders/loadOrders` (`ifstream`, `ofstream`) |
| STL | `vector`, `map`, `string`, `shared_ptr`, `unique_ptr`, `function`, `thread`, `mutex` |
| RTTI | `dynamic_pointer_cast<Customer>` in `getCustomer()` |

## Notes

- **Photos:** a dish shows the picture `frontend/images/<name-in-small-letters-with-dashes>.jpg` (for example `masala-dosa.jpg`). If there is no file, a coloured tile is shown. To add a photo for a new item, save a JPG with that name in the folder.
- Photo sources and licences are listed in `frontend/images/CREDITS.md`.
- Payments are simulated. Do not type real card details.
- The server accepts connections only from the same computer (localhost).
