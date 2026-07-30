/**
 * @file ConfigurationValidator.cpp
 * @brief Startup validation logic — checks for required keys and logs missing configuration
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

#include "config/ConfigurationValidator.h"
#include <cstdlib>
#include <sstream>

namespace cortex::config {

Result<void> ConfigurationValidator::validate(
    const Configuration& config,
    const RequiredKeys& required) noexcept {
    
    // Validate string keys
    for (const auto& key : required.stringKeys) {
        if (!config.getString(key).has_value()) {
            return Result<void>(Error::configError(
                "Required string key not found: " + key));
        }
    }

    // Validate int keys
    for (const auto& key : required.intKeys) {
        if (!config.getInt(key).has_value()) {
            return Result<void>(Error::configError(
                "Required int key not found: " + key));
        }
    }

    // Validate uint keys
    for (const auto& key : required.uintKeys) {
        if (!config.getUInt(key).has_value()) {
            return Result<void>(Error::configError(
                "Required uint key not found: " + key));
        }
    }

    // Validate bool keys
    for (const auto& key : required.boolKeys) {
        if (!config.getBool(key).has_value()) {
            return Result<void>(Error::configError(
                "Required bool key not found: " + key));
        }
    }

    return Result<void>();  // Success
}

Result<std::shared_ptr<EnhancedConfiguration>> EnhancedConfiguration::create(
    std::shared_ptr<Configuration> base,
    const std::unordered_map<std::string, std::string>& defaults) noexcept {
    
    if (!base) {
        return Result<std::shared_ptr<EnhancedConfiguration>>(
            Error::configError("Base configuration cannot be null"));
    }

    try {
        auto enhanced = std::shared_ptr<EnhancedConfiguration>(
            new EnhancedConfiguration(base, defaults));
        return Result<std::shared_ptr<EnhancedConfiguration>>(enhanced);
    } catch (const std::exception& e) {
        return Result<std::shared_ptr<EnhancedConfiguration>>(
            Error::configError(std::string("Failed to create configuration: ") + e.what()));
    }
}

std::string EnhancedConfiguration::getString(
    const std::string& key,
    const std::string& envVarName,
    const std::string& defaultValue) noexcept {
    
    // Priority 1: Environment variable
    if (!envVarName.empty()) {
        const char* envValue = std::getenv(envVarName.c_str());
        if (envValue) {
            return std::string(envValue);
        }
    }

    // Priority 2: Configuration file
    if (auto value = base_->getString(key)) {
        return *value;
    }

    // Priority 3: Defaults map
    auto it = defaults_.find(key);
    if (it != defaults_.end()) {
        return it->second;
    }

    // Priority 4: Provided default
    return defaultValue;
}

std::optional<int32_t> EnhancedConfiguration::getInt(const std::string& key) noexcept {
    return base_->getInt(key);
}

std::optional<uint32_t> EnhancedConfiguration::getUInt(const std::string& key) noexcept {
    return base_->getUInt(key);
}

std::optional<bool> EnhancedConfiguration::getBool(const std::string& key) noexcept {
    return base_->getBool(key);
}

int32_t EnhancedConfiguration::getIntOrDefault(const std::string& key,
                                               int32_t defaultValue) noexcept {
    if (auto value = getInt(key)) {
        return *value;
    }
    return defaultValue;
}

uint32_t EnhancedConfiguration::getUIntOrDefault(const std::string& key,
                                                 uint32_t defaultValue) noexcept {
    if (auto value = getUInt(key)) {
        return *value;
    }
    return defaultValue;
}

bool EnhancedConfiguration::getBoolOrDefault(const std::string& key,
                                             bool defaultValue) noexcept {
    if (auto value = getBool(key)) {
        return *value;
    }
    return defaultValue;
}

} // namespace cortex::config
