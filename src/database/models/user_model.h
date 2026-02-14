/**
 * @file user_model.h
 * @brief User data model and authentication for RootStream
 */

#ifndef ROOTSTREAM_USER_MODEL_H
#define ROOTSTREAM_USER_MODEL_H

#include <string>
#include <cstdint>

#ifdef __cplusplus

#include "../database/database_manager.h"

namespace rootstream {
namespace database {
namespace models {

/**
 * User model for managing user accounts
 */
class User {
public:
    struct UserData {
        uint32_t id;
        std::string username;
        std::string email;
        std::string password_hash;
        std::string display_name;
        std::string avatar_url;
        bool is_verified;
        bool is_active;
        uint64_t created_at_us;
        uint64_t updated_at_us;
        uint64_t last_login_us;
        
        UserData() : id(0), is_verified(false), is_active(true),
                     created_at_us(0), updated_at_us(0), last_login_us(0) {}
    };
    
    User();
    ~User();
    
    /**
     * Create a new user in database
     * @param db Database manager
     * @param username User's username (unique)
     * @param email User's email (unique)
     * @param passwordHash Hashed password
     * @return 0 on success, negative on error
     */
    static int createUser(DatabaseManager& db, 
                          const std::string& username,
                          const std::string& email,
                          const std::string& passwordHash);
    
    /**
     * Load user data by user ID
     * @param db Database manager
     * @param userId User ID to load
     * @return 0 on success, negative on error
     */
    int load(DatabaseManager& db, uint32_t userId);
    
    /**
     * Load user data by username
     * @param db Database manager
     * @param username Username to search for
     * @return 0 on success, negative on error
     */
    int loadByUsername(DatabaseManager& db, const std::string& username);
    
    /**
     * Load user data by email
     * @param db Database manager
     * @param email Email to search for
     * @return 0 on success, negative on error
     */
    int loadByEmail(DatabaseManager& db, const std::string& email);
    
    /**
     * Save current user data to database
     * @param db Database manager
     * @return 0 on success, negative on error
     */
    int save(DatabaseManager& db);
    
    /**
     * Update last login timestamp
     * @param db Database manager
     * @return 0 on success, negative on error
     */
    int updateLastLogin(DatabaseManager& db);
    
    /**
     * Update user profile
     * @param db Database manager
     * @param newData Updated user data
     * @return 0 on success, negative on error
     */
    int updateProfile(DatabaseManager& db, const UserData& newData);
    
    /**
     * Verify user account
     * @param db Database manager
     * @return 0 on success, negative on error
     */
    int verifyAccount(DatabaseManager& db);
    
    /**
     * Deactivate user account
     * @param db Database manager
     * @return 0 on success, negative on error
     */
    int deactivate(DatabaseManager& db);
    
    /**
     * Delete user from database
     * @param db Database manager
     * @return 0 on success, negative on error
     */
    int deleteUser(DatabaseManager& db);
    
    /**
     * Validate password against stored hash
     * @param password Plain text password to check
     * @return true if password matches
     */
    bool validatePassword(const std::string& password) const;
    
    /**
     * Get user data
     * @return Reference to user data
     */
    const UserData& getData() const { return data_; }
    
    /**
     * Get user ID
     * @return User ID
     */
    uint32_t getId() const { return data_.id; }
    
    /**
     * Get username
     * @return Username
     */
    const std::string& getUsername() const { return data_.username; }
    
    /**
     * Get email
     * @return Email address
     */
    const std::string& getEmail() const { return data_.email; }
    
    /**
     * Check if user is verified
     * @return true if verified
     */
    bool isVerified() const { return data_.is_verified; }
    
    /**
     * Check if user is active
     * @return true if active
     */
    bool isActive() const { return data_.is_active; }
    
private:
    UserData data_;
    bool loaded_;
};

} // namespace models
} // namespace database
} // namespace rootstream

#endif // __cplusplus

#endif // ROOTSTREAM_USER_MODEL_H
