/**
 * @file ITechnologyRepository.h
 * @brief Storage interface for persisting technology analysis results.
 * @project Cortex Code Intelligence Platform
 * @author Kartick Kumar Ghosh
 * @github https://github.com/gavinspet
 * @email kartick.ghosh.dev@gmail.com
 * @copyright © 2026
 * @license MIT
 */
#pragma once

#include "technology/TechnologyResult.h"
#include <optional>
#include <string>

namespace cortex::technology {

/**
 * @class ITechnologyRepository
 * @brief Abstract repository for TechnologyAnalysis persistence.
 *
 * Follows the Repository Pattern used throughout Cortex.
 * Current default implementation: InMemoryTechnologyRepository.
 */
class ITechnologyRepository {
public:
    virtual ~ITechnologyRepository() = default;

    virtual void save(const TechnologyAnalysis& analysis) noexcept = 0;

    [[nodiscard]] virtual std::optional<TechnologyAnalysis> findByJobId(
        const std::string& jobId) const noexcept = 0;

    ITechnologyRepository(const ITechnologyRepository&) = delete;
    ITechnologyRepository& operator=(const ITechnologyRepository&) = delete;

protected:
    ITechnologyRepository() = default;
};

} // namespace cortex::technology
