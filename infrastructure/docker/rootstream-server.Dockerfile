FROM ubuntu:22.04

LABEL maintainer="RootStream Team"
LABEL description="RootStream Server - Secure P2P Game Streaming"

WORKDIR /app

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libdrm2 \
    libva2 \
    libva-drm2 \
    libopus0 \
    libsodium23 \
    libpng16-16 \
    libqrencode4 \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Copy application binary
COPY build/rootstream /app/rootstream

# Copy configuration
COPY config/ /app/config/

# Create data directory
RUN mkdir -p /app/data

# Expose streaming ports
# 5000 - Main streaming port
# 5001 - Control/signaling port
EXPOSE 5000/udp 5001/tcp

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD nc -z localhost 5001 || exit 1

# Run as non-root user
RUN useradd -m -u 1000 rootstream && \
    chown -R rootstream:rootstream /app
USER rootstream

# Start server
CMD ["/app/rootstream", "--config", "/app/config/server.conf"]
