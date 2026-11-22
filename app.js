// FoodExpress frontend. It only draws the pages - every rule (prices, bill, delivery fee, stock,
// payment, order status) is decided by the C++ backend, which this file calls through /api/... addresses.

const EMPTY_CART = { items: [], count: 0, subtotal: 0, tax: 0, deliveryFee: 0, total: 0, delivery: "standard", modes: [] };

const state = {
  token: sessionStorage.getItem("token") || "",
  user: null,
  menu: [],
  cart: EMPTY_CART,
  store: sessionStorage.getItem("store") || "kitchen",   // "kitchen" or "instant"
  delivery: "standard",                                   // delivery option chosen on the cart page
  category: "All",
  search: "",
};

const app = document.getElementById("app");
const nav = document.getElementById("nav");
const cartbar = document.getElementById("cartbar");
const ICONS = { "Veg": "\u{1F957}", "Non-Veg": "\u{1F357}", "Beverage": "\u{1F964}", "Dessert": "\u{1F370}", "Instant": "⚡" };
const STATUS_FLOW = ["Placed", "Preparing", "Out for Delivery", "Delivered"];
let ticker = null;                                        // timer used by the live countdown

// ---------- helpers ----------
const rupees = (n) => "₹" + Number(n).toFixed(Number(n) % 1 === 0 ? 0 : 2);

// Always escape text before putting it into the page (prevents HTML injection)
function esc(text) {
  return String(text).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
}

let toastTimer;
function toast(message, isError = false) {
  const el = document.getElementById("toast");
  el.textContent = message;
  el.className = "toast show" + (isError ? " error" : "");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => (el.className = "toast"), 2800);
}

// Talks to the C++ server. Throws an Error with the server's message when something fails.
async function api(method, path, data) {
  const options = { method, headers: { "X-Token": state.token } };
  if (data) {
    options.headers["Content-Type"] = "application/x-www-form-urlencoded";
    options.body = new URLSearchParams(data).toString();
  }
  let reply;
  try {
    const response = await fetch(path, options);
    reply = await response.json();
  } catch (e) {
    throw new Error("Cannot reach the server. Is ./server running?");
  }
  if (!reply.success) throw new Error(reply.message);
  return reply.data;
}

// Runs an action and shows the error as a toast if it fails
async function attempt(action) {
  try { await action(); } catch (e) { toast(e.message, true); }
}

function go(page) { location.hash = page; }
const isCustomer = () => state.user && state.user.role === "customer";
const isAdmin = () => state.user && state.user.role === "admin";
const loadCart = async () => (state.cart = await api("GET", "/api/cart?delivery=" + state.delivery));

// A dish photo. If the picture file is missing, a coloured tile with an emoji is shown instead.
function photo(item, cssClass = "photo") {
  return `<div class="${cssClass}" data-emoji="${ICONS[item.category] || (item.instant ? ICONS.Instant : "\u{1F37D}")}">
            <img src="${esc(item.image)}" alt="${esc(item.name)}" loading="lazy"></div>`;
}
function watchPhotos() {
  app.querySelectorAll(".photo img, .thumb img").forEach((img) => img.addEventListener("error", () => {
    const box = img.parentElement;
    img.remove();
    if (box.classList.contains("thumb")) box.textContent = box.dataset.emoji; else box.classList.add("noimg");
  }));
}

// ---------- navigation bar + floating cart bar ----------
function renderNav() {
  const page = location.hash.slice(1) || "menu";
  const link = (target, label) => `<a href="#${target}" class="${page === target ? "active" : ""}">${label}</a>`;
  let html = link("menu", "Menu");
  if (!state.user) {
    html += link("login", "Login") + link("register", "Sign up");
  } else if (isAdmin()) {
    html += link("admin", "Admin Panel") + `<span class="hello">${esc(state.user.name)}</span><button id="logout">Logout</button>`;
  } else {
    html += link("cart", `Cart <span class="badge">${state.cart.count}</span>`) + link("orders", "My Orders") +
            `<span class="hello">Hi, ${esc(state.user.name.split(" ")[0])}</span><button id="logout">Logout</button>`;
  }
  nav.innerHTML = html;
  const logout = document.getElementById("logout");
  if (logout) logout.onclick = () => attempt(async () => {
    await api("POST", "/api/logout");
    setSession("", null);
    toast("Logged out");
    go("menu"); render();
  });

  document.body.classList.toggle("instant", state.store === "instant" && page === "menu");
  const showBar = isCustomer() && state.cart.count > 0 && page === "menu";
  cartbar.hidden = !showBar;
  if (showBar) cartbar.innerHTML = `<span>${state.cart.count} item${state.cart.count > 1 ? "s" : ""} in cart<small>${rupees(state.cart.subtotal)} + taxes</small></span><span class="go">View cart &rarr;</span>`;
}

