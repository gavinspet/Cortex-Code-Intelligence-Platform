/**
 * @file RepositoryRequest.h
 * @brief Deserializes and validates the POST /repositories request body
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
#include <string_view>

namespace cortex::api::repositories {

/**
 * Repository request DTO
 * Represents incoming POST /repositories request body
 * 
 * Use std::string_view where immutability is needed
 * Use std::string for stored values
 */
class RepositoryRequest {
public:
    explicit RepositoryRequest(std::string repositoryUrl) noexcept
        : repositoryUrl_(std::move(repositoryUrl)) {}

    std::string_view getRepositoryUrl() const noexcept {
        return repositoryUrl_;
    }

    // Check if URL is empty
    bool isEmpty() const noexcept {
        return repositoryUrl_.empty();
    }

    // Delete copy/move
    RepositoryRequest(const RepositoryRequest&) = default;
    RepositoryRequest& operator=(const RepositoryRequest&) = default;
    RepositoryRequest(RepositoryRequest&&) = default;
    RepositoryRequest& operator=(RepositoryRequest&&) = default;

private:
    std::string repositoryUrl_;
};

} // namespace cortex::api::repositories
