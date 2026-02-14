# RootStream Web Dashboard - Deployment Guide

## Overview

This guide explains how to deploy the RootStream Web Dashboard for remote monitoring and management of streaming sessions.

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                  User Browser                        │
│  ┌──────────────────────────────────────────────┐  │
│  │  React SPA (Port 3000 dev, served via nginx) │  │
│  └──────────────────────────────────────────────┘  │
└──────────────┬────────────────────────────┬─────────┘
               │ HTTP/HTTPS                 │ WebSocket
               ▼                            ▼
        ┌──────────────┐           ┌──────────────┐
        │ REST API     │           │ WebSocket    │
        │ Port 8080    │           │ Port 8081    │
        └──────┬───────┘           └──────┬───────┘
               │                          │
               └──────────┬───────────────┘
                          │
                  ┌───────▼────────┐
                  │  RootStream    │
                  │  Application   │
                  └────────────────┘
```

## Prerequisites

### System Requirements

- Linux x86_64 (Ubuntu 22.04+ or equivalent)
- 4GB+ RAM
- Network connectivity
- Root or sudo access (for initial setup)

### Software Requirements

#### Backend (C/C++)
- GCC/G++ 11+ or Clang 13+
- CMake 3.16+
- System libraries (automatically installed):
  - libsodium
  - SDL2
  - Opus
  - VA-API (for hardware encoding)
  - PulseAudio/PipeWire
  - FFmpeg (optional, for recording)
  - GTK3 (for system tray)
  - Avahi (for mDNS discovery)

#### Frontend (React)
- Node.js 16+ and npm
- Modern web browser (Chrome, Firefox, Edge, Safari)

---

## Installation

### Step 1: Install System Dependencies

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake pkg-config \
    libsdl2-dev libsodium-dev libopus-dev \
    libdrm-dev libva-dev libasound2-dev \
    libpulse-dev libavahi-client-dev \
    libqrencode-dev libpng-dev libx11-dev \
    libncurses-dev libgtk-3-dev \
    libavformat-dev libavcodec-dev libavutil-dev \
    nodejs npm

# Arch Linux
sudo pacman -S base-devel cmake pkgconf \
    sdl2 libsodium opus libdrm libva \
    alsa-lib pulseaudio avahi libqrencode \
    libpng libx11 ncurses gtk3 ffmpeg \
    nodejs npm

# Fedora
sudo dnf install gcc gcc-c++ cmake pkgconfig \
    SDL2-devel libsodium-devel opus-devel \
    libdrm-devel libva-devel alsa-lib-devel \
    pulseaudio-libs-devel avahi-devel \
    qrencode-devel libpng-devel libX11-devel \
    ncurses-devel gtk3-devel ffmpeg-devel \
    nodejs npm
```

### Step 2: Build RootStream with Web Dashboard

```bash
# Clone repository (if not already done)
git clone https://github.com/infinityabundance/RootStream.git
cd RootStream

# Create build directory
mkdir -p build
cd build

# Configure with web dashboard enabled
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WEB_DASHBOARD=ON \
    -DBUILD_HOST=ON

# Build
make -j$(nproc)

# Optional: Install system-wide
sudo make install
```

### Step 3: Build Frontend Application

```bash
cd ../frontend

# Install dependencies
npm install

# Build for production
npm run build

# This creates a production build in frontend/build/
```

---

## Configuration

### Backend Configuration

Create `/etc/rootstream/web-dashboard.conf`:

```ini
[api_server]
port = 8080
enable_https = false
max_connections = 100
timeout_seconds = 30

[websocket_server]
port = 8081
enable_wss = false

[authentication]
# Default admin user (change password after first login)
default_user = admin
default_password = admin

[rate_limiting]
requests_per_minute = 1000

[security]
# Enable CORS (for development)
enable_cors = true
# Allowed origins (comma-separated)
allowed_origins = http://localhost:3000,http://localhost:8080
```

### Frontend Configuration

Create `frontend/.env.production`:

```bash
REACT_APP_API_URL=http://your-server-ip:8080/api
REACT_APP_WS_URL=ws://your-server-ip:8081
```

---

## Deployment Options

### Option 1: Development Mode (Quick Start)

**Terminal 1: Run RootStream with Web Dashboard**
```bash
cd RootStream/build
./rootstream --enable-web-dashboard
```

**Terminal 2: Run Frontend Development Server**
```bash
cd RootStream/frontend
npm start
```

Access dashboard at: http://localhost:3000

### Option 2: Production with systemd + nginx

#### A. Create systemd service

Create `/etc/systemd/system/rootstream.service`:

```ini
[Unit]
Description=RootStream with Web Dashboard
After=network.target

[Service]
Type=simple
User=root
ExecStart=/usr/local/bin/rootstream --enable-web-dashboard
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable rootstream
sudo systemctl start rootstream
```

#### B. Configure nginx

Create `/etc/nginx/sites-available/rootstream`:

```nginx
server {
    listen 80;
    server_name your-domain.com;

    # Frontend static files
    location / {
        root /var/www/rootstream;
        index index.html;
        try_files $uri $uri/ /index.html;
    }

    # REST API proxy
    location /api/ {
        proxy_pass http://localhost:8080/api/;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }

    # WebSocket proxy
    location /ws/ {
        proxy_pass http://localhost:8081/;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "Upgrade";
        proxy_set_header Host $host;
    }
}
```

Deploy frontend files:
```bash
sudo mkdir -p /var/www/rootstream
sudo cp -r frontend/build/* /var/www/rootstream/
sudo chown -R www-data:www-data /var/www/rootstream
```

Enable site:
```bash
sudo ln -s /etc/nginx/sites-available/rootstream /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

#### C. Enable HTTPS (Recommended)

```bash
# Install certbot
sudo apt-get install certbot python3-certbot-nginx

# Obtain certificate
sudo certbot --nginx -d your-domain.com

# Auto-renewal is configured automatically
```

### Option 3: Docker Deployment

Create `Dockerfile`:

```dockerfile
FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential cmake pkg-config \
    libsdl2-dev libsodium-dev libopus-dev \
    libdrm-dev libva-dev libasound2-dev \
    libpulse-dev libavahi-client-dev \
    libqrencode-dev libpng-dev libx11-dev \
    libncurses-dev libgtk-3-dev \
    libavformat-dev libavcodec-dev libavutil-dev \
    nodejs npm nginx

# Copy source
COPY . /app
WORKDIR /app

# Build backend
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_WEB_DASHBOARD=ON && \
    make -j$(nproc)

# Build frontend
RUN cd frontend && npm install && npm run build

