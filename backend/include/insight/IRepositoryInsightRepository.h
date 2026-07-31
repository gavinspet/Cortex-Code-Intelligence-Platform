/**
 * @file IRepositoryInsightRepository.h
 * @brief Storage interface for repository insights.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "insight/RepositoryInsightResult.h"
#include <optional>
#include <string>

namespace cortex::insight {

class IRepositoryInsightRepository {
public:
    virtual ~IRepositoryInsightRepository() = default;
    virtual void save(const RepositoryInsightResult& result) noexcept = 0;
    [[nodiscard]] virtual std::optional<RepositoryInsightResult> findByJobId(
        const std::string& jobId) const noexcept = 0;

    IRepositoryInsightRepository(const IRepositoryInsightRepository&) = delete;
    IRepositoryInsightRepository& operator=(const IRepositoryInsightRepository&) = delete;
protected:
    IRepositoryInsightRepository() = default;
};

} // namespace cortex::insight
