#include "config/FileConfiguration.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace cortex::config {

FileConfiguration::FileConfiguration(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open configuration file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream iss(buffer.str());
    
    if (!Json::parseFromStream(builder, iss, &root_, &errs)) {
        throw std::runtime_error("Failed to parse configuration file: " + errs);
    }
}

const Json::Value& FileConfiguration::getJsonValue(const std::string& key) const {
    // Handle dot notation: "server.port" → root["server"]["port"]
    const Json::Value* current = &root_;
    
    size_t start = 0;
    while (start < key.size()) {
        size_t dot = key.find('.', start);
        if (dot == std::string::npos) {
            dot = key.size();
        }
        
        std::string part = key.substr(start, dot - start);
        if (current->isMember(part)) {
            current = &((*current)[part]);
        } else {
            return Json::Value::null;
        }
        
        start = dot + 1;
    }
    
    return *current;
}

std::optional<std::string> FileConfiguration::getString(const std::string& key) const {
    const auto& value = getJsonValue(key);
    if (value.isString()) {
        return value.asString();
    }
    return std::nullopt;
}

std::optional<int32_t> FileConfiguration::getInt(const std::string& key) const {
    const auto& value = getJsonValue(key);
    if (value.isInt()) {
        return value.asInt();
    }
    return std::nullopt;
}

std::optional<uint32_t> FileConfiguration::getUInt(const std::string& key) const {
    const auto& value = getJsonValue(key);
    if (value.isUInt()) {
        return value.asUInt();
    }
    return std::nullopt;
}

std::optional<bool> FileConfiguration::getBool(const std::string& key) const {
    const auto& value = getJsonValue(key);
    if (value.isBool()) {
        return value.asBool();
    }
    return std::nullopt;
}

} // namespace cortex::config
