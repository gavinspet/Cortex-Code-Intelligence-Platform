/**
 * @file ConfigurationValidator.h
 * @brief Validates that all required configuration keys are present at startup
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

#include "Configuration.h"
#include "../utils/Result.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace cortex::config {

using cortex::utils::Result;
using cortex::utils::Error;

/**
 * @class ConfigurationValidator
 * @brief Validates configuration values and ensures required fields exist.
 * 
 * Design Pattern: Strategy Pattern
 * SOLID: Single Responsibility - validates configuration only
 * 
 * Why: Separates validation logic from configuration reading.
 * Makes it easy to change validation rules without touching Configuration class.
 * 
 * Rationale:
 * - Validates at startup, not at runtime
 * - Fails fast with clear error messages
 * - Ensures application never starts with invalid config
 */
class ConfigurationValidator {
public:
    // Define required configuration keys
    struct RequiredKeys {
        std::vector<std::string> stringKeys;
        std::vector<std::string> intKeys;
        std::vector<std::string> uintKeys;
        std::vector<std::string> boolKeys;
    };

    /**
     * Validate configuration against requirements
     * @param config Configuration to validate
     * @param required Required keys
     * @return Result<void> - Success or validation error
     */
    static Result<void> validate(
        const Configuration& config,
        const RequiredKeys& required) noexcept;

    /**
     * Get default value, falling back to provided default
     * @param config Configuration to read from
     * @param key Configuration key
     * @param defaultValue Value to use if key not found
     * @return The configuration value or default
     */
    template<typename T>
    static T getOrDefault(const Configuration& config,
                         const std::string& key,
                         T defaultValue);
};

/**
 * @class EnhancedConfiguration
 * @brief Wraps Configuration with validation, defaults, and environment overrides.
 * 
 * Design Pattern: Decorator Pattern
 * SOLID: 
 * - Dependency Inversion: Still depends on Configuration interface
 * - Open/Closed: Adds behavior without modifying Configuration
 * 
 * Why: Provides production-grade configuration with:
 * - Environment variable overrides
 * - Default values
 * - Validation at creation time
 * - Clear error reporting
 */
class EnhancedConfiguration {
public:
    /**
     * Create configuration with validation and defaults
     * @param base Underlying configuration
     * @param defaults Default values for missing keys
     * @return Configuration ready to use or error
     */
    static Result<std::shared_ptr<EnhancedConfiguration>> create(
        std::shared_ptr<Configuration> base,
        const std::unordered_map<std::string, std::string>& defaults = {}) noexcept;

    /**
     * Get string value with environment variable override support
     * 
     * Priority (highest to lowest):
     * 1. Environment variable (if exists)
     * 2. Configuration file
     * 3. Default value (if provided)
     * 4. Empty string
     */
    std::string getString(const std::string& key,
                         const std::string& envVarName = "",
                         const std::string& defaultValue = "") noexcept;

    std::optional<int32_t> getInt(const std::string& key) noexcept;
    std::optional<uint32_t> getUInt(const std::string& key) noexcept;
    std::optional<bool> getBool(const std::string& key) noexcept;

    // Get with default values
    int32_t getIntOrDefault(const std::string& key, int32_t defaultValue) noexcept;
    uint32_t getUIntOrDefault(const std::string& key, uint32_t defaultValue) noexcept;
    bool getBoolOrDefault(const std::string& key, bool defaultValue) noexcept;

private:
    explicit EnhancedConfiguration(std::shared_ptr<Configuration> base,
                                   const std::unordered_map<std::string, std::string>& defaults)
        : base_(base), defaults_(defaults) {}

    std::shared_ptr<Configuration> base_;
    std::unordered_map<std::string, std::string> defaults_;
};

} // namespace cortex::config
