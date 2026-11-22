// Exceptions.h - custom exception classes (OOP concept: Exception Handling + Inheritance)
#pragma once
#include <stdexcept>
#include <string>

// Base class for every error raised by our system.
// httpStatus tells the web server which status code to send to the browser.
class FoodException : public std::runtime_error {
    int httpStatus;
public:
    FoodException(const std::string& message, int status = 400)
        : std::runtime_error(message), httpStatus(status) {}
    int getHttpStatus() const { return httpStatus; }
};

class InvalidInputException : public FoodException {
public:
    explicit InvalidInputException(const std::string& message) : FoodException(message, 400) {}
};

class AuthException : public FoodException {
public:
    explicit AuthException(const std::string& message) : FoodException(message, 401) {}
};

class NotFoundException : public FoodException {
public:
    explicit NotFoundException(const std::string& message) : FoodException(message, 404) {}
};

class PaymentException : public FoodException {
public:
    explicit PaymentException(const std::string& message) : FoodException(message, 402) {}
};
