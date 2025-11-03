#pragma once

#include <stdexcept>
#include <string>

namespace ephant {

class io_error : public std::runtime_error {
public:
    explicit io_error(const std::string& message)
        : std::runtime_error("[IO Error] " + message) {
            // You can add additional logging or error handling here if needed.
        }
};

class sqr_exception : public std::runtime_error {
public:
    explicit sqr_exception(const std::string& message)
        : std::runtime_error("[Square Error] " + message) {
            // You can add additional logging or error handling here if needed.
        }
};

class search_exception : public std::runtime_error {
public:
    explicit search_exception(const std::string& message)
        : std::runtime_error("[Search Error] " + message) {}
};

} // namespace ephant

#if defined(EXCEPTIONS_ENABLED)
// TODO: Fatal Asserts vs Exceptions, should I combine them?
#define THROW_EXPR(expr, exceptionType, message) \
    if (!(expr)) {                   \
        throw exceptionType(message); \
    }
#else
#define THROW_EXPR(expr, exceptionType, message) 
#endif
