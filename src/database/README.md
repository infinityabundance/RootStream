# RootStream Database Layer

## Overview

The RootStream database layer provides a comprehensive solution for managing users, sessions, streams, recordings, billing, and audit logs with support for:

- **PostgreSQL** for persistent data storage
- **Redis** for high-performance caching and real-time state
- **Event Sourcing** for audit trails and state reconstruction
- **Connection Pooling** for optimal database performance
- **Transaction Management** with ACID guarantees

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│              Application Layer                          │
├─────────────────────────────────────────────────────────┤
│  - User Models                                          │
│  - Stream Models                                        │
│  - Session Models                                       │
└────────────────┬────────────────────────────────────────┘
                 │
    ┌────────────┴────────────┐
    │                         │
┌───▼───────┐         ┌───────▼────────┐
│  Database │         │     Redis      │
│  Manager  │         │     Cache      │
└───────────┘         └────────────────┘
    │                         │
    │                         │
┌───▼─────────────────────────▼────────┐
│     PostgreSQL + Redis Services      │
└──────────────────────────────────────┘
```

## Components

### 1. Database Manager (`database_manager.h/cpp`)

Manages PostgreSQL connections with connection pooling:

```cpp
#include "database/database_manager.h"

using namespace rootstream::database;

// Initialize database
DatabaseManager db;
db.init("postgresql://user:pass@localhost/rootstream", 20);

// Execute queries
db.executeQuery("INSERT INTO users ...");
auto result = db.executeSelect("SELECT * FROM users");

// Run migrations
db.runMigrations("src/database/migrations");
```

### 2. Redis Client (`cache/redis_client.h/cpp`)

Provides caching and pub/sub functionality:

```cpp
#include "cache/redis_client.h"

using namespace rootstream::cache;

// Initialize Redis
RedisClient redis;
redis.init("localhost", 6379);

// Key-value operations
redis.set("session:123", "user_data", 3600);
std::string value;
redis.get("session:123", value);

// Hash operations
redis.hset("user:1", "name", "John");
redis.hget("user:1", "name", value);

// Pub/Sub
redis.publish("stream:events", "stream_started");
```

### 3. User Model (`database/models/user_model.h/cpp`)

Manages user accounts and authentication:

```cpp
#include "database/models/user_model.h"

using namespace rootstream::database::models;

// Create a new user
User::createUser(db, "john_doe", "john@example.com", "hashed_password");

// Load user
User user;
user.loadByUsername(db, "john_doe");

// Update user
user.updateLastLogin(db);
user.verifyAccount(db);
```

### 4. Stream Model (`database/models/stream_model.h/cpp`)

Manages live streams and stream sessions:

```cpp
#include "database/models/stream_model.h"

using namespace rootstream::database::models;

// Create stream
StreamModel stream;
stream.create(db, userId, "My Stream");

// Start streaming
stream.startStream(db, redis);

// Update viewer count
stream.updateViewerCount(redis, 150);

// Stop streaming
stream.stopStream(db, redis);

// Stream session tracking
StreamSessionModel session;
session.create(db, streamId);
session.updateViewerStats(db, 500, 150);
session.end(db);
```

### 5. Event Store (`events/event_store.h/cpp`)

Implements event sourcing and audit logging:

```cpp
#include "events/event_store.h"

using namespace rootstream::events;

// Initialize event store
EventStore eventStore;
eventStore.init(db);

// Append event
EventStore::Event event;
event.aggregate_type = "Stream";
event.aggregate_id = streamId;
event.event_type = "StreamStarted";
event.event_data = {{"bitrate", 5000}, {"resolution", "1920x1080"}};
event.version = 1;
event.user_id = userId;
eventStore.appendEvent(event);

// Get event history
std::vector<EventStore::Event> events;
eventStore.getEvents("Stream", streamId, events);

// Create snapshot
nlohmann::json state = {{"is_live", true}, {"viewers", 100}};
eventStore.createSnapshot("Stream", streamId, 5, state);

// Audit trail
std::vector<EventStore::Event> audit;
eventStore.getAuditTrail(userId, audit);
```

## Database Schema

The schema includes the following tables:

- **users** - User accounts and authentication
- **sessions** - User sessions with device tracking
- **streams** - Stream metadata and configuration
- **stream_sessions** - Individual streaming sessions
- **recordings** - Recording metadata
- **usage_logs** - Usage tracking for billing
- **billing_accounts** - Billing and subscription info
- **event_log** - Event sourcing log
- **snapshots** - State snapshots for optimization

## Setup Instructions

### 1. Install Dependencies

```bash
# Install PostgreSQL
sudo apt-get install postgresql postgresql-contrib

# Install Redis
sudo apt-get install redis-server

# Install C++ dependencies via vcpkg
vcpkg install libpqxx hiredis nlohmann-json
```

### 2. Configure PostgreSQL

```bash
# Create database
sudo -u postgres createdb rootstream

# Create user
sudo -u postgres createuser rootstream -P