function setSession(token, user) {
  state.token = token;
  state.user = user;
  if (token) sessionStorage.setItem("token", token); else sessionStorage.removeItem("token");
  if (!user) state.cart = EMPTY_CART;
}

// ---------- page: menu (Kitchen + Instant Store) ----------
function renderMenu() {
  const instant = state.store === "instant";
  const pool = state.menu.filter((item) => item.instant === instant);
  const categories = ["All", ...new Set(pool.map((item) => item.category))];
  if (!categories.includes(state.category)) state.category = "All";
  const term = state.search.toLowerCase();
  const items = pool.filter((item) =>
    (state.category === "All" || item.category === state.category) &&
    (item.name.toLowerCase().includes(term) || item.description.toLowerCase().includes(term)));
  const inCart = {};
  state.cart.items.forEach((line) => (inCart[line.itemId] = line.quantity));
  const tickerText = `<b>&#9889; 10-MINUTE INSTANT DELIVERY</b><span>FRESH FROM THE KITCHEN</span><b>FREE DELIVERY ABOVE ${rupees(499)}</b><span>PAY BY UPI, CARD OR CASH</span><b>LIVE ORDER TRACKING</b><span>BUILT WITH C++</span>`;

  app.innerHTML = `
    <section class="hero">
      <div>
        <span class="eyebrow"><i></i> Kitchen open &middot; delivering now</span>
        <h1>Hot meals in 40.<br>Snacks in <em>10&nbsp;minutes.</em></h1>
        <p>Order freshly cooked food from our kitchen, or grab famous snacks from the Instant Store and get them at your door in ten minutes.</p>
        <div class="search"><input id="search" type="search" placeholder="Search biryani, dosa, samosa..." value="${esc(state.search)}" aria-label="Search menu"></div>
      </div>
      <div class="collage" aria-hidden="true">
        <figure class="c1"><img src="images/chicken-biryani.jpg" alt=""></figure>
        <figure class="c2"><img src="images/masala-dosa.jpg" alt=""></figure>
        <figure class="c3"><img src="images/samosa.jpg" alt=""></figure>
        <span class="sticker s1">&#9889; 10 min delivery</span>
        <span class="sticker s2">Free delivery over ${rupees(499)}</span>
      </div>
    </section>

    <div class="ticker" aria-hidden="true"><div>${tickerText}${tickerText}</div></div>

    <div class="stores">
      <button class="store kitchen ${instant ? "" : "active"}" data-store="kitchen">
        <span class="icon">&#127858;</span>
        <span><b>Kitchen</b><span>Freshly cooked meals, drinks and desserts</span></span>
        <span class="eta">30&ndash;50 min</span>
      </button>
      <button class="store quick ${instant ? "active" : ""}" data-store="instant">
        <span class="icon">&#9889;</span>
        <span><b>Instant Store</b><span>Famous snacks, packed and delivered fast</span></span>
        <span class="eta">10 min</span>
      </button>
    </div>

    <div class="section-head">
      <h2>${instant ? "Snacks at your door in 10 minutes" : "Today's menu"}</h2>
      <span>${items.length} item${items.length === 1 ? "" : "s"}</span>
    </div>
    ${instant ? "" : `<div class="chips">${categories.map((c) => `<button class="chip ${c === state.category ? "active" : ""}" data-category="${esc(c)}">${esc(c)}</button>`).join("")}</div>`}

    ${items.length === 0 ? `<div class="empty">No items match your search.</div>` : `
    <div class="grid">
      ${items.map((item, index) => {
        const soldOut = !item.available || item.stock === 0;
        const quantity = inCart[item.id] || 0;
        return `
        <article class="card ${soldOut ? "off" : ""}" style="animation-delay:${Math.min(index, 10) * 40}ms">
          ${photo(item)}
          ${item.instant ? `<span class="tag bolt">&#9889; 10 min</span>` : `<span class="tag">${item.prepTime + 20} min</span>`}
          ${item.instant && item.stock > 0 && item.stock <= 5 ? `<span class="tag right warn">Only ${item.stock} left</span>` : ""}
          <div class="card-body">
            <div class="card-title"><span class="dot ${item.veg ? "" : "nonveg"}" title="${item.veg ? "Vegetarian" : "Non-vegetarian"}"></span>${esc(item.name)}</div>
            <div class="desc">${esc(item.description)}</div>
            <div class="card-foot">
              <span class="price">${rupees(item.price)}</span>
              ${soldOut ? `<span class="soldout">${item.stock === 0 ? "Out of stock" : "Not available"}</span>`
                : isAdmin() ? ""
                : quantity > 0 ? `<span class="qty"><button data-set="${item.id}" data-value="${quantity - 1}" aria-label="Remove one ${esc(item.name)}">&minus;</button><span>${quantity}</span><button data-set="${item.id}" data-value="${quantity + 1}" aria-label="Add one more ${esc(item.name)}">+</button></span>`
                : `<button class="btn small" data-add="${item.id}">Add +</button>`}
            </div>
          </div>
        </article>`; }).join("")}
    </div>`}`;

  watchPhotos();
  const search = document.getElementById("search");
  search.oninput = () => {
    state.search = search.value;
    const scroll = window.scrollY;
    renderMenu();
    window.scrollTo(0, scroll);
    const again = document.getElementById("search");      // keep the cursor in the box while typing
    again.focus(); again.setSelectionRange(again.value.length, again.value.length);
  };
  const redraw = () => { const scroll = window.scrollY; renderNav(); renderMenu(); window.scrollTo(0, scroll); };
  app.querySelectorAll("[data-store]").forEach((button) => (button.onclick = () => {
    state.store = button.dataset.store;
    sessionStorage.setItem("store", state.store);
    state.category = "All";
    redraw();
  }));
  app.querySelectorAll("[data-category]").forEach((chip) => (chip.onclick = () => { state.category = chip.dataset.category; redraw(); }));
  app.querySelectorAll("[data-add]").forEach((button) => (button.onclick = () => attempt(async () => {
    if (!state.user) { toast("Please log in to add items"); go("login"); return; }
    state.cart = await api("POST", "/api/cart/add", { itemId: button.dataset.add, quantity: 1, delivery: state.delivery });
    redraw();
  })));
  app.querySelectorAll("[data-set]").forEach((button) => (button.onclick = () => attempt(async () => {
    state.cart = await api("POST", "/api/cart/update", { itemId: button.dataset.set, quantity: button.dataset.value, delivery: state.delivery });
    redraw();
  })));
}

// ---------- pages: login / register ----------
function renderLogin() {
  app.innerHTML = `
    <div class="panel auth">
      <h2>Welcome back</h2>
      <form id="form">
        <label for="email">Email</label><input id="email" type="email" required autocomplete="username">
        <label for="password">Password</label><input id="password" type="password" required autocomplete="current-password">
        <button class="btn block" style="margin-top:22px">Login</button>
      </form>
      <div class="switch">New here? <a href="#register">Create an account</a></div>
      <div class="demo-box">Restaurant staff: use the admin account listed in README.md to open the Admin Panel.</div>
    </div>`;
  document.getElementById("form").onsubmit = (event) => {
    event.preventDefault();
    const f = event.target.elements;
    attempt(async () => {
      const result = await api("POST", "/api/login", { email: f.email.value, password: f.password.value });
      setSession(result.token, result.user);
      toast("Welcome, " + result.user.name);
      if (result.user.role === "admin") go("admin");
      else { await loadCart(); go("menu"); }
      render();
    });
  };
}

function renderRegister() {
  app.innerHTML = `
    <div class="panel auth">
      <h2>Create your account</h2>
      <form id="form">
        <label for="name">Full name</label><input id="name" required>
        <label for="email">Email</label><input id="email" type="email" required autocomplete="username">
        <label for="password">Password (at least 6 characters)</label><input id="password" type="password" required minlength="6" autocomplete="new-password">
        <label for="phone">Phone (10 digits)</label><input id="phone" inputmode="numeric" required maxlength="10">
        <label for="address">Delivery address</label><textarea id="address" rows="2" required></textarea>
        <button class="btn block" style="margin-top:22px">Sign up</button>
      </form>
      <div class="switch">Already registered? <a href="#login">Login</a></div>
    </div>`;
  document.getElementById("form").onsubmit = (event) => {
    event.preventDefault();
    const f = event.target.elements;
    attempt(async () => {
      const result = await api("POST", "/api/register", {
        name: f.name.value, email: f.email.value, password: f.password.value, phone: f.phone.value, address: f.address.value });
      setSession(result.token, result.user);
      toast("Account created. Welcome!");
      go("menu"); render();
    });
  };
}

// ---------- page: cart + checkout ----------
async function renderCart() {
  await loadCart();
  state.delivery = state.cart.delivery;                   // the server falls back to standard when instant is not possible
  renderNav();
  const cart = state.cart;
  if (cart.items.length === 0) {
    app.innerHTML = `<h1 class="page-title">Your cart</h1><div class="empty">Your cart is empty.<br><br><a class="btn" href="#menu">Browse the menu</a></div>`;
    return;
  }
  const chosen = cart.modes.find((mode) => mode.code === cart.delivery);
  app.innerHTML = `
    <h1 class="page-title">Your cart</h1>
    <div class="two-col">
      <div class="panel">
        ${cart.items.map((line) => `
          <div class="row">
            <div class="line-info">
              <div class="thumb" data-emoji="${line.instant ? ICONS.Instant : "\u{1F37D}"}"><img src="${esc(line.image)}" alt=""></div>
              <div><b>${esc(line.name)}</b> ${line.instant ? `<span class="mini">&#9889; INSTANT</span>` : ""}<div class="meta">${rupees(line.price)} each</div></div>
            </div>
            <div style="display:flex;align-items:center;gap:16px">
              <div class="qty">
                <button data-qty="${line.itemId}" data-value="${line.quantity - 1}" aria-label="Decrease">&minus;</button>
                <span>${line.quantity}</span>
                <button data-qty="${line.itemId}" data-value="${line.quantity + 1}" aria-label="Increase">+</button>
              </div>
              <b style="min-width:76px;text-align:right">${rupees(line.lineTotal)}</b>
            </div>
          </div>`).join("")}
        <div style="margin-top:16px"><button class="btn danger small" id="clear">Empty cart</button></div>
      </div>
      <div class="panel">
        <h3>Delivery</h3>
        <div class="options">
          ${cart.modes.map((mode) => `
            <button class="option ${mode.code === cart.delivery ? "active" : ""}" data-delivery="${mode.code}" ${mode.available ? "" : "disabled"}>
              <span class="icon">${mode.code === "instant" ? "&#9889;" : "&#128757;"}</span>
              <span><b>${esc(mode.name)} &middot; ${mode.eta} min</b><small>${mode.available ? (mode.code === "instant" ? "Packed from the Instant Store" : "Cooked fresh, then delivered") : esc(mode.reason)}</small></span>
              <span class="fee">${mode.fee > 0 ? rupees(mode.fee) : "FREE"}</span>
            </button>`).join("")}
        </div>
        <div class="bill">
          <div><span>Item total</span><span>${rupees(cart.subtotal)}</span></div>
          <div><span>GST (5%)</span><span>${rupees(cart.tax)}</span></div>
          <div><span>${esc(chosen.name)}</span><span>${cart.deliveryFee > 0 ? rupees(cart.deliveryFee) : "FREE"}</span></div>
          <div class="total"><span>To pay</span><span>${rupees(cart.total)}</span></div>
        </div>
        <div class="note">Delivering to: ${esc(state.user.address)}</div>
        <form id="checkout">
          <label for="method">Payment method</label>
          <select id="method">
            <option value="cod">Cash on Delivery</option>
            <option value="upi">UPI</option>
            <option value="card">Credit / Debit Card</option>
          </select>
          <div id="detail-box"></div>
          <button class="btn block" style="margin-top:20px">Place order &middot; ${rupees(cart.total)}</button>
          <div class="note">This is a college project. Payments are simulated - never enter real card details.</div>
        </form>
      </div>
    </div>`;

  watchPhotos();
  const method = document.getElementById("method");
  const detailBox = document.getElementById("detail-box");
  method.onchange = () => {
    detailBox.innerHTML =
      method.value === "upi" ? `<label for="detail">UPI ID</label><input id="detail" placeholder="name@bank" required>` :
      method.value === "card" ? `<label for="detail">Card number (use a test number)</label><input id="detail" inputmode="numeric" placeholder="4111 1111 1111 1111" required>` : "";
  };
  app.querySelectorAll("[data-delivery]").forEach((button) => (button.onclick = () => attempt(async () => {
    state.delivery = button.dataset.delivery;
    await renderCart();
  })));
  app.querySelectorAll("[data-qty]").forEach((button) => (button.onclick = () => attempt(async () => {
    await api("POST", "/api/cart/update", { itemId: button.dataset.qty, quantity: button.dataset.value });
    await renderCart();
  })));
  document.getElementById("clear").onclick = () => attempt(async () => { await api("POST", "/api/cart/clear"); await renderCart(); });
  document.getElementById("checkout").onsubmit = (event) => {
    event.preventDefault();
    const detail = document.getElementById("detail");
    attempt(async () => {
      const order = await api("POST", "/api/orders/place", { method: method.value, detail: detail ? detail.value : "", delivery: state.delivery });
      toast(order.instant ? `Order #${order.id} placed - arriving in ${order.etaMinutes} minutes!` : `Order #${order.id} placed!`);
      state.delivery = "standard";
      go("orders");
    });
  };
}

// ---------- page: my orders ----------
const clock = (seconds) => String(Math.floor(seconds / 60)).padStart(2, "0") + ":" + String(seconds % 60).padStart(2, "0");

function orderCard(order, extra = "") {
  const step = STATUS_FLOW.indexOf(order.status);
  const live = order.instant && order.status !== "Delivered" && order.status !== "Cancelled";
  const total = order.etaMinutes * 60;
  const done = Math.min(100, Math.round(((total - order.secondsLeft) / total) * 100));
  return `
    <div class="panel order">
      <div class="order-head">
        <div><b>Order #${order.id}</b> ${order.instant ? `<span class="mini">&#9889; INSTANT</span>` : ""} <span class="meta">&middot; ${esc(order.placedAt)} &middot; ${esc(order.deliveryMode)}</span></div>
        <span class="status ${esc(order.status.split(" ")[0])}">${esc(order.status)}</span>
      </div>
      ${live ? `
      <div class="countdown" data-left="${order.secondsLeft}" data-total="${total}">
        <div><div class="meta">Arriving in</div><div class="clock">${clock(order.secondsLeft)}</div></div>
        <div class="road"><i style="width:${done}%"></i><em style="left:${done}%">&#128757;</em></div>
      </div>` : ""}
      ${order.status === "Cancelled" ? "" : `
      <div class="steps">${STATUS_FLOW.map((s, i) => `<span class="${i <= step ? "done" : ""}"></span>`).join("")}</div>
      <div class="step-names">${STATUS_FLOW.map((s) => `<span>${order.instant && s === "Preparing" ? "Packing" : s}</span>`).join("")}</div>`}
      ${order.items.map((line) => `<div class="row"><span>${line.quantity} &times; ${esc(line.name)}</span><span>${rupees(line.lineTotal)}</span></div>`).join("")}
      <div class="row"><span class="meta">GST ${rupees(order.tax)} &middot; Delivery ${order.deliveryFee > 0 ? rupees(order.deliveryFee) : "FREE"} &middot; ${esc(order.paymentMethod)}</span><b>Total ${rupees(order.total)}</b></div>
      ${extra}
    </div>`;
}

async function renderOrders() {
  const orders = await api("GET", "/api/orders");
  await loadCart();
  renderNav();
  app.innerHTML = `<h1 class="page-title">My orders</h1>` + (orders.length === 0
    ? `<div class="empty">You have not ordered anything yet.<br><br><a class="btn" href="#menu">Order now</a></div>`
    : orders.map((order) => orderCard(order, order.status === "Placed"
        ? `<div style="margin-top:12px"><button class="btn danger small" data-cancel="${order.id}">Cancel order</button></div>` : "")).join(""));
  app.querySelectorAll("[data-cancel]").forEach((button) => (button.onclick = () => attempt(async () => {
    await api("POST", "/api/orders/cancel", { orderId: button.dataset.cancel });
    toast("Order cancelled");
    await renderOrders();
  })));

  // Live countdown for Instant orders: tick every second, ask the server again every 15 seconds
  clearInterval(ticker);
  if (app.querySelector(".countdown")) {
    let seconds = 0;
    ticker = setInterval(() => {
      if ((location.hash.slice(1) || "menu") !== "orders") { clearInterval(ticker); return; }
      seconds++;
      let finished = false;
      app.querySelectorAll(".countdown").forEach((box) => {
        const left = Math.max(0, Number(box.dataset.left) - 1);
        box.dataset.left = left;
        const done = Math.min(100, ((box.dataset.total - left) / box.dataset.total) * 100);
        box.querySelector(".clock").textContent = clock(left);
        box.querySelector(".road i").style.width = done + "%";
        box.querySelector(".road em").style.left = done + "%";
        if (left === 0) finished = true;
      });
      if (finished || seconds % 15 === 0) attempt(renderOrders);
    }, 1000);
  }
}

// ---------- page: admin panel ----------
async function renderAdmin() {
  const [stats, orders] = await Promise.all([api("GET", "/api/admin/stats"), api("GET", "/api/admin/orders")]);
  state.menu = await api("GET", "/api/menu");
  const statuses = [...STATUS_FLOW, "Cancelled"];

  app.innerHTML = `
    <h1 class="page-title">Admin panel</h1>
    <div class="stats">
      <div class="stat"><b>${stats.orders}</b><span>Total orders</span></div>
      <div class="stat"><b>${stats.activeOrders}</b><span>Active orders</span></div>
      <div class="stat hot"><b>${stats.instantOrders}</b><span>&#9889; Instant orders</span></div>
      <div class="stat"><b>${rupees(stats.revenue)}</b><span>Revenue</span></div>
      <div class="stat"><b>${stats.customers}</b><span>Customers</span></div>
      <div class="stat"><b>${stats.menuItems}</b><span>Menu items</span></div>
    </div>

    <div class="panel">
      <h3>Orders</h3>
      ${orders.length === 0 ? `<p class="meta">No orders yet.</p>` : `
      <div class="table-wrap"><table>
        <thead><tr><th>Order</th><th>Customer</th><th>Items</th><th>Total</th><th>Delivery</th><th>Status</th></tr></thead>
        <tbody>${orders.map((order) => `
          <tr>
            <td><b>#${order.id}</b><div class="meta">${esc(order.placedAt)}</div></td>
            <td>${esc(order.customerName)}<div class="meta">${esc(order.address)}</div></td>
            <td>${order.items.map((line) => `${line.quantity} &times; ${esc(line.name)}`).join("<br>")}</td>
            <td><b>${rupees(order.total)}</b><div class="meta">${esc(order.paymentMethod)}</div></td>
            <td>${order.instant ? `<span class="mini">&#9889; INSTANT</span>` : "Standard"}<div class="meta">${order.etaMinutes} min</div></td>
            <td>${order.status === "Delivered" || order.status === "Cancelled"
              ? `<span class="status ${esc(order.status)}">${esc(order.status)}</span>`
              : `<select data-order="${order.id}" aria-label="Status of order ${order.id}">${statuses.map((s) => `<option ${s === order.status ? "selected" : ""}>${s}</option>`).join("")}</select>`}</td>
          </tr>`).join("")}</tbody>
      </table></div>`}
    </div>

    <div class="panel section-gap">
      <h3>Add a menu item</h3>
      <form id="add-item">
        <div class="form-grid">
          <div><label for="category">Category</label><select id="category"><option>Veg</option><option>Non-Veg</option><option>Beverage</option><option>Dessert</option><option value="Instant">Instant (10-minute store)</option></select></div>
          <div><label for="name">Name</label><input id="name" required></div>
          <div><label for="price">Price (&#8377;)</label><input id="price" type="number" min="1" step="1" required></div>
        </div>
        <label for="description">Description</label><input id="description" required>
        <button class="btn" style="margin-top:18px">Add item</button>
      </form>
    </div>

    <div class="panel section-gap">
      <h3>Menu items</h3>
      <div class="table-wrap"><table>
        <thead><tr><th>#</th><th>Name</th><th>Category</th><th>Price</th><th>Stock</th><th>Available</th><th></th></tr></thead>
        <tbody>${state.menu.map((item) => `
          <tr>
            <td>${item.id}</td><td><b>${esc(item.name)}</b></td><td>${item.instant ? `<span class="mini">&#9889; INSTANT</span>` : esc(item.category)}</td><td>${rupees(item.price)}</td>
            <td>${item.instant ? `${item.stock} <button class="btn ghost small" data-restock="${item.id}">+10</button>` : `<span class="meta">made fresh</span>`}</td>
            <td><button class="btn ghost small" data-toggle="${item.id}" data-next="${!item.available}">${item.available ? "Yes - hide" : "No - show"}</button></td>
            <td><button class="btn danger small" data-remove="${item.id}">Remove</button></td>
          </tr>`).join("")}</tbody>
      </table></div>
    </div>`;

  app.querySelectorAll("[data-order]").forEach((select) => (select.onchange = () => attempt(async () => {
    await api("POST", "/api/admin/orders/status", { orderId: select.dataset.order, status: select.value });
    toast("Order #" + select.dataset.order + " is now " + select.value);
    await renderAdmin();
  })));
  app.querySelectorAll("[data-restock]").forEach((button) => (button.onclick = () => attempt(async () => {
    await api("POST", "/api/admin/menu/restock", { itemId: button.dataset.restock, quantity: 10 });
    toast("Stock increased by 10");
    await renderAdmin();
  })));
  app.querySelectorAll("[data-toggle]").forEach((button) => (button.onclick = () => attempt(async () => {
    await api("POST", "/api/admin/menu/availability", { itemId: button.dataset.toggle, available: button.dataset.next });
    await renderAdmin();
  })));
  app.querySelectorAll("[data-remove]").forEach((button) => (button.onclick = () => attempt(async () => {
    if (!confirm("Remove this item from the menu?")) return;
    await api("POST", "/api/admin/menu/remove", { itemId: button.dataset.remove });
    toast("Item removed");
    await renderAdmin();
  })));
  document.getElementById("add-item").onsubmit = (event) => {
    event.preventDefault();
    const f = event.target.elements;
    attempt(async () => {
      await api("POST", "/api/admin/menu/add", { category: f.category.value, name: f.name.value, price: f.price.value, description: f.description.value });
      toast("Item added to the menu");
      await renderAdmin();
    });
  };
}

// ---------- router: decides which page to draw ----------
async function render() {
  clearInterval(ticker);
  let page = location.hash.slice(1) || "menu";
  if ((page === "cart" || page === "orders") && !isCustomer()) page = state.user ? "menu" : "login";
  if (page === "admin" && !isAdmin()) page = state.user ? "menu" : "login";
  if ((page === "login" || page === "register") && state.user) page = "menu";
  renderNav();
  window.scrollTo(0, 0);
  await attempt(async () => {
    if (page === "login") renderLogin();
    else if (page === "register") renderRegister();
    else if (page === "cart") {
      if (state.store === "instant") state.delivery = "instant";   // shopping in the Instant Store: prefer 10-minute delivery
      await renderCart();
    }
    else if (page === "orders") await renderOrders();
    else if (page === "admin") await renderAdmin();
    else { state.menu = await api("GET", "/api/menu"); renderMenu(); }
  });
}

// ---------- start ----------
async function start() {
  if (state.token) {
    try {
      state.user = await api("GET", "/api/me");
      if (state.user.role === "customer") await loadCart();
    } catch (e) { setSession("", null); }                 // the server was restarted - log in again
  }
  window.addEventListener("hashchange", render);
  render();
}
start();
