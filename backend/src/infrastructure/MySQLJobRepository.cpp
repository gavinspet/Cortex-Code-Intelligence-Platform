/**
 * @file MySQLJobRepository.cpp
 * @brief All MySQL CRUD operations for jobs using prepared statements — no SQL string concatenation
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

#include "infrastructure/MySQLJobRepository.h"
#include "database/Database.h"
#include "logging/Logger.h"
#include "domain/Job.h"
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/sqlstring.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace cortex::infrastructure {

using cortex::logging::Logger;
using cortex::domain::Job;
using cortex::domain::JobStatus;
using cortex::domain::jobStatusToString;
using cortex::database::Database;

std::string MySQLJobRepository::timePointToDatetimeString(
    std::chrono::system_clock::time_point tp) noexcept
{
    try {
        auto time_t_val = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t_val), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    } catch (...) {
        return "";
    }
}

std::chrono::system_clock::time_point MySQLJobRepository::datetimeStringToTimePoint(
    const std::string& dateStr) noexcept
{
    try {
        std::tm tm_time = {};
        std::istringstream ss(dateStr);
        ss >> std::get_time(&tm_time, "%Y-%m-%d %H:%M:%S");
        return std::chrono::system_clock::from_time_t(std::mktime(&tm_time));
    } catch (...) {
        return std::chrono::system_clock::now();
    }
}

Job MySQLJobRepository::buildJobFromResultSet(
    std::shared_ptr<sql::ResultSet>& rs) noexcept
{
    try {
        std::string id = rs->getString("id");
        std::string repoUrl = rs->getString("repository_url");
        std::string statusStr = rs->getString("status");
        std::string createdAtStr = rs->getString("created_at");
        
        JobStatus status = JobStatus::QUEUED;
        if (statusStr == "RUNNING") status = JobStatus::RUNNING;
        else if (statusStr == "COMPLETED") status = JobStatus::COMPLETED;
        else if (statusStr == "FAILED") status = JobStatus::FAILED;

        auto createdAt = datetimeStringToTimePoint(createdAtStr);
        Job job(id, repoUrl, status, createdAt);

        // Set optional timestamps
        if (!rs->isNull("started_at")) {
            std::string startedAtStr = rs->getString("started_at");
            job.setStartedAt(datetimeStringToTimePoint(startedAtStr));
        }

        if (!rs->isNull("completed_at")) {
            std::string completedAtStr = rs->getString("completed_at");
            job.setCompletedAt(datetimeStringToTimePoint(completedAtStr));
        }

        return job;

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error building job from result set: ") + e.what());
        throw;
    }
}

void MySQLJobRepository::save(const Job& job) noexcept {
    try {
        auto conn = Database::instance().getConnection();
        std::string sql = "INSERT INTO jobs (id, repository_url, status, created_at) VALUES (?, ?, ?, ?)";
        
        std::shared_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(sql));
        
        pstmt->setString(1, job.getId());
        pstmt->setString(2, job.getRepositoryUrl());
        pstmt->setString(3, std::string(jobStatusToString(job.getStatus())));
        pstmt->setString(4, timePointToDatetimeString(job.getCreatedAt()));
        
        pstmt->execute();
        Logger::instance().info(std::string("Inserted job into MySQL: ") + job.getId());

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error saving job to database: ") + e.what());
    }
}

std::optional<Job> MySQLJobRepository::findById(const std::string& jobId) const noexcept {
    try {
        auto conn = Database::instance().getConnection();
        std::string sql = "SELECT id, repository_url, status, created_at, started_at, completed_at FROM jobs WHERE id = ?";
        
        std::shared_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(sql));
        pstmt->setString(1, jobId);
        
        std::shared_ptr<sql::ResultSet> res(pstmt->executeQuery());
        
        if (res->next()) {
            Logger::instance().info(std::string("Retrieved job from MySQL: ") + jobId);
            return buildJobFromResultSet(res);
        }
        
        Logger::instance().info(std::string("Job not found in MySQL: ") + jobId);
        return std::nullopt;

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error querying job by ID: ") + e.what());
        return std::nullopt;
    }
}

std::vector<Job> MySQLJobRepository::findAll() const noexcept {
    std::vector<Job> jobs;
    try {
        auto conn = Database::instance().getConnection();
        std::string sql = "SELECT id, repository_url, status, created_at, started_at, completed_at FROM jobs ORDER BY created_at DESC";
        
        std::shared_ptr<sql::Statement> stmt(conn->createStatement());
        std::shared_ptr<sql::ResultSet> res(stmt->executeQuery(sql));
        
        while (res->next()) {
            jobs.push_back(buildJobFromResultSet(res));
        }
        
        Logger::instance().info(std::string("Retrieved ") + std::to_string(jobs.size()) + " jobs from MySQL");

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error listing all jobs: ") + e.what());
    }
    return jobs;
}

std::optional<Job> MySQLJobRepository::dequeueNextJob() noexcept {
    try {
        auto conn = Database::instance().getConnection();
        std::string sql = "SELECT id, repository_url, status, created_at, started_at, completed_at FROM jobs WHERE status = ? ORDER BY created_at ASC LIMIT 1";
        
        std::shared_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(sql));
        pstmt->setString(1, "QUEUED");
        
        std::shared_ptr<sql::ResultSet> res(pstmt->executeQuery());
        
        if (res->next()) {
            Logger::instance().info(std::string("Dequeued job from MySQL: ") + std::string(res->getString("id")));
            return buildJobFromResultSet(res);
        }
        
        return std::nullopt;

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error dequeuing job: ") + e.what());
        return std::nullopt;
    }
}

bool MySQLJobRepository::updateStatus(const std::string& jobId, JobStatus newStatus) noexcept {
    try {
        auto conn = Database::instance().getConnection();
        std::string sql = "UPDATE jobs SET status = ? WHERE id = ?";
        
        std::shared_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(sql));
        pstmt->setString(1, std::string(jobStatusToString(newStatus)));
        pstmt->setString(2, jobId);
        
        int affectedRows = pstmt->executeUpdate();
        
        if (affectedRows > 0) {
            Logger::instance().info(std::string("Updated job status in MySQL: ") + jobId + 
                                   " -> " + std::string(jobStatusToString(newStatus)));
            return true;
        }
        
        Logger::instance().warn(std::string("Job not found for status update: ") + jobId);
        return false;

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error updating job status: ") + e.what());
        return false;
    }
}

bool MySQLJobRepository::setStartedAt(const std::string& jobId, 
                                     std::chrono::system_clock::time_point timestamp) noexcept {
    try {
        auto conn = Database::instance().getConnection();
        std::string sql = "UPDATE jobs SET started_at = ? WHERE id = ?";
        
        std::shared_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(sql));
        pstmt->setString(1, timePointToDatetimeString(timestamp));
        pstmt->setString(2, jobId);
        
        int affectedRows = pstmt->executeUpdate();
        
        if (affectedRows > 0) {
            Logger::instance().info(std::string("Updated started_at in MySQL: ") + jobId);
            return true;
        }
        
        return false;

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error updating started_at: ") + e.what());
        return false;
    }
}

bool MySQLJobRepository::setCompletedAt(const std::string& jobId,
                                       std::chrono::system_clock::time_point timestamp) noexcept {
    try {
        auto conn = Database::instance().getConnection();
        std::string sql = "UPDATE jobs SET completed_at = ? WHERE id = ?";
        
        std::shared_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(sql));
        pstmt->setString(1, timePointToDatetimeString(timestamp));
        pstmt->setString(2, jobId);
        
        int affectedRows = pstmt->executeUpdate();
        
        if (affectedRows > 0) {
            Logger::instance().info(std::string("Updated completed_at in MySQL: ") + jobId);
            return true;
        }
        
        return false;

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error updating completed_at: ") + e.what());
        return false;
    }
}

bool MySQLJobRepository::updateCloneInfo(const std::string& jobId,
                                        const std::string& repositoryPath,
                                        long long durationMs,
                                        const char* errorMessage) noexcept {
    try {
        auto conn = Database::instance().getConnection();
        std::string sql = "UPDATE jobs SET repository_path = ?, clone_duration_ms = ?, error_message = ? WHERE id = ?";
        
        std::shared_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(sql));
        pstmt->setString(1, repositoryPath);
        pstmt->setInt64(2, durationMs);
        
        if (errorMessage) {
            pstmt->setString(3, errorMessage);
        } else {
            pstmt->setNull(3, sql::DataType::VARCHAR);
        }
        
        pstmt->setString(4, jobId);
        
        int affectedRows = pstmt->executeUpdate();
        
        if (affectedRows > 0) {
            Logger::instance().info(std::string("Updated clone info in MySQL: ") + jobId);
            return true;
        }
        
        return false;

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error updating clone info: ") + e.what());
        return false;
    }
}

bool MySQLJobRepository::updateError(const std::string& jobId,
                                    const std::string& errorMessage) noexcept {
    try {
        auto conn = Database::instance().getConnection();
        std::string sql = "UPDATE jobs SET status = ?, error_message = ? WHERE id = ?";
        
        std::shared_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(sql));
        pstmt->setString(1, "FAILED");
        pstmt->setString(2, errorMessage);
        pstmt->setString(3, jobId);
        
        int affectedRows = pstmt->executeUpdate();
        
        if (affectedRows > 0) {
            Logger::instance().error(std::string("Job failed: ") + jobId + " - " + errorMessage);
            return true;
        }
        
        return false;

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error updating job error: ") + e.what());
        return false;
    }
}

} // namespace cortex::infrastructure
