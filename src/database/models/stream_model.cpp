/**
 * @file stream_model.cpp
 * @brief Implementation of stream models
 */

#include "stream_model.h"
#include <iostream>
#include <sstream>
#include <random>
#include <chrono>
#include <iomanip>

namespace rootstream {
namespace database {
namespace models {

// ============================================================================
// StreamModel Implementation
// ============================================================================

StreamModel::StreamModel() : loaded_(false) {}

StreamModel::~StreamModel() {}

std::string StreamModel::generateStreamKey() {
    // Generate a random stream key
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "sk_";
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

int StreamModel::create(DatabaseManager& db, uint32_t userId, const std::string& name) {
    try {
        std::string streamKey = generateStreamKey();
        
        std::string query = "INSERT INTO streams (user_id, name, stream_key, is_live, is_public) "
                           "VALUES ($1, $2, $3, false, true) RETURNING id";
        
        std::vector<std::string> params = {
            std::to_string(userId),
            name,
            streamKey
        };
        
        auto result = db.executeParams(query, params);
        
        if (result.size() > 0) {
            data_.id = std::stoul(result[0][0].c_str());
            data_.user_id = userId;
            data_.name = name;
            data_.stream_key = streamKey;
            data_.is_live = false;
            data_.is_public = true;
            loaded_ = true;
            
            std::cout << "Stream created with ID: " << data_.id << std::endl;
            return 0;
        }
        
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create stream: " << e.what() << std::endl;
        return -1;
    }
}

int StreamModel::load(DatabaseManager& db, uint32_t streamId) {
    try {
        std::stringstream query;
        query << "SELECT id, user_id, name, description, stream_key, stream_url, "
              << "thumbnail_url, is_live, viewer_count, bitrate_kbps, resolution, "
              << "fps, codec, is_public, "
              << "EXTRACT(EPOCH FROM created_at) * 1000000 as created_at_us, "
              << "EXTRACT(EPOCH FROM updated_at) * 1000000 as updated_at_us, "
              << "EXTRACT(EPOCH FROM started_at) * 1000000 as started_at_us, "
              << "EXTRACT(EPOCH FROM ended_at) * 1000000 as ended_at_us "
              << "FROM streams WHERE id = " << streamId;
        
        auto result = db.executeSelect(query.str());
        
        if (result.size() == 0) {
            std::cerr << "Stream not found: " << streamId << std::endl;
            return -1;
        }
        
        auto row = result[0];
        data_.id = std::stoul(row["id"].c_str());
        data_.user_id = std::stoul(row["user_id"].c_str());
        data_.name = row["name"].c_str();
        data_.description = row["description"].is_null() ? "" : row["description"].c_str();
        data_.stream_key = row["stream_key"].c_str();
        data_.stream_url = row["stream_url"].is_null() ? "" : row["stream_url"].c_str();
        data_.thumbnail_url = row["thumbnail_url"].is_null() ? "" : row["thumbnail_url"].c_str();
        data_.is_live = strcmp(row["is_live"].c_str(), "t") == 0;
        data_.viewer_count = row["viewer_count"].is_null() ? 0 : std::stoul(row["viewer_count"].c_str());
        data_.bitrate_kbps = row["bitrate_kbps"].is_null() ? 0 : std::stoul(row["bitrate_kbps"].c_str());
        data_.resolution = row["resolution"].is_null() ? "" : row["resolution"].c_str();
        data_.fps = row["fps"].is_null() ? 0 : std::stoul(row["fps"].c_str());
        data_.codec = row["codec"].is_null() ? "" : row["codec"].c_str();
        data_.is_public = strcmp(row["is_public"].c_str(), "t") == 0;
        data_.created_at_us = row["created_at_us"].is_null() ? 0 : std::stoull(row["created_at_us"].c_str());
        data_.updated_at_us = row["updated_at_us"].is_null() ? 0 : std::stoull(row["updated_at_us"].c_str());
        data_.started_at_us = row["started_at_us"].is_null() ? 0 : std::stoull(row["started_at_us"].c_str());
        data_.ended_at_us = row["ended_at_us"].is_null() ? 0 : std::stoull(row["ended_at_us"].c_str());
        
        loaded_ = true;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load stream: " << e.what() << std::endl;
        return -1;
    }
}

int StreamModel::loadByStreamKey(DatabaseManager& db, const std::string& key) {
    try {
        std::string query = "SELECT id, user_id, name, description, stream_key, stream_url, "
                           "thumbnail_url, is_live, viewer_count, bitrate_kbps, resolution, "
                           "fps, codec, is_public, "
                           "EXTRACT(EPOCH FROM created_at) * 1000000 as created_at_us, "
                           "EXTRACT(EPOCH FROM updated_at) * 1000000 as updated_at_us, "
                           "EXTRACT(EPOCH FROM started_at) * 1000000 as started_at_us, "
                           "EXTRACT(EPOCH FROM ended_at) * 1000000 as ended_at_us "
                           "FROM streams WHERE stream_key = $1";
        
        std::vector<std::string> params = {key};
        auto result = db.executeParams(query, params);
        
        if (result.size() == 0) {
            std::cerr << "Stream not found with key: " << key << std::endl;
            return -1;
        }
        
        auto row = result[0];
        data_.id = std::stoul(row["id"].c_str());
        data_.user_id = std::stoul(row["user_id"].c_str());
        data_.name = row["name"].c_str();
        data_.description = row["description"].is_null() ? "" : row["description"].c_str();
        data_.stream_key = row["stream_key"].c_str();
        data_.stream_url = row["stream_url"].is_null() ? "" : row["stream_url"].c_str();
        data_.thumbnail_url = row["thumbnail_url"].is_null() ? "" : row["thumbnail_url"].c_str();
        data_.is_live = strcmp(row["is_live"].c_str(), "t") == 0;
        data_.viewer_count = row["viewer_count"].is_null() ? 0 : std::stoul(row["viewer_count"].c_str());
        data_.bitrate_kbps = row["bitrate_kbps"].is_null() ? 0 : std::stoul(row["bitrate_kbps"].c_str());
        data_.resolution = row["resolution"].is_null() ? "" : row["resolution"].c_str();
        data_.fps = row["fps"].is_null() ? 0 : std::stoul(row["fps"].c_str());
        data_.codec = row["codec"].is_null() ? "" : row["codec"].c_str();
        data_.is_public = strcmp(row["is_public"].c_str(), "t") == 0;
        data_.created_at_us = row["created_at_us"].is_null() ? 0 : std::stoull(row["created_at_us"].c_str());
        data_.updated_at_us = row["updated_at_us"].is_null() ? 0 : std::stoull(row["updated_at_us"].c_str());
        data_.started_at_us = row["started_at_us"].is_null() ? 0 : std::stoull(row["started_at_us"].c_str());
        data_.ended_at_us = row["ended_at_us"].is_null() ? 0 : std::stoull(row["ended_at_us"].c_str());
        
        loaded_ = true;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load stream by key: " << e.what() << std::endl;
        return -1;
    }
}

int StreamModel::startStream(DatabaseManager& db, cache::RedisClient& redis) {
    if (!loaded_) {
        std::cerr << "Cannot start unloaded stream" << std::endl;
        return -1;
    }
    
    try {
        auto now = std::chrono::system_clock::now();
        data_.started_at_us = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
        
        std::stringstream query;
        query << "UPDATE streams SET is_live = true, started_at = CURRENT_TIMESTAMP "
              << "WHERE id = " << data_.id;
        
        int result = db.executeQuery(query.str());
        if (result >= 0) {
            data_.is_live = true;
            
            // Cache stream state in Redis
            std::stringstream key;
            key << "stream:" << data_.id << ":live";
            redis.set(key.str(), "1", 3600); // 1 hour TTL
            
            // Publish stream start event
            std::stringstream channel;
            channel << "stream:" << data_.id << ":events";
            redis.publish(channel.str(), "started");
        }
        return (result >= 0) ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start stream: " << e.what() << std::endl;
        return -1;
    }
}

int StreamModel::stopStream(DatabaseManager& db, cache::RedisClient& redis) {
    if (!loaded_) {
        std::cerr << "Cannot stop unloaded stream" << std::endl;
        return -1;
    }
    
    try {
        auto now = std::chrono::system_clock::now();
        data_.ended_at_us = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
        
        std::stringstream query;
        query << "UPDATE streams SET is_live = false, ended_at = CURRENT_TIMESTAMP "
              << "WHERE id = " << data_.id;
        
        int result = db.executeQuery(query.str());
        if (result >= 0) {
            data_.is_live = false;
            data_.viewer_count = 0;
            
            // Clear Redis cache
            std::stringstream key;
            key << "stream:" << data_.id << ":live";
            redis.del(key.str());
            
            // Clear viewer count
            std::stringstream vcKey;
            vcKey << "stream:" << data_.id << ":viewers";
            redis.del(vcKey.str());
            
            // Publish stream end event
            std::stringstream channel;
            channel << "stream:" << data_.id << ":events";
            redis.publish(channel.str(), "ended");
        }
        return (result >= 0) ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to stop stream: " << e.what() << std::endl;
        return -1;
    }
}

int StreamModel::updateViewerCount(cache::RedisClient& redis, uint32_t count) {
    if (!loaded_) {
        return -1;
    }
    
    data_.viewer_count = count;
    
    // Cache in Redis
    std::stringstream key;
    key << "stream:" << data_.id << ":viewers";
    return redis.set(key.str(), std::to_string(count), 300); // 5 min TTL
}

int StreamModel::updateStreamStats(DatabaseManager& db, uint32_t bitrate, uint32_t fps) {
    if (!loaded_) {
        return -1;
    }
    
    try {
        data_.bitrate_kbps = bitrate;
        data_.fps = fps;
        
        std::stringstream query;
        query << "UPDATE streams SET bitrate_kbps = " << bitrate 
              << ", fps = " << fps << " WHERE id = " << data_.id;
        
        return db.executeQuery(query.str()) >= 0 ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to update stream stats: " << e.what() << std::endl;
        return -1;
    }
}

int StreamModel::save(DatabaseManager& db) {
    if (!loaded_) {
        return -1;
    }
    
    try {
        std::string query = "UPDATE streams SET "
                           "name = $1, description = $2, stream_url = $3, thumbnail_url = $4, "
                           "is_public = $5 "
                           "WHERE id = $6";
        
        std::vector<std::string> params = {
            data_.name,
            data_.description,
            data_.stream_url,
            data_.thumbnail_url,
            data_.is_public ? "true" : "false",
            std::to_string(data_.id)
        };
        
        auto result = db.executeParams(query, params);
        return (result.affected_rows() > 0) ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save stream: " << e.what() << std::endl;
        return -1;
    }
}

int StreamModel::deleteStream(DatabaseManager& db) {
    if (!loaded_) {
        return -1;
    }
    
    try {
        std::stringstream query;
        query << "DELETE FROM streams WHERE id = " << data_.id;
        
        int result = db.executeQuery(query.str());
        if (result >= 0) {
            loaded_ = false;
        }
        return (result >= 0) ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to delete stream: " << e.what() << std::endl;
        return -1;
    }
}

// ============================================================================
// StreamSessionModel Implementation
// ============================================================================

StreamSessionModel::StreamSessionModel() : loaded_(false) {}

StreamSessionModel::~StreamSessionModel() {}

int StreamSessionModel::create(DatabaseManager& db, uint32_t streamId) {
    try {
        auto now = std::chrono::system_clock::now();
        data_.session_start_us = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
        
        std::stringstream query;
        query << "INSERT INTO stream_sessions (stream_id, session_start) "
              << "VALUES (" << streamId << ", CURRENT_TIMESTAMP) RETURNING id";
        
        auto result = db.executeSelect(query.str());
        
        if (result.size() > 0) {
            data_.id = std::stoul(result[0][0].c_str());
            data_.stream_id = streamId;
            loaded_ = true;
            
            std::cout << "Stream session created with ID: " << data_.id << std::endl;
            return 0;
        }
        
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create stream session: " << e.what() << std::endl;
        return -1;
    }
}

int StreamSessionModel::end(DatabaseManager& db) {
    if (!loaded_) {
        return -1;
    }
    
    try {
        auto now = std::chrono::system_clock::now();
        data_.session_end_us = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
        
        data_.duration_seconds = static_cast<uint32_t>(
            (data_.session_end_us - data_.session_start_us) / 1000000);
        
        std::stringstream query;
        query << "UPDATE stream_sessions SET "
              << "session_end = CURRENT_TIMESTAMP, "
              << "duration_seconds = " << data_.duration_seconds << " "
              << "WHERE id = " << data_.id;
        
        return db.executeQuery(query.str()) >= 0 ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to end stream session: " << e.what() << std::endl;
        return -1;
    }
}

int StreamSessionModel::save(DatabaseManager& db) {
    if (!loaded_) {
        return -1;
    }
    
    try {
        std::stringstream query;
        query << "UPDATE stream_sessions SET "
              << "total_viewers = " << data_.total_viewers << ", "
              << "peak_viewers = " << data_.peak_viewers << ", "
              << "total_bytes_sent = " << data_.total_bytes_sent << ", "
              << "is_recorded = " << (data_.is_recorded ? "true" : "false") << ", "
              << "recording_path = " << (data_.recording_path.empty() ? "NULL" : "'" + data_.recording_path + "'") << " "
              << "WHERE id = " << data_.id;
        
        return db.executeQuery(query.str()) >= 0 ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save stream session: " << e.what() << std::endl;
        return -1;
    }
}

int StreamSessionModel::updateViewerStats(DatabaseManager& db, uint32_t totalViewers, uint32_t peakViewers) {
    if (!loaded_) {
        return -1;
    }
    
    data_.total_viewers = totalViewers;
    data_.peak_viewers = peakViewers;
    
    try {
        std::stringstream query;
        query << "UPDATE stream_sessions SET "
              << "total_viewers = " << totalViewers << ", "
              << "peak_viewers = " << peakViewers << " "
              << "WHERE id = " << data_.id;
        
        return db.executeQuery(query.str()) >= 0 ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to update viewer stats: " << e.what() << std::endl;
        return -1;
    }
}

} // namespace models
} // namespace database
} // namespace rootstream
