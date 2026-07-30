/**
 * @file Database.h
 * @brief Meyers singleton providing MySQL connectivity, health checking, and graceful shutdown
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

#include <memory>
#include <string>
#include <cppconn/connection.h>

namespace cortex::database {

/**
 * Database singleton for MySQL connectivity
 * 
 * Responsibilities:
 * - Establish MySQL connection
 * - Health checking
 * - Graceful shutdown
 * - Connection verification
 * 
 * Design:
 * - Meyers Singleton pattern (thread-safe)
 * - Lazy initialization
 * - Exception-safe resource management
 */
class Database {
public:
    /**
     * Get singleton instance
     * @return Reference to Database singleton
     */
    static Database& instance() noexcept;

    /**
     * Initialize database connection
     * 
     * @param host MySQL host (default: localhost)
     * @param port MySQL port (default: 3306)
     * @param user MySQL user (default: cortex)
     * @param password MySQL password (default: cortex)
     * @param database Database name (default: cortex)
     * @return true if successful, false otherwise
     * 
     * Call this once during application startup.
     * Throws std::runtime_error if connection fails.
     */
    bool initialize(
        const std::string& host = "localhost",
        int port = 3306,
        const std::string& user = "cortex",
        const std::string& password = "cortex",
        const std::string& database = "cortex"
    );

    /**
     * Get a connection from the pool
     * 
     * @return Shared pointer to sql::Connection
     * @throws std::runtime_error if no connection available
     */
    std::shared_ptr<sql::Connection> getConnection();

    /**
     * Check if database is connected and healthy
     * 
     * @return true if connected and responsive, false otherwise
     */
    bool isHealthy() const noexcept;

    /**
     * Gracefully shutdown database
     * 
     * Closes all pooled connections.
     * Safe to call multiple times.
     */
    void shutdown() noexcept;

    // Delete copy/move
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

private:
    Database() = default;
    ~Database() = default;

    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string database_;
    std::shared_ptr<sql::Connection> connection_;
    bool initialized_;
};

} // namespace cortex::database
