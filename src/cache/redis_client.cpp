/**
 * @file redis_client.cpp
 * @brief Implementation of Redis client for caching and pub/sub
 */

#include "redis_client.h"
#include <iostream>
#include <cstdarg>
#include <cstring>

namespace rootstream {
namespace cache {

RedisClient::RedisClient() 
    : context_(nullptr), port_(6379), initialized_(false) {
}

RedisClient::~RedisClient() {
    cleanup();
}

int RedisClient::init(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        std::cerr << "RedisClient already initialized" << std::endl;
        return -1;
    }
    
    host_ = host;
    port_ = port;
    
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    context_ = redisConnectWithTimeout(host.c_str(), port, timeout);
    
    if (!context_ || context_->err) {
        if (context_) {
            std::cerr << "Redis connection error: " << context_->errstr << std::endl;
            redisFree(context_);
            context_ = nullptr;
        } else {
            std::cerr << "Redis connection error: can't allocate redis context" << std::endl;
        }
        return -1;
    }
    
    initialized_ = true;
    std::cout << "RedisClient connected to " << host << ":" << port << std::endl;
    return 0;
}

redisReply* RedisClient::executeCommand(const char* format, ...) {
    if (!initialized_ || !context_) {
        return nullptr;
    }
    
    va_list args;
    va_start(args, format);
    redisReply* reply = static_cast<redisReply*>(redisvCommand(context_, format, args));
    va_end(args);
    
    return reply;
}

void RedisClient::freeReply(redisReply* reply) {
    if (reply) {
        freeReplyObject(reply);
    }
}

// ============================================================================
// Key-Value Operations
// ============================================================================

int RedisClient::set(const std::string& key, const std::string& value, uint32_t ttl_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply;
    if (ttl_seconds > 0) {
        reply = executeCommand("SETEX %s %d %s", key.c_str(), ttl_seconds, value.c_str());
    } else {
        reply = executeCommand("SET %s %s", key.c_str(), value.c_str());
    }
    
    if (!reply) {
        std::cerr << "Redis SET failed for key: " << key << std::endl;
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_STATUS && 
                  strcmp(reply->str, "OK") == 0) ? 0 : -1;
    freeReply(reply);
    return result;
}

int RedisClient::get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("GET %s", key.c_str());
    if (!reply) {
        return -1;
    }
    
    if (reply->type == REDIS_REPLY_STRING) {
        value = std::string(reply->str, reply->len);
        freeReply(reply);
        return 0;
    } else if (reply->type == REDIS_REPLY_NIL) {
        freeReply(reply);
        return -1; // Key not found
    }
    
    freeReply(reply);
    return -1;
}

int RedisClient::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("DEL %s", key.c_str());
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_INTEGER) ? 0 : -1;
    freeReply(reply);
    return result;
}

int RedisClient::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("EXISTS %s", key.c_str());
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_INTEGER) ? reply->integer : -1;
    freeReply(reply);
    return result;
}

// ============================================================================
// Hash Operations
// ============================================================================

int RedisClient::hset(const std::string& key, const std::string& field, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_INTEGER) ? 0 : -1;
    freeReply(reply);
    return result;
}

int RedisClient::hget(const std::string& key, const std::string& field, std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("HGET %s %s", key.c_str(), field.c_str());
    if (!reply) {
        return -1;
    }
    
    if (reply->type == REDIS_REPLY_STRING) {
        value = std::string(reply->str, reply->len);
        freeReply(reply);
        return 0;
    } else if (reply->type == REDIS_REPLY_NIL) {
        freeReply(reply);
        return -1;
    }
    
    freeReply(reply);
    return -1;
}

int RedisClient::hdel(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("HDEL %s %s", key.c_str(), field.c_str());
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_INTEGER) ? 0 : -1;
    freeReply(reply);
    return result;
}

int RedisClient::hgetall(const std::string& key, std::map<std::string, std::string>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("HGETALL %s", key.c_str());
    if (!reply) {
        return -1;
    }
    
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; i += 2) {
            if (i + 1 < reply->elements) {
                std::string field(reply->element[i]->str, reply->element[i]->len);
                std::string value(reply->element[i + 1]->str, reply->element[i + 1]->len);
                data[field] = value;
            }
        }
        freeReply(reply);
        return 0;
    }
    
    freeReply(reply);
    return -1;
}

