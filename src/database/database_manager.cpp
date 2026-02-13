/**
 * @file database_manager.cpp
 * @brief Implementation of database connection and management
 */

#include "database_manager.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace rootstream {
namespace database {

// ============================================================================
// Connection Implementation
// ============================================================================

Connection::Connection(const std::string& connStr) {
    try {
        conn_ = std::make_unique<pqxx::connection>(connStr);
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to database: " << e.what() << std::endl;
        throw;
    }
}

Connection::~Connection() {
    if (conn_) {
        conn_->close();
    }
}

bool Connection::isConnected() const {
    return conn_ && conn_->is_open();
}

// ============================================================================
// Transaction Implementation
// ============================================================================

Transaction::Transaction(Connection& conn) 
    : conn_(conn), committed_(false) {
    txn_ = std::make_unique<pqxx::work>(conn_.get());
}

Transaction::~Transaction() {
    if (!committed_) {
        try {
            txn_->abort();
        } catch (const std::exception& e) {
            std::cerr << "Error aborting transaction: " << e.what() << std::endl;
        }
    }
}

void Transaction::commit() {
    if (!committed_) {
        txn_->commit();
        committed_ = true;
    }
}

void Transaction::rollback() {
    if (!committed_) {
        txn_->abort();
        committed_ = true;
    }
}

pqxx::result Transaction::exec(const std::string& query) {
    return txn_->exec(query);
}

pqxx::result Transaction::exec_params(const std::string& query, 
                                      const std::vector<std::string>& params) {
    pqxx::params p;
    for (const auto& param : params) {
        p.append(param);
    }
    return txn_->exec_params(query, p);
}

// ============================================================================
// ConnectionPool Implementation
// ============================================================================

ConnectionPool::ConnectionPool(const std::string& connStr, size_t poolSize)
    : connStr_(connStr), poolSize_(poolSize) {
    
    // Pre-create connections
    for (size_t i = 0; i < poolSize; ++i) {
        try {
            auto conn = std::make_shared<Connection>(connStr_);
            available_.push(conn);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create connection " << i << ": " << e.what() << std::endl;
        }
    }
}

ConnectionPool::~ConnectionPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!available_.empty()) {
        available_.pop();
    }
}

std::shared_ptr<Connection> ConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait for an available connection
    cv_.wait(lock, [this] { return !available_.empty(); });
    
    auto conn = available_.front();
    available_.pop();
    return conn;
}

void ConnectionPool::release(std::shared_ptr<Connection> conn) {
    if (!conn) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    available_.push(conn);
    cv_.notify_one();
}

size_t ConnectionPool::availableCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return available_.size();
}

// ============================================================================
// DatabaseManager Implementation
// ============================================================================

DatabaseManager::DatabaseManager() : initialized_(false) {}

DatabaseManager::~DatabaseManager() {
    cleanup();
}

int DatabaseManager::init(const std::string& connStr, size_t poolSize) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        std::cerr << "DatabaseManager already initialized" << std::endl;
        return -1;
    }
    
    try {
        connectionString_ = connStr;
        pool_ = std::make_unique<ConnectionPool>(connStr, poolSize);
        initialized_ = true;
        
        std::cout << "DatabaseManager initialized with pool size: " << poolSize << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize DatabaseManager: " << e.what() << std::endl;
        return -1;
    }
}

int DatabaseManager::executeQuery(const std::string& query) {
    if (!initialized_) {
        std::cerr << "DatabaseManager not initialized" << std::endl;
        return -1;
    }
    
    try {
        auto conn = pool_->acquire();
        Transaction txn(*conn);
        auto result = txn.exec(query);
        txn.commit();
        pool_->release(conn);
        
        return static_cast<int>(result.affected_rows());
    } catch (const std::exception& e) {
        std::cerr << "Query execution failed: " << e.what() << std::endl;
        return -1;
    }
}

pqxx::result DatabaseManager::executeSelect(const std::string& query) {
    if (!initialized_) {
        throw std::runtime_error("DatabaseManager not initialized");
    }
    
    auto conn = pool_->acquire();
    Transaction txn(*conn);
    auto result = txn.exec(query);
    txn.commit();
    pool_->release(conn);
    
    return result;
}

pqxx::result DatabaseManager::executeParams(const std::string& query, 
                                            const std::vector<std::string>& params) {
    if (!initialized_) {
        throw std::runtime_error("DatabaseManager not initialized");
    }
    
    auto conn = pool_->acquire();
    Transaction txn(*conn);
    auto result = txn.exec_params(query, params);
    txn.commit();
    pool_->release(conn);
    
    return result;
}

