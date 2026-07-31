/**
 * @file IRepositoryHealthRepository.h
 * @brief Storage interface for repository health results.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "health/RepositoryHealthResult.h"
#include <optional>
#include <string>

namespace cortex::health {

class IRepositoryHealthRepository {
public:
    virtual ~IRepositoryHealthRepository() = default;

    virtual void save(const RepositoryHealthResult& result) noexcept = 0;

    [[nodiscard]] virtual std::optional<RepositoryHealthResult> findByJobId(
        const std::string& jobId) const noexcept = 0;

    IRepositoryHealthRepository(const IRepositoryHealthRepository&) = delete;
    IRepositoryHealthRepository& operator=(const IRepositoryHealthRepository&) = delete;

protected:
    IRepositoryHealthRepository() = default;
};

} // namespace cortex::health
