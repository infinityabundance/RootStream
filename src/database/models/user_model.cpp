/**
 * @file user_model.cpp
 * @brief Implementation of user model
 */

#include "user_model.h"
#include <iostream>
#include <chrono>
#include <sstream>

namespace rootstream {
namespace database {
namespace models {

User::User() : loaded_(false) {}

User::~User() {}

int User::createUser(DatabaseManager& db, 
                     const std::string& username,
                     const std::string& email,
                     const std::string& passwordHash) {
    try {
        std::stringstream query;
        query << "INSERT INTO users (username, email, password_hash, is_active, is_verified) "
              << "VALUES ('" << username << "', '" << email << "', '" 
              << passwordHash << "', true, false) RETURNING id";
        
        auto result = db.executeSelect(query.str());
        
        if (result.size() > 0) {
            std::cout << "User created successfully with ID: " << result[0][0].c_str() << std::endl;
            return 0;
        }
        
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create user: " << e.what() << std::endl;
        return -1;
    }
}

int User::load(DatabaseManager& db, uint32_t userId) {
    try {
        std::stringstream query;
        query << "SELECT id, username, email, password_hash, display_name, avatar_url, "
              << "is_verified, is_active, "
              << "EXTRACT(EPOCH FROM created_at) * 1000000 as created_at_us, "
              << "EXTRACT(EPOCH FROM updated_at) * 1000000 as updated_at_us, "
              << "EXTRACT(EPOCH FROM last_login_at) * 1000000 as last_login_us "
              << "FROM users WHERE id = " << userId;
        
        auto result = db.executeSelect(query.str());
        
        if (result.size() == 0) {
            std::cerr << "User not found: " << userId << std::endl;
            return -1;
        }
        
        auto row = result[0];
        data_.id = std::stoul(row["id"].c_str());
        data_.username = row["username"].c_str();
        data_.email = row["email"].c_str();
        data_.password_hash = row["password_hash"].c_str();
        data_.display_name = row["display_name"].is_null() ? "" : row["display_name"].c_str();
        data_.avatar_url = row["avatar_url"].is_null() ? "" : row["avatar_url"].c_str();
        data_.is_verified = strcmp(row["is_verified"].c_str(), "t") == 0;
        data_.is_active = strcmp(row["is_active"].c_str(), "t") == 0;
        data_.created_at_us = row["created_at_us"].is_null() ? 0 : std::stoull(row["created_at_us"].c_str());
        data_.updated_at_us = row["updated_at_us"].is_null() ? 0 : std::stoull(row["updated_at_us"].c_str());
        data_.last_login_us = row["last_login_us"].is_null() ? 0 : std::stoull(row["last_login_us"].c_str());
        
        loaded_ = true;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load user: " << e.what() << std::endl;
        return -1;
    }
}

int User::loadByUsername(DatabaseManager& db, const std::string& username) {
    try {
        std::stringstream query;
        query << "SELECT id, username, email, password_hash, display_name, avatar_url, "
              << "is_verified, is_active, "
              << "EXTRACT(EPOCH FROM created_at) * 1000000 as created_at_us, "
              << "EXTRACT(EPOCH FROM updated_at) * 1000000 as updated_at_us, "
              << "EXTRACT(EPOCH FROM last_login_at) * 1000000 as last_login_us "
              << "FROM users WHERE username = '" << username << "'";
        
        auto result = db.executeSelect(query.str());
        
        if (result.size() == 0) {
            std::cerr << "User not found: " << username << std::endl;
            return -1;
        }
        
        auto row = result[0];
        data_.id = std::stoul(row["id"].c_str());
        data_.username = row["username"].c_str();
        data_.email = row["email"].c_str();
        data_.password_hash = row["password_hash"].c_str();
        data_.display_name = row["display_name"].is_null() ? "" : row["display_name"].c_str();
        data_.avatar_url = row["avatar_url"].is_null() ? "" : row["avatar_url"].c_str();
        data_.is_verified = strcmp(row["is_verified"].c_str(), "t") == 0;
        data_.is_active = strcmp(row["is_active"].c_str(), "t") == 0;
        data_.created_at_us = row["created_at_us"].is_null() ? 0 : std::stoull(row["created_at_us"].c_str());
        data_.updated_at_us = row["updated_at_us"].is_null() ? 0 : std::stoull(row["updated_at_us"].c_str());
        data_.last_login_us = row["last_login_us"].is_null() ? 0 : std::stoull(row["last_login_us"].c_str());
        
        loaded_ = true;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load user by username: " << e.what() << std::endl;
        return -1;
    }
}

int User::loadByEmail(DatabaseManager& db, const std::string& email) {
    try {
        std::stringstream query;
        query << "SELECT id, username, email, password_hash, display_name, avatar_url, "
              << "is_verified, is_active, "
              << "EXTRACT(EPOCH FROM created_at) * 1000000 as created_at_us, "
              << "EXTRACT(EPOCH FROM updated_at) * 1000000 as updated_at_us, "
              << "EXTRACT(EPOCH FROM last_login_at) * 1000000 as last_login_us "
              << "FROM users WHERE email = '" << email << "'";
        
        auto result = db.executeSelect(query.str());
        
        if (result.size() == 0) {
            std::cerr << "User not found: " << email << std::endl;
            return -1;
        }
        
        auto row = result[0];
        data_.id = std::stoul(row["id"].c_str());
        data_.username = row["username"].c_str();
        data_.email = row["email"].c_str();
        data_.password_hash = row["password_hash"].c_str();
        data_.display_name = row["display_name"].is_null() ? "" : row["display_name"].c_str();
        data_.avatar_url = row["avatar_url"].is_null() ? "" : row["avatar_url"].c_str();
        data_.is_verified = strcmp(row["is_verified"].c_str(), "t") == 0;
        data_.is_active = strcmp(row["is_active"].c_str(), "t") == 0;
        data_.created_at_us = row["created_at_us"].is_null() ? 0 : std::stoull(row["created_at_us"].c_str());
        data_.updated_at_us = row["updated_at_us"].is_null() ? 0 : std::stoull(row["updated_at_us"].c_str());
        data_.last_login_us = row["last_login_us"].is_null() ? 0 : std::stoull(row["last_login_us"].c_str());
        
        loaded_ = true;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load user by email: " << e.what() << std::endl;
        return -1;
    }
}

int User::save(DatabaseManager& db) {
    if (!loaded_) {
        std::cerr << "Cannot save unloaded user" << std::endl;
        return -1;
    }
    
    try {
        std::stringstream query;
        query << "UPDATE users SET "
              << "username = '" << data_.username << "', "
              << "email = '" << data_.email << "', "
              << "display_name = " << (data_.display_name.empty() ? "NULL" : "'" + data_.display_name + "'") << ", "
              << "avatar_url = " << (data_.avatar_url.empty() ? "NULL" : "'" + data_.avatar_url + "'") << ", "
              << "is_verified = " << (data_.is_verified ? "true" : "false") << ", "
              << "is_active = " << (data_.is_active ? "true" : "false") << " "
              << "WHERE id = " << data_.id;
        
        int result = db.executeQuery(query.str());
        return (result >= 0) ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save user: " << e.what() << std::endl;
        return -1;
    }
}

int User::updateLastLogin(DatabaseManager& db) {
    if (!loaded_) {
        std::cerr << "Cannot update last login for unloaded user" << std::endl;
        return -1;
    }
    
    try {
        std::stringstream query;
        query << "UPDATE users SET last_login_at = CURRENT_TIMESTAMP WHERE id = " << data_.id;
        
        int result = db.executeQuery(query.str());
        if (result >= 0) {
            auto now = std::chrono::system_clock::now();
            data_.last_login_us = std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()).count();
        }
        return (result >= 0) ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to update last login: " << e.what() << std::endl;
        return -1;
    }
}

int User::updateProfile(DatabaseManager& db, const UserData& newData) {
    data_.display_name = newData.display_name;
    data_.avatar_url = newData.avatar_url;
    return save(db);
}

int User::verifyAccount(DatabaseManager& db) {
    if (!loaded_) {
        return -1;
    }
    
    try {
        std::stringstream query;
        query << "UPDATE users SET is_verified = true WHERE id = " << data_.id;
        
        int result = db.executeQuery(query.str());
        if (result >= 0) {
            data_.is_verified = true;
        }
        return (result >= 0) ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to verify account: " << e.what() << std::endl;
        return -1;
    }
}

int User::deactivate(DatabaseManager& db) {
    if (!loaded_) {
        return -1;
    }
    
    try {
        std::stringstream query;
        query << "UPDATE users SET is_active = false WHERE id = " << data_.id;
        
        int result = db.executeQuery(query.str());
        if (result >= 0) {
            data_.is_active = false;
        }
        return (result >= 0) ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to deactivate account: " << e.what() << std::endl;
        return -1;
    }
}

int User::deleteUser(DatabaseManager& db) {
    if (!loaded_) {
        return -1;
    }
    
    try {
        std::stringstream query;
        query << "DELETE FROM users WHERE id = " << data_.id;
        
        int result = db.executeQuery(query.str());
        if (result >= 0) {
            loaded_ = false;
        }
        return (result >= 0) ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to delete user: " << e.what() << std::endl;
        return -1;
    }
}

bool User::validatePassword(const std::string& password) const {
    // Note: In production, this should use proper password hashing (bcrypt, argon2, etc.)
    // This is a simplified implementation
    return false; // Placeholder
}

} // namespace models
} // namespace database
} // namespace rootstream
