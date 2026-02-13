# RootStream Web Dashboard API Documentation

## Overview

The RootStream Web Dashboard provides a comprehensive REST API and WebSocket interface for remote monitoring and management of streaming sessions.

- **Base URL**: `http://localhost:8080/api`
- **WebSocket URL**: `ws://localhost:8081`
- **Authentication**: JWT Bearer tokens
- **Content-Type**: `application/json`

---

## Authentication

### Login

**Endpoint**: `POST /api/auth/login`

**Request Body**:
```json
{
  "username": "admin",
  "password": "password"
}
```

**Response**:
```json
{
  "success": true,
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "role": "ADMIN"
}
```

**Status Codes**:
- `200 OK` - Authentication successful
- `401 Unauthorized` - Invalid credentials

---

### Logout

**Endpoint**: `POST /api/auth/logout`

**Headers**: 
```
Authorization: Bearer <token>
```

**Response**:
```json
{
  "success": true,
  "message": "Logged out"
}
```

---

### Verify Token

**Endpoint**: `GET /api/auth/verify`

**Headers**: 
```
Authorization: Bearer <token>
```

**Response**:
```json
{
  "valid": true,
  "username": "admin",
  "role": "ADMIN"
}
```

---

## Host Management

### Get Host Information

**Endpoint**: `GET /api/host/info`

**Response**:
```json
{
  "hostname": "gaming-pc",
  "platform": "Linux",
  "rootstream_version": "1.0.0",
  "uptime_seconds": 3600,
  "is_streaming": true
}
```

---

### Start Host

**Endpoint**: `POST /api/host/start`

**Response**:
```json
{
  "success": true,
  "message": "Host started"
}
```

**Permissions**: ADMIN, OPERATOR

---

### Stop Host

**Endpoint**: `POST /api/host/stop`

**Response**:
```json
{
  "success": true,
  "message": "Host stopped"
}
```

**Permissions**: ADMIN, OPERATOR

---

## Metrics

### Get Current Metrics

**Endpoint**: `GET /api/metrics/current`

**Response**:
```json
{
  "fps": 60,
  "rtt_ms": 15,
  "jitter_ms": 2,
  "gpu_util": 45,
  "gpu_temp": 65,
  "cpu_util": 30,
  "bandwidth_mbps": 25.5,
  "packets_sent": 150000,
  "packets_lost": 12,
  "bytes_sent": 50000000,
  "timestamp_us": 1707813600000000
}
```

---

### Get Metrics History

**Endpoint**: `GET /api/metrics/history`

**Response**:
```json
{
  "fps_history": [60, 59, 60, 61, 60, ...],
  "latency_history": [15, 16, 14, 15, 17, ...],
  "gpu_util_history": [45, 46, 44, 45, 47, ...],
  "cpu_util_history": [30, 31, 29, 30, 32, ...]
}
```

---

## Peer Management

### Get Peers

**Endpoint**: `GET /api/peers`

**Response**:
```json
{
  "peers": [
    {
      "peer_id": "peer_123",
      "name": "Client-1",
      "capability": "client",
      "ip_address": "192.168.1.100",
      "port": 9090,
      "version": "1.0.0",
      "is_online": true,
      "last_seen_time_us": 1707813600000000
    }
  ]
}
```

---

## Stream Management

### Get Streams

**Endpoint**: `GET /api/streams`

**Response**:
```json
{
  "streams": [
    {
      "stream_id": "stream_001",
      "peer_name": "Client-1",
      "width": 1920,
      "height": 1080,
      "fps": 60,
      "start_time_us": 1707813600000000,
      "is_recording": true,
      "recording_file": "/recordings/stream_001.mkv",
      "recording_size_bytes": 104857600
    }
  ]
}
```

---

### Start Recording

**Endpoint**: `POST /api/streams/:stream_id/record`

**Response**:
```json
{
  "success": true,
  "message": "Recording started"
}
```

**Permissions**: ADMIN, OPERATOR

---

### Stop Recording

**Endpoint**: `POST /api/streams/:stream_id/stop-record`

**Response**:
```json
{
  "success": true,
  "message": "Recording stopped"
}
```

**Permissions**: ADMIN, OPERATOR

---

## Settings Management

### Get Video Settings

**Endpoint**: `GET /api/settings/video`

**Response**:
```json
{
  "width": 1920,
  "height": 1080,
  "fps": 60,
  "bitrate_kbps": 20000,
  "encoder": "vaapi",
  "codec": "h264"
}
```

---

### Update Video Settings

**Endpoint**: `PUT /api/settings/video`

**Request Body**:
```json
{
  "width": 2560,
  "height": 1440,
  "fps": 120,
  "bitrate_kbps": 30000,
  "encoder": "nvenc",
  "codec": "h265"
}
```

**Response**:
```json
{
  "success": true,
  "message": "Video settings updated"
}
```

**Permissions**: ADMIN, OPERATOR

---

### Get Audio Settings

**Endpoint**: `GET /api/settings/audio`

