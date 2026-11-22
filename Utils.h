// Utils.h - small helper functions used by every other file
#pragma once
#include <cctype>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace util {

// Split "a|b|c" into {"a","b","c"}
inline std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : text) {
        if (c == delimiter) { parts.push_back(current); current.clear(); }
        else current += c;
    }
    parts.push_back(current);
    return parts;
}

inline std::string trim(const std::string& s) {
    size_t start = 0, end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

// Remove the characters we use as separators in the data files
inline std::string sanitize(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '|' || c == '~' || c == ';' || c == '\n' || c == '\r' || c == '\t') out += ' ';
        else out += c;
    }
    return trim(out);
}

// "Paneer Butter Masala" -> "paneer-butter-masala" (used for photo file names)
inline std::string slugify(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (std::isalnum(c)) out += static_cast<char>(std::tolower(c));
        else if (!out.empty() && out.back() != '-') out += '-';
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    return out;
}

inline std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// Decode text sent by an HTML form ("Masala%20Dosa" -> "Masala Dosa")
inline std::string urlDecode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '+') out += ' ';
        else if (s[i] == '%' && i + 2 < s.size() + 0 && std::isxdigit(static_cast<unsigned char>(s[i + 1]))
                 && std::isxdigit(static_cast<unsigned char>(s[i + 2]))) {
            out += static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16));
            i += 2;
        } else out += s[i];
    }
    return out;
}

inline std::string nowString() {
    std::time_t t = std::time(nullptr);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M", std::localtime(&t));
    return buffer;
}

// Simple FNV-1a hash so that plain passwords are never written to the file.
// (Good enough for a college project - a real site would use bcrypt/argon2.)
inline std::string hashPassword(const std::string& password) {
    const std::string salted = "FoodExpress#" + password;
    std::uint64_t hash = 1469598103934665603ULL;
    for (unsigned char c : salted) { hash ^= c; hash *= 1099511628211ULL; }
    std::ostringstream out;
    out << std::hex << hash;
    return out.str();
}

// Random session id given to the browser after login
inline std::string randomToken() {
    static std::random_device device;
    static std::mt19937_64 generator(device());
    std::ostringstream out;
    out << std::hex << generator() << generator();
    return out.str();
}

inline std::string money(double amount) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << amount;
    return out.str();
}

// ---- JSON helpers (the frontend talks to the backend using JSON) ----
inline std::string jsonEscape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) out += ' ';
                else out += static_cast<char>(c);
        }
    }
    return out;
}

inline std::string jsonStr(const std::string& s) { return "\"" + jsonEscape(s) + "\""; }

} // namespace util
