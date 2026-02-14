/**
 * @file event_store.h
 * @brief Event sourcing and audit logging for RootStream
 */

#ifndef ROOTSTREAM_EVENT_STORE_H
#define ROOTSTREAM_EVENT_STORE_H

#include <string>
#include <vector>
#include <cstdint>

#ifdef __cplusplus

#include <nlohmann/json.hpp>
#include "../database/database_manager.h"

namespace rootstream {
namespace events {

/**
 * Event store for event sourcing and audit trail
 */
class EventStore {
public:
    struct Event {
        uint64_t id;
        std::string aggregate_type;
        uint32_t aggregate_id;
        std::string event_type;
        nlohmann::json event_data;
        uint64_t timestamp_us;
        uint32_t version;
        uint32_t user_id;
        
        Event() : id(0), aggregate_id(0), timestamp_us(0), version(0), user_id(0) {}
    };
    
    EventStore();
    ~EventStore();
    
    /**
     * Initialize event store with database
     * @param dbManager Database manager
     * @return 0 on success, negative on error
     */
    int init(database::DatabaseManager& dbManager);
    
    /**
     * Append an event to the log
     * @param event Event to append
     * @return 0 on success, negative on error
     */
    int appendEvent(const Event& event);
    
    /**
     * Get events for an aggregate
     * @param aggregateType Type of aggregate (e.g., "User", "Stream")
     * @param aggregateId ID of the aggregate
     * @param events Output vector of events
     * @param fromVersion Starting version (0 for all events)
     * @return 0 on success, negative on error
     */
    int getEvents(const std::string& aggregateType,
                 uint32_t aggregateId,
                 std::vector<Event>& events,
                 uint32_t fromVersion = 0);
    
    /**
     * Create a state snapshot
     * @param aggregateType Type of aggregate
     * @param aggregateId ID of the aggregate
     * @param version Version number
     * @param state State data as JSON
     * @return 0 on success, negative on error
     */
    int createSnapshot(const std::string& aggregateType,
                      uint32_t aggregateId,
                      uint32_t version,
                      const nlohmann::json& state);
    
    /**
     * Get the latest snapshot
     * @param aggregateType Type of aggregate
     * @param aggregateId ID of the aggregate
     * @param state Output parameter for state data
     * @param version Output parameter for version number
     * @return 0 on success, negative on error
     */
    int getSnapshot(const std::string& aggregateType,
                   uint32_t aggregateId,
                   nlohmann::json& state,
                   uint32_t& version);
    
    /**
     * Get audit trail for a user
     * @param userId User ID
     * @param events Output vector of events
     * @param fromTime Starting timestamp in microseconds (0 for all)
     * @return 0 on success, negative on error
     */
    int getAuditTrail(uint32_t userId,
                     std::vector<Event>& events,
                     uint64_t fromTime = 0);
    
    /**
     * Cleanup resources
     */
    void cleanup();
    
private:
    database::DatabaseManager* db_;
    bool initialized_;
};

} // namespace events
} // namespace rootstream

#endif // __cplusplus

#endif // ROOTSTREAM_EVENT_STORE_H
