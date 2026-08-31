#include "infrastructure/MySQLAnalysisRepository.h"

#include "database/Database.h"
#include "logging/Logger.h"
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <json/json.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace cortex::infrastructure {

using cortex::database::Database;
using cortex::domain::AnalysisResult;
using cortex::logging::Logger;

std::string MySQLAnalysisRepository::toDatetime(
    std::chrono::system_clock::time_point tp) noexcept
{
    try {
        const auto asTimeT = std::chrono::system_clock::to_time_t(tp);
        std::tm tmUtc = *std::gmtime(&asTimeT);
        std::ostringstream out;
        out << std::put_time(&tmUtc, "%Y-%m-%d %H:%M:%S");
        return out.str();
    } catch (...) {
        return "1970-01-01 00:00:00";
    }
}

std::chrono::system_clock::time_point MySQLAnalysisRepository::fromDatetime(
    const std::string& value) noexcept
{
    try {
        std::tm tm = {};
        std::istringstream in(value);
        in >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    } catch (...) {
        return std::chrono::system_clock::now();
    }
}

void MySQLAnalysisRepository::save(const AnalysisResult& result) noexcept {
    try {
        Json::Value languages(Json::objectValue);
        for (const auto& entry : result.languageDistribution) {
            languages[entry.first] = entry.second;
        }

        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        const std::string languagesJson = Json::writeString(writerBuilder, languages);

        const std::string sql =
            "INSERT INTO analysis_results "
            "(job_id, file_count, dir_count, total_lines, language_distribution_json, analyzed_at, clone_path) "
            "VALUES (?, ?, ?, ?, ?, ?, ?) "
            "ON DUPLICATE KEY UPDATE "
            "file_count = VALUES(file_count), "
            "dir_count = VALUES(dir_count), "
            "total_lines = VALUES(total_lines), "
            "language_distribution_json = VALUES(language_distribution_json), "
            "analyzed_at = VALUES(analyzed_at), "
            "clone_path = VALUES(clone_path)";

        std::shared_ptr<sql::Connection> conn = Database::instance().getConnection();
        std::shared_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(sql));

        pstmt->setString(1, result.jobId);
        pstmt->setInt(2, result.fileCount);
        pstmt->setInt(3, result.dirCount);
        pstmt->setInt64(4, result.totalLines);
        pstmt->setString(5, languagesJson);
        pstmt->setString(6, toDatetime(result.analyzedAt));
        pstmt->setString(7, result.clonePath);

        pstmt->executeUpdate();
    } catch (const std::exception& ex) {
        Logger::instance().error(std::string("Error saving analysis result to MySQL: ") + ex.what());
    }
}

std::optional<AnalysisResult> MySQLAnalysisRepository::findByJobId(
    const std::string& jobId) const noexcept
{
    try {
        const std::string sql =
            "SELECT job_id, file_count, dir_count, total_lines, "
            "language_distribution_json, analyzed_at, clone_path "
            "FROM analysis_results WHERE job_id = ?";

        std::shared_ptr<sql::Connection> conn = Database::instance().getConnection();
        std::shared_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(sql));
        pstmt->setString(1, jobId);

        std::shared_ptr<sql::ResultSet> rs(pstmt->executeQuery());
        if (!rs->next()) {
            return std::nullopt;
        }

        AnalysisResult result;
        result.jobId = rs->getString("job_id");
        result.fileCount = rs->getInt("file_count");
        result.dirCount = rs->getInt("dir_count");
        result.totalLines = rs->getInt64("total_lines");
        result.analyzedAt = fromDatetime(rs->getString("analyzed_at"));
        result.clonePath = rs->getString("clone_path");

        const std::string languagesJson = rs->getString("language_distribution_json");
        Json::CharReaderBuilder readerBuilder;
        Json::Value root;
        std::string errors;
        std::istringstream input(languagesJson);

        if (Json::parseFromStream(readerBuilder, input, &root, &errors) && root.isObject()) {
            const std::vector<std::string> keys = root.getMemberNames();
            for (const std::string& key : keys) {
                if (root[key].isInt()) {
                    result.languageDistribution[key] = root[key].asInt();
                }
            }
        }

        return result;
    } catch (const std::exception& ex) {
        Logger::instance().error(std::string("Error loading analysis result from MySQL: ") + ex.what());
        return std::nullopt;
    }
}

} // namespace cortex::infrastructure
