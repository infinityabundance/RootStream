/**
 * @file database_manager.h
 * @brief Database connection and management for RootStream
 * 
 * Provides PostgreSQL database connection pooling, transaction management,
 * and schema migration support.
 */

#ifndef ROOTSTREAM_DATABASE_MANAGER_H
#define ROOTSTREAM_DATABASE_MANAGER_H

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>

#ifdef __cplusplus
extern "C" {
#endif

// C-compatible opaque handle for C code
typedef struct DatabaseManager DatabaseManager;

/**
 * Initialize database manager with connection string
 * @param connStr PostgreSQL connection string (e.g., "postgresql://user:pass@localhost/dbname")
 * @param poolSize Maximum number of connections in the pool
 * @return 0 on success, negative on error
 */
int database_manager_init(DatabaseManager** manager, const char* connStr, size_t poolSize);

/**
 * Execute a query (INSERT, UPDATE, DELETE)
 * @param manager Database manager instance
 * @param query SQL query string
 * @return Number of rows affected, or negative on error
 */
int database_manager_execute(DatabaseManager* manager, const char* query);

/**
 * Execute a SELECT query and return result
 * @param manager Database manager instance
 * @param query SQL query string
 * @param resultJson Output buffer for JSON result (caller must free)
 * @return 0 on success, negative on error
 */
int database_manager_query(DatabaseManager* manager, const char* query, char** resultJson);

/**
 * Run database migrations from a directory
 * @param manager Database manager instance
 * @param migrationsPath Path to directory containing migration SQL files
 * @return 0 on success, negative on error
 */
int database_manager_run_migrations(DatabaseManager* manager, const char* migrationsPath);

/**
 * Check if database connection is healthy
 * @param manager Database manager instance
 * @return 1 if connected, 0 if not
 */
int database_manager_is_connected(DatabaseManager* manager);

/**
 * Cleanup and destroy database manager
 * @param manager Database manager instance to cleanup
 */
void database_manager_cleanup(DatabaseManager* manager);

#ifdef __cplusplus
}
#endif

// C++ interface (when compiled as C++)
#ifdef __cplusplus

#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

namespace rootstream {
namespace database {

/**
 * Connection wrapper for C++ usage
 */
class Connection {
public:
    Connection(const std::string& connStr);
    ~Connection();
    
    pqxx::connection& get() { return *conn_; }
    bool isConnected() const;
    
private:
    std::unique_ptr<pqxx::connection> conn_;
};

/**
 * Transaction wrapper for RAII-style transaction management
 */
class Transaction {
public:
    Transaction(Connection& conn);
    ~Transaction();
    
    void commit();
    void rollback();
    
    pqxx::result exec(const std::string& query);
    pqxx::result exec_params(const std::string& query, const std::vector<std::string>& params);
    
private:
    Connection& conn_;
    std::unique_ptr<pqxx::work> txn_;
    bool committed_;
};

/**
 * Connection pool for managing multiple database connections
 */
class ConnectionPool {
public:
    ConnectionPool(const std::string& connStr, size_t poolSize);
    ~ConnectionPool();
    
    // Get a connection from the pool (blocks if none available)
    std::shared_ptr<Connection> acquire();
    
    // Return a connection to the pool
    void release(std::shared_ptr<Connection> conn);
    
    size_t availableCount() const;
    size_t totalCount() const { return poolSize_; }
    
private:
    std::string connStr_;
    size_t poolSize_;
    std::queue<std::shared_ptr<Connection>> available_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

/**
 * Main database manager class
 */
class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();
    
    /**
     * Initialize database with connection string and pool size
     * @param connStr PostgreSQL connection string
     * @param poolSize Number of connections to maintain in pool
     * @return 0 on success, negative on error
     */
    int init(const std::string& connStr, size_t poolSize = 20);
    
    /**
     * Execute a non-SELECT query (INSERT, UPDATE, DELETE)
     * @param query SQL query string
     * @return Number of rows affected, or negative on error
     */
    int executeQuery(const std::string& query);
    
    /**
     * Execute a SELECT query
     * @param query SQL query string
     * @return Query result
     */
    pqxx::result executeSelect(const std::string& query);
    
    /**
     * Execute a parameterized query
     * @param query SQL query with $1, $2... placeholders
     * @param params Parameter values
     * @return Query result
     */
    pqxx::result executeParams(const std::string& query, const std::vector<std::string>& params);
    
    /**
     * Run database migrations from a directory
     * @param migrationsPath Path to directory containing .sql migration files
     * @return 0 on success, negative on error
     */
    int runMigrations(const std::string& migrationsPath);
    
    /**
     * Check if database is connected and healthy
     * @return true if connected
     */
    bool isConnected();
    
    /**
     * Get a connection from the pool for manual transaction management
     * @return Shared pointer to connection
     */
    std::shared_ptr<Connection> getConnection();
    
    /**
     * Release a connection back to the pool
     * @param conn Connection to release
     */
    void releaseConnection(std::shared_ptr<Connection> conn);
    
    /**
     * Cleanup resources
     */
    void cleanup();
    
private:
    std::unique_ptr<ConnectionPool> pool_;
    std::string connectionString_;
    bool initialized_;
    std::mutex mutex_;
};

} // namespace database
} // namespace rootstream

#endif // __cplusplus

#endif // ROOTSTREAM_DATABASE_MANAGER_H
