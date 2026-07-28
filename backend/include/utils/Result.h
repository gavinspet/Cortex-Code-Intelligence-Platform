#pragma once

#include <string>
#include <optional>
#include <variant>

namespace cortex::utils {

/**
 * @class Error
 * @brief Represents an error with code and message.
 * 
 * Design Pattern: Value Object
 * SOLID: Single Responsibility - represents error state only
 * 
 * Why: Better than throwing exceptions for expected errors.
 * Provides type-safe error handling.
 */
class Error {
public:
    enum class Code {
        Success = 0,
        ConfigurationError = 1,
        ValidationError = 2,
        EnvironmentError = 3,
        RuntimeError = 4
    };

    explicit Error(Code code, const std::string& message = "")
        : code_(code), message_(message) {}

    Code code() const noexcept { return code_; }
    const std::string& message() const noexcept { return message_; }
    bool isSuccess() const noexcept { return code_ == Code::Success; }

    static Error success() { return Error(Code::Success); }
    static Error configError(const std::string& msg) {
        return Error(Code::ConfigurationError, msg);
    }
    static Error validationError(const std::string& msg) {
        return Error(Code::ValidationError, msg);
    }
    static Error envError(const std::string& msg) {
        return Error(Code::EnvironmentError, msg);
    }
    static Error runtimeError(const std::string& msg) {
        return Error(Code::RuntimeError, msg);
    }

private:
    Code code_;
    std::string message_;
};

/**
 * @class Result
 * @brief Represents either a successful value or an error.
 * 
 * Design Pattern: Discriminated Union (variant-based)
 * SOLID: Interface Segregation - only exposes value OR error
 * 
 * Usage:
 *   auto result = doSomething();
 *   if (result) {
 *       auto value = *result;
 *   } else {
 *       auto error = result.error();
 *   }
 */
template<typename T>
class Result {
public:
    // Success constructor
    explicit Result(const T& value) : data_(value) {}
    explicit Result(T&& value) : data_(std::move(value)) {}

    // Error constructor
    explicit Result(const Error& error) : data_(error) {}
    explicit Result(Error&& error) : data_(std::move(error)) {}

    // Conversions
    bool isOk() const noexcept {
        return std::holds_alternative<T>(data_);
    }

    bool isError() const noexcept {
        return std::holds_alternative<Error>(data_);
    }

    // Explicit bool conversion
    explicit operator bool() const noexcept {
        return isOk();
    }

    // Access value
    T& value() & {
        if (!isOk()) throw std::logic_error("Result holds error, not value");
        return std::get<T>(data_);
    }

    const T& value() const& {
        if (!isOk()) throw std::logic_error("Result holds error, not value");
        return std::get<T>(data_);
    }

    T&& value() && {
        if (!isOk()) throw std::logic_error("Result holds error, not value");
        return std::get<T>(std::move(data_));
    }

    // Dereference operators
    T* operator->() noexcept {
        return isOk() ? &std::get<T>(data_) : nullptr;
    }

    const T* operator->() const noexcept {
        return isOk() ? &std::get<T>(data_) : nullptr;
    }

    T& operator*() & {
        return value();
    }

    const T& operator*() const& {
        return value();
    }

    // Access error
    const Error& error() const& {
        if (!isError()) throw std::logic_error("Result holds value, not error");
        return std::get<Error>(data_);
    }

    Error&& error() && {
        if (!isError()) throw std::logic_error("Result holds value, not error");
        return std::get<Error>(std::move(data_));
    }

private:
    std::variant<T, Error> data_;
};

/**
 * @class Result<void>
 * @brief Specialization for functions that return only success/error.
 */
template<>
class Result<void> {
public:
    // Success constructor
    Result() : error_(std::nullopt) {}

    // Error constructor
    explicit Result(const Error& error) : error_(error) {}
    explicit Result(Error&& error) : error_(std::move(error)) {}

    bool isOk() const noexcept { return !error_.has_value(); }
    bool isError() const noexcept { return error_.has_value(); }

    explicit operator bool() const noexcept { return isOk(); }

    const Error& error() const {
        if (!isError()) throw std::logic_error("Result is success, not error");
        return *error_;
    }

private:
    std::optional<Error> error_;
};

} // namespace cortex::utils
