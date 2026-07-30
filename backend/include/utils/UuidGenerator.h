/**
 * @file UuidGenerator.h
 * @brief Generates RFC 4122 UUID v4 strings used as unique job identifiers
 *
 * @project Cortex Code Intelligence Platform
 *
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 *
 * @copyright Copyright (c) 2026 Kartick Kumar Ghosh
 * @license MIT
 */

#pragma once

#include <string>
#include <random>
#include <sstream>
#include <iomanip>

namespace cortex::utils {

/**
 * Simple UUID v4 generator using random numbers
 * Note: This is a simplified implementation for demonstration.
 * Production systems should use a proper UUID library.
 */
class UuidGenerator {
public:
    /**
     * Generate a UUID v4 format string
     * Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
     * where x is any hexadecimal digit and y is one of 8, 9, A, or B
     */
    static std::string generate() noexcept {
        try {
            // Use thread-local random engine for thread safety
            thread_local static std::random_device rd;
            thread_local static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 15);

            std::stringstream ss;
            // Generate 8-4-4-4-12 format UUID
            for (int i = 0; i < 8; ++i) {
                ss << std::hex << dis(gen);
            }
            ss << "-";
            for (int i = 0; i < 4; ++i) {
                ss << std::hex << dis(gen);
            }
            ss << "-4"; // Version 4
            for (int i = 0; i < 3; ++i) {
                ss << std::hex << dis(gen);
            }
            ss << "-";
            ss << std::hex << (8 + dis(gen) % 4); // Variant
            for (int i = 0; i < 3; ++i) {
                ss << std::hex << dis(gen);
            }
            ss << "-";
            for (int i = 0; i < 12; ++i) {
                ss << std::hex << dis(gen);
            }

            return ss.str();
        } catch (...) {
            // Fallback to a simple counter-based ID if generation fails
            thread_local static int counter = 0;
            return "job-" + std::to_string(++counter);
        }
    }

    // Static utility class - no instances
    UuidGenerator() = delete;
};

} // namespace cortex::utils
