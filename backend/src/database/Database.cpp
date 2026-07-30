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
#include <mysql_driver.h>
#include <stdexcept>

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
    try {
        host_ = host;
        port_ = port;
        user_ = user;
        password_ = password;
        database_ = database;

        Logger::instance().info("Connecting to MySQL database...");
        Logger::instance().info(std::string("Host: ") + host + ":" + std::to_string(port));
        Logger::instance().info(std::string("Database: ") + database);

        // Get MySQL driver
        sql::Driver* driver = sql::mysql::get_mysql_driver_instance();

        // Create connection
        std::string url = std::string("tcp://") + host + ":" + std::to_string(port);
        connection_ = std::shared_ptr<sql::Connection>(
            driver->connect(url, user, password)
        );

        // Select database
        connection_->setSchema(database);

        // Test connection
        std::shared_ptr<sql::Statement> stmt(connection_->createStatement());
        stmt->execute("SELECT 1");

        initialized_ = true;
        Logger::instance().info("Database connected successfully");
        return true;

    } catch (sql::SQLException& e) {
        Logger::instance().error(std::string("MySQL Error: ") + e.what());
        Logger::instance().error(std::string("SQLState: ") + e.getSQLState());
        Logger::instance().error(std::string("Error Code: ") + std::to_string(e.getErrorCode()));
        initialized_ = false;
        return false;

    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Database initialization error: ") + e.what());
        initialized_ = false;
        return false;
    }
}

std::shared_ptr<sql::Connection> Database::getConnection() {
    if (!initialized_ || !connection_) {
        throw std::runtime_error("Database not initialized");
    }
    return connection_;
}

bool Database::isHealthy() const noexcept {
    try {
        if (!initialized_ || !connection_) {
            return false;
        }
        
        // Test connection with simple query
        std::shared_ptr<sql::Statement> stmt(connection_->createStatement());
        stmt->execute("SELECT 1");
        return true;

    } catch (...) {
        return false;
    }
}

void Database::shutdown() noexcept {
    try {
        if (connection_) {
            connection_->close();
        }
        initialized_ = false;
        Logger::instance().info("Database connection closed");
    } catch (const std::exception& e) {
        Logger::instance().error(std::string("Error closing database: ") + e.what());
    }
}

} // namespace cortex::database