int DatabaseManager::runMigrations(const std::string& migrationsPath) {
    if (!initialized_) {
        std::cerr << "DatabaseManager not initialized" << std::endl;
        return -1;
    }
    
    try {
        std::vector<std::string> migrationFiles;
        
        // Collect all .sql files
        for (const auto& entry : fs::directory_iterator(migrationsPath)) {
            if (entry.path().extension() == ".sql") {
                migrationFiles.push_back(entry.path().string());
            }
        }
        
        // Sort migration files by name
        std::sort(migrationFiles.begin(), migrationFiles.end());
        
        // Execute each migration
        for (const auto& file : migrationFiles) {
            std::cout << "Running migration: " << file << std::endl;
            
            std::ifstream sqlFile(file);
            if (!sqlFile.is_open()) {
                std::cerr << "Failed to open migration file: " << file << std::endl;
                return -1;
            }
            
            std::string sql((std::istreambuf_iterator<char>(sqlFile)),
                           std::istreambuf_iterator<char>());
            sqlFile.close();
            
            if (executeQuery(sql) < 0) {
                std::cerr << "Failed to execute migration: " << file << std::endl;
                return -1;
            }
        }
        
        std::cout << "All migrations completed successfully" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Migration failed: " << e.what() << std::endl;
        return -1;
    }
}

bool DatabaseManager::isConnected() {
    if (!initialized_ || !pool_) {
        return false;
    }
    
    try {
        auto conn = pool_->acquire();
        bool connected = conn->isConnected();
        pool_->release(conn);
        return connected;
    } catch (const std::exception& e) {
        std::cerr << "Error checking connection: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<Connection> DatabaseManager::getConnection() {
    if (!initialized_) {
        throw std::runtime_error("DatabaseManager not initialized");
    }
    return pool_->acquire();
}

void DatabaseManager::releaseConnection(std::shared_ptr<Connection> conn) {
    if (pool_) {
        pool_->release(conn);
    }
}

void DatabaseManager::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        pool_.reset();
        initialized_ = false;
        std::cout << "DatabaseManager cleaned up" << std::endl;
    }
}

} // namespace database
} // namespace rootstream

// ============================================================================
// C API Implementation
// ============================================================================

using namespace rootstream::database;

struct DatabaseManager {
    rootstream::database::DatabaseManager* manager;
};

int database_manager_init(DatabaseManager** manager, const char* connStr, size_t poolSize) {
    if (!manager || !connStr) {
        return -1;
    }
    
    try {
        *manager = new DatabaseManager();
        (*manager)->manager = new rootstream::database::DatabaseManager();
        return (*manager)->manager->init(connStr, poolSize);
    } catch (const std::exception& e) {
        std::cerr << "C API init failed: " << e.what() << std::endl;
        return -1;
    }
}

int database_manager_execute(DatabaseManager* manager, const char* query) {
    if (!manager || !manager->manager || !query) {
        return -1;
    }
    
    try {
        return manager->manager->executeQuery(query);
    } catch (const std::exception& e) {
        std::cerr << "C API execute failed: " << e.what() << std::endl;
        return -1;
    }
}

int database_manager_query(DatabaseManager* manager, const char* query, char** resultJson) {
    if (!manager || !manager->manager || !query || !resultJson) {
        return -1;
    }
    
    try {
        auto result = manager->manager->executeSelect(query);
        
        json j = json::array();
        for (const auto& row : result) {
            json rowObj = json::object();
            for (size_t i = 0; i < row.size(); ++i) {
                rowObj[row[i].name()] = row[i].c_str();
            }
            j.push_back(rowObj);
        }
        
        std::string jsonStr = j.dump();
        *resultJson = strdup(jsonStr.c_str());
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "C API query failed: " << e.what() << std::endl;
        return -1;
    }
}

int database_manager_run_migrations(DatabaseManager* manager, const char* migrationsPath) {
    if (!manager || !manager->manager || !migrationsPath) {
        return -1;
    }
    
    try {
        return manager->manager->runMigrations(migrationsPath);
    } catch (const std::exception& e) {
        std::cerr << "C API migrations failed: " << e.what() << std::endl;
        return -1;
    }
}

int database_manager_is_connected(DatabaseManager* manager) {
    if (!manager || !manager->manager) {
        return 0;
    }
    
    return manager->manager->isConnected() ? 1 : 0;
}

void database_manager_cleanup(DatabaseManager* manager) {
    if (manager) {
        if (manager->manager) {
            manager->manager->cleanup();
            delete manager->manager;
        }
        delete manager;
    }
}
