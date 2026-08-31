/**
 * @file MySQLAnalysisRepository.h
 * @brief MySQL-backed implementation of IAnalysisRepository.
 */

#pragma once

#include "analysis/IAnalysisRepository.h"

namespace cortex::infrastructure {

class MySQLAnalysisRepository final : public cortex::analysis::IAnalysisRepository {
public:
    MySQLAnalysisRepository() = default;

    void save(const cortex::domain::AnalysisResult& result) noexcept override;

    std::optional<cortex::domain::AnalysisResult> findByJobId(
        const std::string& jobId) const noexcept override;

private:
    static std::string toDatetime(std::chrono::system_clock::time_point tp) noexcept;
    static std::chrono::system_clock::time_point fromDatetime(const std::string& value) noexcept;
};

} // namespace cortex::infrastructure
