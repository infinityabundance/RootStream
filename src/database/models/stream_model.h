/**
 * @file stream_model.h
 * @brief Stream data model for managing live streams
 */

#ifndef ROOTSTREAM_STREAM_MODEL_H
#define ROOTSTREAM_STREAM_MODEL_H

#include <string>
#include <cstdint>

#ifdef __cplusplus

#include "../database_manager.h"
#include "../../cache/redis_client.h"

namespace rootstream {
namespace database {
namespace models {

/**
 * Stream model for managing live streams
 */
class StreamModel {
public:
    struct StreamData {
        uint32_t id;
        uint32_t user_id;
        std::string name;
        std::string description;
        std::string stream_key;
        std::string stream_url;
        std::string thumbnail_url;
        bool is_live;
        uint32_t viewer_count;
        uint32_t bitrate_kbps;
        std::string resolution;
        uint32_t fps;
        std::string codec;
        bool is_public;
        uint64_t created_at_us;
        uint64_t updated_at_us;
        uint64_t started_at_us;
        uint64_t ended_at_us;
        
        StreamData() : id(0), user_id(0), is_live(false), viewer_count(0),
                      bitrate_kbps(0), fps(0), is_public(true),
                      created_at_us(0), updated_at_us(0), 
                      started_at_us(0), ended_at_us(0) {}
    };
    
    StreamModel();
    ~StreamModel();
    
    /**
     * Create a new stream
     * @param db Database manager
     * @param userId Owner user ID
     * @param name Stream name
     * @return 0 on success, negative on error
     */
    int create(DatabaseManager& db, uint32_t userId, const std::string& name);
    
    /**
     * Load stream by ID
     * @param db Database manager
     * @param streamId Stream ID
     * @return 0 on success, negative on error
     */
    int load(DatabaseManager& db, uint32_t streamId);
    
    /**
     * Load stream by stream key
     * @param db Database manager
     * @param key Stream key
     * @return 0 on success, negative on error
     */
    int loadByStreamKey(DatabaseManager& db, const std::string& key);
    
    /**
     * Start the stream (mark as live)
     * @param db Database manager
     * @param redis Redis client for caching
     * @return 0 on success, negative on error
     */
    int startStream(DatabaseManager& db, cache::RedisClient& redis);
    
    /**
     * Stop the stream (mark as offline)
     * @param db Database manager
     * @param redis Redis client for clearing cache
     * @return 0 on success, negative on error
     */
    int stopStream(DatabaseManager& db, cache::RedisClient& redis);
    
    /**
     * Update viewer count (cached in Redis)
     * @param redis Redis client
     * @param count New viewer count
     * @return 0 on success, negative on error
     */
    int updateViewerCount(cache::RedisClient& redis, uint32_t count);
    
    /**
     * Update stream stats (bitrate, fps, resolution)
     * @param db Database manager
     * @param bitrate Bitrate in kbps
     * @param fps Frames per second
     * @return 0 on success, negative on error
     */
    int updateStreamStats(DatabaseManager& db, uint32_t bitrate, uint32_t fps);
    
    /**
     * Save current stream data
     * @param db Database manager
     * @return 0 on success, negative on error
     */
    int save(DatabaseManager& db);
    
    /**
     * Delete stream
     * @param db Database manager
     * @return 0 on success, negative on error
     */
    int deleteStream(DatabaseManager& db);
    
    /**
     * Get stream data
     * @return Reference to stream data
     */
    const StreamData& getData() const { return data_; }
    
    /**
     * Get stream ID
     * @return Stream ID
     */
    uint32_t getId() const { return data_.id; }
    
    /**
     * Check if stream is live
     * @return true if live
     */
    bool isLive() const { return data_.is_live; }
    
private:
    StreamData data_;
    bool loaded_;
    
    std::string generateStreamKey();
};

/**
 * Stream session model for tracking individual streaming sessions
 */
class StreamSessionModel {
public:
    struct StreamSessionData {
        uint32_t id;
        uint32_t stream_id;
        uint64_t session_start_us;
        uint64_t session_end_us;
        uint32_t total_viewers;
        uint32_t peak_viewers;
        uint64_t total_bytes_sent;
        uint32_t duration_seconds;
        bool is_recorded;
        std::string recording_path;
        
        StreamSessionData() : id(0), stream_id(0), session_start_us(0), 
                             session_end_us(0), total_viewers(0), peak_viewers(0),
                             total_bytes_sent(0), duration_seconds(0), 
                             is_recorded(false) {}
    };
    
    StreamSessionModel();
    ~StreamSessionModel();
    
    /**
     * Create a new stream session
     * @param db Database manager
     * @param streamId Stream ID
     * @return 0 on success, negative on error
     */
    int create(DatabaseManager& db, uint32_t streamId);
    
    /**
     * End the stream session
     * @param db Database manager
     * @return 0 on success, negative on error
     */
    int end(DatabaseManager& db);
    
    /**
     * Save session data
     * @param db Database manager
     * @return 0 on success, negative on error
     */
    int save(DatabaseManager& db);
    
    /**
     * Update viewer statistics
     * @param db Database manager
     * @param totalViewers Total unique viewers
     * @param peakViewers Peak concurrent viewers
     * @return 0 on success, negative on error
     */
    int updateViewerStats(DatabaseManager& db, uint32_t totalViewers, uint32_t peakViewers);
    
    /**
     * Get session data
     * @return Reference to session data
     */
    const StreamSessionData& getData() const { return data_; }
    
private:
    StreamSessionData data_;
    bool loaded_;
};

} // namespace models
} // namespace database
} // namespace rootstream

#endif // __cplusplus

#endif // ROOTSTREAM_STREAM_MODEL_H
