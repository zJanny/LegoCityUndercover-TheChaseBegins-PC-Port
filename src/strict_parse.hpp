#pragma once
#include <charconv>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace strict {

[[noreturn]] inline void reject(const std::string& reason, const std::string& text) {
    throw std::runtime_error(reason + ": '" + text + "'");
}

inline long long integer(const std::string& text, long long low, long long high) {
    long long value = 0;
    const char* end = text.data() + text.size();
    const std::from_chars_result result = std::from_chars(text.data(), end, value);
    if (text.empty() || result.ec != std::errc() || result.ptr != end) reject("not a whole number", text);
    if (value < low || value > high) reject("out of range, allowed " + std::to_string(low) + " to " + std::to_string(high), text);
    return value;
}

inline double real(const std::string& text, double low, double high) {
    double value = 0;
    const char* end = text.data() + text.size();
    const std::from_chars_result result = std::from_chars(text.data(), end, value, std::chars_format::general);
    if (text.empty() || result.ec != std::errc() || result.ptr != end || !std::isfinite(value)) reject("not a number", text);
    if (value < low || value > high) {
        std::ostringstream allowed;
        allowed << "out of range, allowed " << low << " to " << high;
        reject(allowed.str(), text);
    }
    return value;
}

inline std::vector<std::string> words(const std::string& text) {
    std::vector<std::string> fields;
    std::istringstream stream(text);
    std::string field;
    while (stream >> field) fields.push_back(field);
    return fields;
}

inline std::vector<std::string> split(const std::string& text, char separator) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        const std::size_t end = text.find(separator, start);
        parts.push_back(text.substr(start, end == std::string::npos ? std::string::npos : end - start));
        if (end == std::string::npos) return parts;
        start = end + 1;
    }
}

}
