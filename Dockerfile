# Builds and runs the FoodExpress C++ server online (used by Render).
# Works whether the files are in backend/ and frontend/ folders or all in one folder.

# Step 1: compile the C++ code and collect the website files
FROM gcc:13 AS build
WORKDIR /src
COPY . .
RUN if [ -f backend/main.cpp ]; then SRC=backend; WEB=frontend; else SRC=.; WEB=.; fi \
 && mkdir -p /app/frontend/images \
 && g++ -std=c++17 -O2 -pthread -static "$SRC/main.cpp" -o /app/server \
 && cp "$WEB/index.html" "$WEB/app.js" "$WEB/style.css" /app/frontend/ \
 && find . -name '*.jpg' -exec cp {} /app/frontend/images/ \;

# Step 2: small image that only contains the program and the website files
FROM debian:bookworm-slim
WORKDIR /app
COPY --from=build /app ./
# Render gives the port through the PORT variable; the server reads it automatically
ENV PORT=10000
EXPOSE 10000
CMD ["./server"]
