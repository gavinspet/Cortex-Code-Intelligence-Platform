/**
 * @file EnvLimits.h
 * @brief Parsing of positive, capped integer limits from environment values
 *
 * @project Cortex Code Intelligence Platform
 *
 * @copyright Copyright (c) 2026 Kartick Kumar Ghosh
 * @license MIT
 */

#pragma once

#include <charconv>
#include <string_view>
#include <system_error>

namespace cortex::utils {

/**
 * Parse a positive decimal integer and clamp it to maximum.
 *
 * @return fallback if raw is null, empty, non-numeric, has trailing characters,
 *         or is <= 0; maximum if the value exceeds maximum (including overflow)
 */
inline long long parsePositiveCapped(const char* raw, long long fallback, long long maximum) noexcept {
    if (raw == nullptr) {
        return fallback;
    }
    const std::string_view text(raw);
    if (text.empty()) {
        return fallback;
    }
    long long value = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (end != text.data() + text.size()) {
        return fallback;
    }
    if (ec == std::errc::result_out_of_range) {
        return text.front() == '-' ? fallback : maximum;
    }
    if (ec != std::errc() || value <= 0) {
        return fallback;
    }
    return value > maximum ? maximum : value;
}

inline constexpr long long kDefaultCloneTimeoutSeconds = 120;
inline constexpr long long kMaxCloneTimeoutSeconds = 3600;

/** CLONE_TIMEOUT_SECONDS value: positive, default 120, capped at 3600. */
inline long long cloneTimeoutSeconds(const char* raw) noexcept {
    return parsePositiveCapped(raw, kDefaultCloneTimeoutSeconds, kMaxCloneTimeoutSeconds);
}

} // namespace cortex::utils