// ============================================================================
// List Operations
// ============================================================================

int RedisClient::lpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("LPUSH %s %s", key.c_str(), value.c_str());
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_INTEGER) ? 0 : -1;
    freeReply(reply);
    return result;
}

int RedisClient::rpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("RPOP %s", key.c_str());
    if (!reply) {
        return -1;
    }
    
    if (reply->type == REDIS_REPLY_STRING) {
        value = std::string(reply->str, reply->len);
        freeReply(reply);
        return 0;
    } else if (reply->type == REDIS_REPLY_NIL) {
        freeReply(reply);
        return -1;
    }
    
    freeReply(reply);
    return -1;
}

int RedisClient::llen(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("LLEN %s", key.c_str());
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_INTEGER) ? reply->integer : -1;
    freeReply(reply);
    return result;
}

// ============================================================================
// Pub/Sub Operations
// ============================================================================

int RedisClient::subscribe(const std::string& channel, 
                           std::function<void(const std::string&)> callback) {
    // Note: Actual pub/sub requires a dedicated connection
    // This is a simplified implementation
    std::cerr << "Pub/Sub subscribe not fully implemented yet" << std::endl;
    return -1;
}

int RedisClient::publish(const std::string& channel, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("PUBLISH %s %s", channel.c_str(), message.c_str());
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_INTEGER) ? 0 : -1;
    freeReply(reply);
    return result;
}

int RedisClient::unsubscribe(const std::string& channel) {
    std::cerr << "Pub/Sub unsubscribe not fully implemented yet" << std::endl;
    return -1;
}

// ============================================================================
// Transaction Operations
// ============================================================================

int RedisClient::multi() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("MULTI");
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_STATUS) ? 0 : -1;
    freeReply(reply);
    return result;
}

int RedisClient::exec() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("EXEC");
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_ARRAY) ? 0 : -1;
    freeReply(reply);
    return result;
}

int RedisClient::discard() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("DISCARD");
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_STATUS) ? 0 : -1;
    freeReply(reply);
    return result;
}

// ============================================================================
// TTL Management
// ============================================================================

int RedisClient::expire(const std::string& key, uint32_t seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("EXPIRE %s %d", key.c_str(), seconds);
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1) ? 0 : -1;
    freeReply(reply);
    return result;
}

int RedisClient::ttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    redisReply* reply = executeCommand("TTL %s", key.c_str());
    if (!reply) {
        return -1;
    }
    
    int result = (reply->type == REDIS_REPLY_INTEGER) ? reply->integer : -1;
    freeReply(reply);
    return result;
}

bool RedisClient::isConnected() const {
    return initialized_ && context_ && !context_->err;
}

void RedisClient::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
    }
    initialized_ = false;
}

} // namespace cache
} // namespace rootstream

// ============================================================================
// C API Implementation
// ============================================================================

using namespace rootstream::cache;

struct RedisClient {
    rootstream::cache::RedisClient* client;
};

int redis_client_init(RedisClient** client, const char* host, uint16_t port) {
    if (!client || !host) {
        return -1;
    }
    
    try {
        *client = new RedisClient();
        (*client)->client = new rootstream::cache::RedisClient();
        return (*client)->client->init(host, port);
    } catch (const std::exception& e) {
        std::cerr << "C API init failed: " << e.what() << std::endl;
        return -1;
    }
}

int redis_client_set(RedisClient* client, const char* key, const char* value, uint32_t ttl_seconds) {
    if (!client || !client->client || !key || !value) {
        return -1;
    }
    
    return client->client->set(key, value, ttl_seconds);
}

int redis_client_get(RedisClient* client, const char* key, char** value) {
    if (!client || !client->client || !key || !value) {
        return -1;
    }
    
    std::string val;
    int result = client->client->get(key, val);
    if (result == 0) {
        *value = strdup(val.c_str());
    }
    return result;
}

int redis_client_del(RedisClient* client, const char* key) {
    if (!client || !client->client || !key) {
        return -1;
    }
    
    return client->client->del(key);
}

int redis_client_is_connected(RedisClient* client) {
    if (!client || !client->client) {
        return 0;
    }
    
    return client->client->isConnected() ? 1 : 0;
}

void redis_client_cleanup(RedisClient* client) {
    if (client) {
        if (client->client) {
            client->client->cleanup();
            delete client->client;
        }
        delete client;
    }
}
