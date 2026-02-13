/**
 * @file redis_client.h
 * @brief Redis caching and pub/sub client for RootStream
 * 
 * Provides key-value operations, hash operations, list operations,
 * and pub/sub functionality for real-time state synchronization.
 */

#ifndef ROOTSTREAM_REDIS_CLIENT_H
#define ROOTSTREAM_REDIS_CLIENT_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif

// C-compatible opaque handle
typedef struct RedisClient RedisClient;

/**
 * Initialize Redis client
 * @param client Output pointer for client handle
 * @param host Redis server host
 * @param port Redis server port
 * @return 0 on success, negative on error
 */
int redis_client_init(RedisClient** client, const char* host, uint16_t port);

/**
 * Set a key-value pair
 * @param client Redis client handle
 * @param key Key to set
 * @param value Value to set
 * @param ttl_seconds TTL in seconds (0 for no expiration)
 * @return 0 on success, negative on error
 */
int redis_client_set(RedisClient* client, const char* key, const char* value, uint32_t ttl_seconds);

/**
 * Get a value by key
 * @param client Redis client handle
 * @param key Key to get
 * @param value Output buffer (caller must free)
 * @return 0 on success, negative on error
 */
int redis_client_get(RedisClient* client, const char* key, char** value);

/**
 * Delete a key
 * @param client Redis client handle
 * @param key Key to delete
 * @return 0 on success, negative on error
 */
int redis_client_del(RedisClient* client, const char* key);

/**
 * Check if database connection is healthy
 * @param client Redis client handle
 * @return 1 if connected, 0 if not
 */
int redis_client_is_connected(RedisClient* client);

/**
 * Cleanup and destroy Redis client
 * @param client Redis client to cleanup
 */
void redis_client_cleanup(RedisClient* client);

#ifdef __cplusplus
}
#endif

// C++ interface
#ifdef __cplusplus

#include <hiredis/hiredis.h>

namespace rootstream {
namespace cache {

/**
 * Redis client for caching and pub/sub
 */
class RedisClient {
public:
    RedisClient();
    ~RedisClient();
    
    /**
     * Initialize connection to Redis server
     * @param host Redis server host
     * @param port Redis server port (default 6379)
     * @return 0 on success, negative on error
     */
    int init(const std::string& host, uint16_t port = 6379);
    
    // ========================================================================
    // Key-Value Operations
    // ========================================================================
    
    /**
     * Set a key-value pair
     * @param key Key to set
     * @param value Value to set
     * @param ttl_seconds TTL in seconds (0 for no expiration)
     * @return 0 on success, negative on error
     */
    int set(const std::string& key, const std::string& value, uint32_t ttl_seconds = 0);
    
    /**
     * Get value by key
     * @param key Key to retrieve
     * @param value Output parameter for value
     * @return 0 on success, negative on error (key not found)
     */
    int get(const std::string& key, std::string& value);
    
    /**
     * Delete a key
     * @param key Key to delete
     * @return 0 on success, negative on error
     */
    int del(const std::string& key);
    
    /**
     * Check if key exists
     * @param key Key to check
     * @return 1 if exists, 0 if not, negative on error
     */
    int exists(const std::string& key);
    
    // ========================================================================
    // Hash Operations
    // ========================================================================
    
    /**
     * Set a field in a hash
     * @param key Hash key
     * @param field Field name
     * @param value Field value
     * @return 0 on success, negative on error
     */
    int hset(const std::string& key, const std::string& field, const std::string& value);
    
    /**
     * Get a field from a hash
     * @param key Hash key
     * @param field Field name
     * @param value Output parameter for field value
     * @return 0 on success, negative on error
     */
    int hget(const std::string& key, const std::string& field, std::string& value);
    
    /**
     * Delete a field from a hash
     * @param key Hash key
     * @param field Field name
     * @return 0 on success, negative on error
     */
    int hdel(const std::string& key, const std::string& field);
    
    /**
     * Get all fields and values from a hash
     * @param key Hash key
     * @param data Output map of field -> value
     * @return 0 on success, negative on error
     */
    int hgetall(const std::string& key, std::map<std::string, std::string>& data);
    
    // ========================================================================
    // List Operations
    // ========================================================================
    
    /**
     * Push value to left side of list
     * @param key List key
     * @param value Value to push
     * @return 0 on success, negative on error
     */
    int lpush(const std::string& key, const std::string& value);
    
    /**
     * Pop value from right side of list
     * @param key List key
     * @param value Output parameter for popped value
     * @return 0 on success, negative on error
     */
    int rpop(const std::string& key, std::string& value);
    
    /**
     * Get length of list
     * @param key List key
     * @return List length, or negative on error
     */
    int llen(const std::string& key);
    
    // ========================================================================
    // Pub/Sub Operations
    // ========================================================================
    
    /**
     * Publish a message to a channel
     * @param channel Channel name
     * @param message Message to publish
     * @return 0 on success, negative on error
     */
    int publish(const std::string& channel, const std::string& message);
    
    // ========================================================================
    // Transaction Operations
    // ========================================================================
    
    /**
     * Begin a transaction
     * @return 0 on success, negative on error
     */
    int multi();
    
    /**
     * Execute queued commands
     * @return 0 on success, negative on error
     */
    int exec();
    
    /**
     * Discard queued commands
     * @return 0 on success, negative on error
     */
    int discard();
    
    // ========================================================================
    // TTL Management
    // ========================================================================
    
    /**
     * Set expiration time on a key
     * @param key Key to expire
     * @param seconds TTL in seconds
     * @return 0 on success, negative on error
     */
    int expire(const std::string& key, uint32_t seconds);
    
    /**
     * Get TTL of a key
     * @param key Key to check
     * @return TTL in seconds, -1 if no expiration, -2 if key doesn't exist
     */
    int ttl(const std::string& key);
    
    /**
     * Check if connected to Redis
     * @return true if connected
     */
    bool isConnected() const;
    
    /**
     * Cleanup resources
     */
    void cleanup();
    
private:
    redisContext* context_;
    std::string host_;
    uint16_t port_;
    bool initialized_;
    std::mutex mutex_;
    
    // Helper to execute command
    redisReply* executeCommand(const char* format, ...);
    void freeReply(redisReply* reply);
};

} // namespace cache
} // namespace rootstream

#endif // __cplusplus

#endif // ROOTSTREAM_REDIS_CLIENT_H
