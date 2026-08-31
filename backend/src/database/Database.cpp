/**
 * @file Database.cpp
 * @brief MySQL connection initialization, SELECT 1 health checking, and graceful shutdown
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

#include "database/Database.h"
#include "logging/Logger.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <mysql_driver.h>
#include <stdexcept>
#include <cstdlib>
#include <chrono>
#include <thread>

namespace cortex::database {

using cortex::logging::Logger;

Database& Database::instance() noexcept {
    static Database db;
    return db;
}

bool Database::initialize(
    const std::string& host,
    int port,
    const std::string& user,
    const std::string& password,
    const std::string& database)
{
    host_ = host;
    port_ = port;
    user_ = user;
    password_ = password;
    database_ = database;

    int attempts = 5;
    int retryDelayMs = 2000;

    if (const char* rawAttempts = std::getenv("MYSQL_CONNECT_RETRIES")) {
        try {
            const int parsed = std::stoi(rawAttempts);
            if (parsed > 0) {
                attempts = parsed;
            }
        } catch (...) {
            // Keep defaults if parsing fails.
        }
    }

    if (const char* rawDelayMs = std::getenv("MYSQL_CONNECT_RETRY_DELAY_MS")) {
        try {
            const int parsed = std::stoi(rawDelayMs);
            if (parsed >= 0) {
                retryDelayMs = parsed;
            }
        } catch (...) {
            // Keep defaults if parsing fails.
        }
    }

    Logger::instance().info("Connecting to MySQL database...");
    Logger::instance().info(std::string("Host: ") + host_ + ":" + std::to_string(port_));
    Logger::instance().info(std::string("Database: ") + database_);
    Logger::instance().info(std::string("Connect retries: ") + std::to_string(attempts));

    for (int attempt = 1; attempt <= attempts; ++attempt) {
        try {
            // Get MySQL driver
            sql::Driver* driver = sql::mysql::get_mysql_driver_instance();

            // Create connection
            std::string url = std::string("tcp://") + host_ + ":" + std::to_string(port_);
            std::shared_ptr<sql::Connection> connection(
                driver->connect(url, user_, password_)
            );

            // Select database
            connection->setSchema(database_);

            // Test connection
            std::shared_ptr<sql::Statement> stmt(connection->createStatement());
            std::shared_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT 1"));
            while (rs->next()) {
                // fully consume result
            }

            initialized_.store(true);
            Logger::instance().info("Database connected successfully");
            return true;

        } catch (sql::SQLException& e) {
            Logger::instance().error(
                std::string("MySQL connect attempt ") + std::to_string(attempt) +
                "/" + std::to_string(attempts) + " failed: " + e.what());
            Logger::instance().error(std::string("SQLState: ") + e.getSQLState());
            Logger::instance().error(std::string("Error Code: ") + std::to_string(e.getErrorCode()));
        } catch (const std::exception& e) {
            Logger::instance().error(
                std::string("Database connect attempt ") + std::to_string(attempt) +
                "/" + std::to_string(attempts) + " failed: " + e.what());
        }

        if (attempt < attempts && retryDelayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
        }
    }

    initialized_.store(false);
    return false;
}

std::shared_ptr<sql::Connection> Database::getConnection() {
    if (!initialized_.load()) {
        throw std::runtime_error("Database not initialized");
    }

    thread_local std::shared_ptr<sql::Connection> threadConnection;

    auto connectFresh = [this]() {
        sql::Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::string url = std::string("tcp://") + host_ + ":" + std::to_string(port_);
        std::shared_ptr<sql::Connection> conn(driver->connect(url, user_, password_));
        conn->setSchema(database_);
        return conn;
    };

    if (!threadConnection) {
        threadConnection = connectFresh();
    } else {
        try {
            if (threadConnection->isClosed()) {
                threadConnection = connectFresh();
            }
        } catch (...) {
            threadConnection = connectFresh();
        }
    }

    return threadConnection;
}

bool Database::isHealthy() const noexcept {
    try {
        if (!initialized_.load()) {
            return false;
        }

        std::shared_ptr<sql::Connection> conn = Database::instance().getConnection();
        return conn && !conn->isClosed() && conn->isValid();

    } catch (...) {
        return false;
    }
}

void Database::shutdown() noexcept {
    try {
        initialized_.store(false);
        Logger::instance().info("Database connection closed");
    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error closing database: ") + e.what());
    }
}

} // namespace cortex::database