**Response**:
```json
{
  "output_device": "default",
  "input_device": "default",
  "sample_rate": 48000,
  "channels": 2,
  "bitrate_kbps": 128
}
```

---

### Update Audio Settings

**Endpoint**: `PUT /api/settings/audio`

**Request Body**:
```json
{
  "output_device": "headphones",
  "input_device": "microphone",
  "sample_rate": 48000,
  "channels": 2,
  "bitrate_kbps": 128
}
```

**Response**:
```json
{
  "success": true,
  "message": "Audio settings updated"
}
```

**Permissions**: ADMIN, OPERATOR

---

### Get Network Settings

**Endpoint**: `GET /api/settings/network`

**Response**:
```json
{
  "port": 9090,
  "target_bitrate_mbps": 25,
  "buffer_size_ms": 100,
  "enable_tcp_fallback": true,
  "enable_encryption": true
}
```

---

### Update Network Settings

**Endpoint**: `PUT /api/settings/network`

**Request Body**:
```json
{
  "port": 9090,
  "target_bitrate_mbps": 30,
  "buffer_size_ms": 150,
  "enable_tcp_fallback": true,
  "enable_encryption": true
}
```

**Response**:
```json
{
  "success": true,
  "message": "Network settings updated"
}
```

**Permissions**: ADMIN, OPERATOR

---

## WebSocket API

### Connection

Connect to: `ws://localhost:8081`

### Message Types

#### Metrics Update
```json
{
  "type": "metrics",
  "data": {
    "fps": 60,
    "rtt_ms": 15,
    "jitter_ms": 2,
    "gpu_util": 45,
    "gpu_temp": 65,
    "cpu_util": 30,
    "bandwidth_mbps": 25.5,
    "packets_sent": 150000,
    "packets_lost": 12,
    "bytes_sent": 50000000,
    "timestamp_us": 1707813600000000
  }
}
```

Frequency: 1 Hz (every second)

#### Event Notification
```json
{
  "type": "event",
  "event_type": "stream_started|stream_stopped|peer_connected|peer_disconnected|recording_started|recording_stopped",
  "data": "..."
}
```

#### Command (Client to Server)
```json
{
  "type": "command",
  "command": "start_streaming|stop_streaming|start_recording|stop_recording",
  "params": {}
}
```

#### Acknowledgment
```json
{
  "type": "ack",
  "message_id": 12345,
  "success": true
}
```

---

## User Roles

### ADMIN
- Full access to all endpoints
- Can manage users
- Can modify all settings
- Can control streaming

### OPERATOR
- Can control streaming (start/stop)
- Can modify settings
- Can manage recordings
- Cannot manage users

### VIEWER
- Read-only access
- Can view metrics and status
- Cannot modify settings
- Cannot control streaming

---

## Rate Limiting

- **Default limit**: 1000 requests per minute per IP
- **429 Too Many Requests** returned when limit exceeded
- Rate limits reset every 60 seconds

---

## Error Responses

All errors follow this format:

```json
{
  "error": true,
  "status": 400,
  "message": "Error description"
}
```

**Common Status Codes**:
- `400 Bad Request` - Invalid input
- `401 Unauthorized` - Missing or invalid token
- `403 Forbidden` - Insufficient permissions
- `404 Not Found` - Resource not found
- `429 Too Many Requests` - Rate limit exceeded
- `500 Internal Server Error` - Server error

---

## Examples

### Using curl

```bash
# Login
TOKEN=$(curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin"}' \
  | jq -r '.token')

# Get host info
curl http://localhost:8080/api/host/info \
  -H "Authorization: Bearer $TOKEN"

# Get current metrics
curl http://localhost:8080/api/metrics/current \
  -H "Authorization: Bearer $TOKEN"

# Update video settings
curl -X PUT http://localhost:8080/api/settings/video \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"width":1920,"height":1080,"fps":60}'
```

### Using JavaScript/Fetch

```javascript
// Login
const response = await fetch('/api/auth/login', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ username: 'admin', password: 'admin' })
});
const { token } = await response.json();
localStorage.setItem('authToken', token);

// Get metrics
const metrics = await fetch('/api/metrics/current', {
  headers: { 'Authorization': `Bearer ${token}` }
}).then(r => r.json());

// WebSocket
const ws = new WebSocket('ws://localhost:8081');
ws.onmessage = (event) => {
  const message = JSON.parse(event.data);
  console.log('Received:', message);
};
```

---

## Security Considerations

1. **Always use HTTPS in production** (enable with `enable_https: true`)
2. **Store JWT tokens securely** (HttpOnly cookies recommended)
3. **Validate all inputs** on the server side
4. **Use strong passwords** for user accounts
5. **Enable rate limiting** to prevent abuse
6. **Regular token rotation** (24-hour expiry by default)
7. **Monitor for suspicious activity** in logs
8. **Use WSS (WebSocket Secure)** in production

---

## Support

For issues or questions, please refer to the main RootStream documentation or open an issue on GitHub.
