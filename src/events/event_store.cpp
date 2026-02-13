/**
 * @file event_store.cpp
 * @brief Implementation of event store
 */

#include "event_store.h"
#include <iostream>
#include <sstream>
#include <chrono>

namespace rootstream {
namespace events {

EventStore::EventStore() : db_(nullptr), initialized_(false) {}

EventStore::~EventStore() {
    cleanup();
}

int EventStore::init(database::DatabaseManager& dbManager) {
    if (initialized_) {
        std::cerr << "EventStore already initialized" << std::endl;
        return -1;
    }
    
    db_ = &dbManager;
    initialized_ = true;
    std::cout << "EventStore initialized" << std::endl;
    return 0;
}

int EventStore::appendEvent(const Event& event) {
    if (!initialized_) {
        std::cerr << "EventStore not initialized" << std::endl;
        return -1;
    }
    
    try {
        std::string eventDataStr = event.event_data.dump();
        
        // Escape single quotes in JSON string
        size_t pos = 0;
        while ((pos = eventDataStr.find("'", pos)) != std::string::npos) {
            eventDataStr.replace(pos, 1, "''");
            pos += 2;
        }
        
        std::stringstream query;
        query << "INSERT INTO event_log "
              << "(aggregate_type, aggregate_id, event_type, event_data, version, user_id) "
              << "VALUES ('" << event.aggregate_type << "', " << event.aggregate_id 
              << ", '" << event.event_type << "', '" << eventDataStr << "'::jsonb, "
              << event.version << ", ";
        
        if (event.user_id > 0) {
            query << event.user_id;
        } else {
            query << "NULL";
        }
        
        query << ")";
        
        int result = db_->executeQuery(query.str());
        if (result >= 0) {
            std::cout << "Event appended: " << event.event_type << " for " 
                     << event.aggregate_type << ":" << event.aggregate_id << std::endl;
        }
        return (result >= 0) ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to append event: " << e.what() << std::endl;
        return -1;
    }
}

int EventStore::getEvents(const std::string& aggregateType,
                         uint32_t aggregateId,
                         std::vector<Event>& events,
                         uint32_t fromVersion) {
    if (!initialized_) {
        std::cerr << "EventStore not initialized" << std::endl;
        return -1;
    }
    
    try {
        std::stringstream query;
        query << "SELECT id, aggregate_type, aggregate_id, event_type, "
              << "event_data::text, "
              << "EXTRACT(EPOCH FROM timestamp) * 1000000 as timestamp_us, "
              << "version, COALESCE(user_id, 0) as user_id "
              << "FROM event_log "
              << "WHERE aggregate_type = '" << aggregateType << "' "
              << "AND aggregate_id = " << aggregateId << " ";
        
        if (fromVersion > 0) {
            query << "AND version >= " << fromVersion << " ";
        }
        
        query << "ORDER BY version ASC";
        
        auto result = db_->executeSelect(query.str());
        
        for (const auto& row : result) {
            Event event;
            event.id = std::stoull(row["id"].c_str());
            event.aggregate_type = row["aggregate_type"].c_str();
            event.aggregate_id = std::stoul(row["aggregate_id"].c_str());
            event.event_type = row["event_type"].c_str();
            event.event_data = nlohmann::json::parse(row["event_data"].c_str());
            event.timestamp_us = std::stoull(row["timestamp_us"].c_str());
            event.version = std::stoul(row["version"].c_str());
            event.user_id = std::stoul(row["user_id"].c_str());
            
            events.push_back(event);
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to get events: " << e.what() << std::endl;
        return -1;
    }
}

int EventStore::createSnapshot(const std::string& aggregateType,
                               uint32_t aggregateId,
                               uint32_t version,
                               const nlohmann::json& state) {
    if (!initialized_) {
        std::cerr << "EventStore not initialized" << std::endl;
        return -1;
    }
    
    try {
        std::string stateStr = state.dump();
        
        // Escape single quotes
        size_t pos = 0;
        while ((pos = stateStr.find("'", pos)) != std::string::npos) {
            stateStr.replace(pos, 1, "''");
            pos += 2;
        }
        
        std::stringstream query;
        query << "INSERT INTO snapshots "
              << "(aggregate_type, aggregate_id, version, state) "
              << "VALUES ('" << aggregateType << "', " << aggregateId 
              << ", " << version << ", '" << stateStr << "'::jsonb) "
              << "ON CONFLICT (aggregate_type, aggregate_id, version) "
              << "DO UPDATE SET state = EXCLUDED.state";
        
        int result = db_->executeQuery(query.str());
        if (result >= 0) {
            std::cout << "Snapshot created for " << aggregateType << ":" 
                     << aggregateId << " v" << version << std::endl;
        }
        return (result >= 0) ? 0 : -1;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create snapshot: " << e.what() << std::endl;
        return -1;
    }
}

int EventStore::getSnapshot(const std::string& aggregateType,
                           uint32_t aggregateId,
                           nlohmann::json& state,
                           uint32_t& version) {
    if (!initialized_) {
        std::cerr << "EventStore not initialized" << std::endl;
        return -1;
    }
    
    try {
        std::stringstream query;
        query << "SELECT version, state::text "
              << "FROM snapshots "
              << "WHERE aggregate_type = '" << aggregateType << "' "
              << "AND aggregate_id = " << aggregateId << " "
              << "ORDER BY version DESC LIMIT 1";
        
        auto result = db_->executeSelect(query.str());
        
        if (result.size() == 0) {
            return -1; // No snapshot found
        }
        
        auto row = result[0];
        version = std::stoul(row["version"].c_str());
        state = nlohmann::json::parse(row["state"].c_str());
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to get snapshot: " << e.what() << std::endl;
        return -1;
    }
}

int EventStore::getAuditTrail(uint32_t userId,
                              std::vector<Event>& events,
                              uint64_t fromTime) {
    if (!initialized_) {
        std::cerr << "EventStore not initialized" << std::endl;
        return -1;
    }
    
    try {
        std::stringstream query;
        query << "SELECT id, aggregate_type, aggregate_id, event_type, "
              << "event_data::text, "
              << "EXTRACT(EPOCH FROM timestamp) * 1000000 as timestamp_us, "
              << "version, user_id "
              << "FROM event_log "
              << "WHERE user_id = " << userId << " ";
        
        if (fromTime > 0) {
            query << "AND EXTRACT(EPOCH FROM timestamp) * 1000000 >= " << fromTime << " ";
        }
        
        query << "ORDER BY timestamp DESC";
        
        auto result = db_->executeSelect(query.str());
        
        for (const auto& row : result) {
            Event event;
            event.id = std::stoull(row["id"].c_str());
            event.aggregate_type = row["aggregate_type"].c_str();
            event.aggregate_id = std::stoul(row["aggregate_id"].c_str());
            event.event_type = row["event_type"].c_str();
            event.event_data = nlohmann::json::parse(row["event_data"].c_str());
            event.timestamp_us = std::stoull(row["timestamp_us"].c_str());
            event.version = std::stoul(row["version"].c_str());
            event.user_id = std::stoul(row["user_id"].c_str());
            
            events.push_back(event);
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed to get audit trail: " << e.what() << std::endl;
        return -1;
    }
}

void EventStore::cleanup() {
    initialized_ = false;
    db_ = nullptr;
}

} // namespace events
} // namespace rootstream
