-- RootStream Database Schema
-- PostgreSQL 12+
-- Phase 24.2: Database Layer & State Management

-- ============================================================================
-- Users Table
-- ============================================================================
CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(255) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    display_name VARCHAR(255),
    avatar_url VARCHAR(512),
    is_verified BOOLEAN DEFAULT FALSE,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login_at TIMESTAMP,
    
    CONSTRAINT email_format CHECK (email ~ '^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}$')
);

CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_users_active ON users(is_active);

-- ============================================================================
-- Sessions Table
-- ============================================================================
CREATE TABLE IF NOT EXISTS sessions (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    session_token VARCHAR(512) UNIQUE NOT NULL,
    device_id VARCHAR(255),
    user_agent TEXT,
    ip_address INET,
    expires_at TIMESTAMP NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_activity TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE,
    
    CONSTRAINT valid_expiration CHECK (expires_at > created_at)
);

CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_sessions_token ON sessions(session_token);
CREATE INDEX IF NOT EXISTS idx_sessions_expires_at ON sessions(expires_at);
CREATE INDEX IF NOT EXISTS idx_sessions_active ON sessions(is_active, expires_at);

-- ============================================================================
-- Streams Table
-- ============================================================================
CREATE TABLE IF NOT EXISTS streams (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    name VARCHAR(255) NOT NULL,
    description TEXT,
    stream_key VARCHAR(512) UNIQUE NOT NULL,
    stream_url VARCHAR(512),
    thumbnail_url VARCHAR(512),
    is_live BOOLEAN DEFAULT FALSE,
    viewer_count INTEGER DEFAULT 0,
    bitrate_kbps INTEGER,
    resolution VARCHAR(50),
    fps INTEGER,
    codec VARCHAR(50),
    is_public BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    started_at TIMESTAMP,
    ended_at TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_streams_user_id ON streams(user_id);
CREATE INDEX IF NOT EXISTS idx_streams_is_live ON streams(is_live);
CREATE INDEX IF NOT EXISTS idx_streams_stream_key ON streams(stream_key);
CREATE INDEX IF NOT EXISTS idx_streams_public_live ON streams(is_public, is_live);

-- ============================================================================
-- Stream Sessions Table (tracks each stream session)
-- ============================================================================
CREATE TABLE IF NOT EXISTS stream_sessions (
    id SERIAL PRIMARY KEY,
    stream_id INTEGER NOT NULL REFERENCES streams(id) ON DELETE CASCADE,
    session_start TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    session_end TIMESTAMP,
    total_viewers INTEGER DEFAULT 0,
    peak_viewers INTEGER DEFAULT 0,
    total_bytes_sent BIGINT DEFAULT 0,
    duration_seconds INTEGER,
    is_recorded BOOLEAN DEFAULT FALSE,
    recording_path VARCHAR(512),
    
    CONSTRAINT valid_session_time CHECK (session_end IS NULL OR session_end > session_start)
);

CREATE INDEX IF NOT EXISTS idx_stream_sessions_stream_id ON stream_sessions(stream_id);
CREATE INDEX IF NOT EXISTS idx_stream_sessions_start ON stream_sessions(session_start DESC);

-- ============================================================================
-- Recording Metadata Table
-- ============================================================================
CREATE TABLE IF NOT EXISTS recordings (
    id SERIAL PRIMARY KEY,
    stream_id INTEGER NOT NULL REFERENCES streams(id) ON DELETE CASCADE,
    session_id INTEGER NOT NULL REFERENCES stream_sessions(id) ON DELETE CASCADE,
    file_path VARCHAR(512) NOT NULL,
    file_size_bytes BIGINT,
    duration_seconds INTEGER,
    codec VARCHAR(50),
    resolution VARCHAR(50),
    fps INTEGER,
    bitrate_kbps INTEGER,
    is_processed BOOLEAN DEFAULT FALSE,
    is_available BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP,
    
    CONSTRAINT valid_file_size CHECK (file_size_bytes IS NULL OR file_size_bytes > 0)
);

CREATE INDEX IF NOT EXISTS idx_recordings_stream_id ON recordings(stream_id);
CREATE INDEX IF NOT EXISTS idx_recordings_session_id ON recordings(session_id);
CREATE INDEX IF NOT EXISTS idx_recordings_expires_at ON recordings(expires_at);
CREATE INDEX IF NOT EXISTS idx_recordings_available ON recordings(is_available);

-- ============================================================================
-- Usage Tracking Table
-- ============================================================================
-- Usage Tracking Table
-- Note: stream_id uses SET NULL to preserve usage history after stream deletion
-- ============================================================================
CREATE TABLE IF NOT EXISTS usage_logs (
    id BIGSERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    stream_id INTEGER REFERENCES streams(id) ON DELETE SET NULL,
    event_type VARCHAR(50), -- 'stream_start', 'stream_end', 'viewer_join', 'viewer_leave'
    bytes_transferred BIGINT DEFAULT 0,
    duration_seconds INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_usage_logs_user_id ON usage_logs(user_id);
CREATE INDEX IF NOT EXISTS idx_usage_logs_stream_id ON usage_logs(stream_id);
CREATE INDEX IF NOT EXISTS idx_usage_logs_created_at ON usage_logs(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_usage_logs_event_type ON usage_logs(event_type);

-- ============================================================================
-- Billing Table
-- ============================================================================
CREATE TABLE IF NOT EXISTS billing_accounts (
    id SERIAL PRIMARY KEY,
    user_id INTEGER UNIQUE NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    payment_method VARCHAR(50), -- 'credit_card', 'paypal', etc
    subscription_tier VARCHAR(50), -- 'free', 'pro', 'enterprise'
    monthly_limit_gb INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_billing_user_id ON billing_accounts(user_id);
CREATE INDEX IF NOT EXISTS idx_billing_tier ON billing_accounts(subscription_tier);

-- ============================================================================
-- Event Log (for event sourcing)
-- ============================================================================
CREATE TABLE IF NOT EXISTS event_log (
    id BIGSERIAL PRIMARY KEY,
    aggregate_type VARCHAR(50), -- 'User', 'Stream', 'Session'
    aggregate_id INTEGER,
    event_type VARCHAR(100),
    event_data JSONB NOT NULL,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    version INTEGER,
    user_id INTEGER REFERENCES users(id) ON DELETE SET NULL
);

CREATE INDEX IF NOT EXISTS idx_event_log_aggregate ON event_log(aggregate_type, aggregate_id);
CREATE INDEX IF NOT EXISTS idx_event_log_timestamp ON event_log(timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_event_log_type ON event_log(event_type);
CREATE INDEX IF NOT EXISTS idx_event_log_user_id ON event_log(user_id);

-- ============================================================================
-- Snapshots (for event sourcing optimization)
-- ============================================================================
CREATE TABLE IF NOT EXISTS snapshots (
    id BIGSERIAL PRIMARY KEY,
    aggregate_type VARCHAR(50),
    aggregate_id INTEGER,
    version INTEGER,
    state JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT unique_aggregate_snapshot UNIQUE(aggregate_type, aggregate_id, version)
);

CREATE INDEX IF NOT EXISTS idx_snapshots_aggregate ON snapshots(aggregate_type, aggregate_id);
CREATE INDEX IF NOT EXISTS idx_snapshots_version ON snapshots(aggregate_type, aggregate_id, version DESC);

-- ============================================================================
-- Triggers for updated_at timestamps
-- ============================================================================

CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

CREATE TRIGGER update_users_updated_at BEFORE UPDATE ON users
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_streams_updated_at BEFORE UPDATE ON streams
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_billing_updated_at BEFORE UPDATE ON billing_accounts
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();