# Grant privileges
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE rootstream TO rootstream;"
```

### 3. Initialize Schema

```bash
# Run schema creation
psql -U rootstream -d rootstream -f src/database/schema.sql
```

### 4. Configure Connection

Set environment variables or configuration:

```bash
export DATABASE_URL="postgresql://rootstream:password@localhost/rootstream"
export REDIS_URL="redis://localhost:6379"
```

## Docker Compose Setup

Use the provided docker-compose.yml for easy setup:

```yaml
services:
  postgres:
    image: postgres:15-alpine
    environment:
      POSTGRES_DB: rootstream
      POSTGRES_USER: rootstream
      POSTGRES_PASSWORD: password
    ports:
      - "5432:5432"
    volumes:
      - postgres-data:/var/lib/postgresql/data

  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"
    volumes:
      - redis-data:/data

volumes:
  postgres-data:
  redis-data:
```

Start services:

```bash
cd infrastructure/docker
docker-compose up -d postgres redis
```

## Usage Example

Complete example application:

```cpp
#include "database/database_manager.h"
#include "cache/redis_client.h"
#include "database/models/user_model.h"
#include "database/models/stream_model.h"
#include "events/event_store.h"

int main() {
    // Initialize database
    rootstream::database::DatabaseManager db;
    if (db.init("postgresql://rootstream:password@localhost/rootstream", 20) != 0) {
        return -1;
    }
    
    // Initialize Redis
    rootstream::cache::RedisClient redis;
    if (redis.init("localhost", 6379) != 0) {
        return -1;
    }
    
    // Initialize event store
    rootstream::events::EventStore eventStore;
    eventStore.init(db);
    
    // Create user
    rootstream::database::models::User::createUser(
        db, "streamer1", "streamer@example.com", "hashed_pass");
    
    // Load user
    rootstream::database::models::User user;
    user.loadByUsername(db, "streamer1");
    
    // Create stream
    rootstream::database::models::StreamModel stream;
    stream.create(db, user.getId(), "My Gaming Stream");
    
    // Start streaming
    stream.startStream(db, redis);
    
    // Log event
    rootstream::events::EventStore::Event event;
    event.aggregate_type = "Stream";
    event.aggregate_id = stream.getId();
    event.event_type = "StreamStarted";
    event.event_data = {{"quality", "HD"}};
    event.user_id = user.getId();
    eventStore.appendEvent(event);
    
    // Update viewer count
    stream.updateViewerCount(redis, 100);
    
    // Stop streaming
    stream.stopStream(db, redis);
    
    // Cleanup
    db.cleanup();
    redis.cleanup();
    
    return 0;
}
```

## Performance Considerations

### Connection Pooling

The database manager maintains a connection pool (default 20 connections) to minimize connection overhead:

```cpp
// Adjust pool size based on your needs
db.init(connStr, 50); // 50 connections
```

### Redis Caching

Use Redis to cache frequently accessed data:

- Session data (TTL: 24 hours)
- Stream live status (TTL: 1 hour)
- Viewer counts (TTL: 5 minutes)

### Indexing

The schema includes indexes on commonly queried columns:
- `users.username`, `users.email`
- `sessions.user_id`, `sessions.expires_at`
- `streams.stream_key`, `streams.is_live`
- `event_log.aggregate_type`, `event_log.aggregate_id`

### Event Sourcing Optimization

Use snapshots to avoid replaying all events:

```cpp
// Create snapshot every 100 events
if (version % 100 == 0) {
    eventStore.createSnapshot(aggregateType, aggregateId, version, state);
}
```

## Testing

Run database tests:

```bash
# Run unit tests
./build/tests/database_tests

# Run integration tests (requires running database)
./build/tests/integration_tests
```

## Security

### SQL Injection Prevention

Use parameterized queries (prepared statements):

```cpp
// Use executeParams for user input
std::vector<std::string> params = {username, email};
db.executeParams("SELECT * FROM users WHERE username=$1 AND email=$2", params);
```

### Password Hashing

Always hash passwords before storing:

```cpp
// Use bcrypt or argon2 for password hashing
std::string hash = hashPassword(plaintext);
User::createUser(db, username, email, hash);
```

### Connection Security

Use SSL/TLS for database connections:

```
postgresql://user:pass@localhost/db?sslmode=require
```

## Monitoring

### Health Checks

```cpp
// Check database health
if (!db.isConnected()) {
    // Handle database connection issues
}

// Check Redis health
if (!redis.isConnected()) {
    // Handle Redis connection issues
}
```

### Metrics

Track key metrics:
- Connection pool utilization
- Query execution time
- Cache hit/miss ratio
- Event log growth rate

## Troubleshooting

### Connection Issues

```bash
# Test PostgreSQL connection
psql -U rootstream -d rootstream -c "SELECT 1;"

# Test Redis connection
redis-cli ping
```

### Performance Issues

```sql
-- Check slow queries
SELECT * FROM pg_stat_statements ORDER BY mean_exec_time DESC LIMIT 10;

-- Check index usage
SELECT * FROM pg_stat_user_indexes;
```

### Migration Issues

```bash
# Check applied migrations
psql -U rootstream -d rootstream -c "SELECT * FROM schema_migrations;"
```

## Contributing

When adding new features:

1. Update the schema in `schema.sql`
2. Create migration files in `migrations/`
3. Add corresponding model classes
4. Write unit tests
5. Update this documentation

## License

MIT License - See LICENSE file for details