# Configure nginx
COPY deploy/nginx.conf /etc/nginx/sites-available/default
RUN cp -r frontend/build/* /var/www/html/

EXPOSE 80 8080 8081

CMD service nginx start && /app/build/rootstream --enable-web-dashboard
```

Build and run:
```bash
docker build -t rootstream-dashboard .
docker run -d -p 80:80 -p 8080:8080 -p 8081:8081 \
    --device /dev/dri:/dev/dri \
    rootstream-dashboard
```

---

## Security Hardening

### 1. Configure Initial Admin Credentials

**IMPORTANT**: No default credentials are created. You must set up an initial admin account using environment variables:

```bash
# Set environment variables before starting RootStream
export ROOTSTREAM_ADMIN_USERNAME="your_admin_username"
export ROOTSTREAM_ADMIN_PASSWORD="YourSecurePassword123!"

# Start RootStream - it will create the admin user on first run
./rootstream-host

# Login with your configured credentials
curl -X POST http://localhost:8080/api/auth/login \
    -H "Content-Type: application/json" \
    -d '{"username":"your_admin_username","password":"YourSecurePassword123!"}'
```

**Password Requirements**:
- Minimum 8 characters
- Must contain at least one letter and one number
- Maximum 128 characters

### 2. Enable HTTPS

Always use HTTPS in production. Use Let's Encrypt (free) or your own certificates.

### 3. Firewall Configuration

```bash
# Allow only necessary ports
sudo ufw allow 80/tcp   # HTTP
sudo ufw allow 443/tcp  # HTTPS
sudo ufw enable

# If API is exposed directly (not recommended in production)
sudo ufw allow 8080/tcp
sudo ufw allow 8081/tcp
```

### 4. Rate Limiting

Configure in `/etc/rootstream/web-dashboard.conf`:
```ini
[rate_limiting]
requests_per_minute = 100  # Lower for production
```

### 5. User Roles

Create users with appropriate roles:
- **ADMIN**: Full access (limit to 1-2 users)
- **OPERATOR**: Can control streaming and settings
- **VIEWER**: Read-only access

---

## Monitoring

### Check Backend Status

```bash
# Check if running
systemctl status rootstream

# View logs
journalctl -u rootstream -f

# Check API health
curl http://localhost:8080/api/host/info
```

### Check Frontend Status

```bash
# nginx status
systemctl status nginx

# View logs
tail -f /var/log/nginx/access.log
tail -f /var/log/nginx/error.log
```

### WebSocket Connection Test

```bash
# Install websocat
sudo apt-get install websocat

# Test connection
websocat ws://localhost:8081
```

---

## Troubleshooting

### Backend Won't Start

```bash
# Check port conflicts
sudo netstat -tulpn | grep -E '8080|8081'

# Check dependencies
ldd build/rootstream | grep "not found"

# Check configuration
cat /etc/rootstream/web-dashboard.conf
```

### Frontend Connection Issues

1. Check browser console for errors (F12)
2. Verify API URL in `.env.production`
3. Check CORS headers in browser network tab
4. Test API directly: `curl http://localhost:8080/api/host/info`

### WebSocket Won't Connect

1. Check if WebSocket server is running
2. Verify port 8081 is accessible
3. Check firewall rules
4. Test with websocat (see above)

### Authentication Failures

1. Verify default credentials haven't been changed
2. Check token expiry (24 hours by default)
3. Clear browser localStorage
4. Check server logs for authentication errors

---

## Performance Tuning

### Backend

```ini
[api_server]
max_connections = 1000  # Increase for high traffic
timeout_seconds = 60    # Adjust based on needs

[rate_limiting]
requests_per_minute = 5000  # Increase if needed
```

### Frontend

```bash
# Enable production optimizations
npm run build

# Enable gzip in nginx
gzip on;
gzip_types text/plain text/css application/json application/javascript;
```

### Database (Future Enhancement)

Current implementation uses in-memory storage. For production:
- Consider adding Redis for session management
- Use PostgreSQL/MySQL for persistent storage
- Implement database connection pooling

---

## Backup and Recovery

### Backup Configuration

```bash
# Backup config files
sudo tar -czf rootstream-config-$(date +%Y%m%d).tar.gz \
    /etc/rootstream/

# Backup certificates (if any)
sudo tar -czf certs-$(date +%Y%m%d).tar.gz \
    /etc/letsencrypt/
```

### Recovery

```bash
# Restore configuration
sudo tar -xzf rootstream-config-YYYYMMDD.tar.gz -C /

# Restart services
sudo systemctl restart rootstream nginx
```

---

## Upgrading

```bash
# Stop services
sudo systemctl stop rootstream nginx

# Pull latest code
cd RootStream
git pull origin main

# Rebuild
cd build
cmake .. -DBUILD_WEB_DASHBOARD=ON
make -j$(nproc)
sudo make install

# Rebuild frontend
cd ../frontend
npm install
npm run build
sudo cp -r build/* /var/www/rootstream/

# Start services
sudo systemctl start rootstream nginx
```

---

## Support

For issues or questions:
1. Check TROUBLESHOOTING.md
2. Review logs (journalctl -u rootstream)
3. Open issue on GitHub: https://github.com/infinityabundance/RootStream/issues
4. Consult API documentation: docs/WEB_DASHBOARD_API.md

---

## License

See LICENSE file in the root directory.
