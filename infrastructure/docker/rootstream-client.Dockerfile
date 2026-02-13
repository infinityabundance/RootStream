FROM ubuntu:22.04

LABEL maintainer="RootStream Team"
LABEL description="RootStream Client - Secure P2P Game Streaming Client"

WORKDIR /app

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libsdl2-2.0-0 \
    libva2 \
    libva-drm2 \
    libopus0 \
    libsodium23 \
    libasound2 \
    libpulse0 \
    libx11-6 \
    && rm -rf /var/lib/apt/lists/*

# Copy client binary
COPY clients/kde-plasma-client/build/rootstream-client /app/rootstream-client

# Copy configuration
COPY config/ /app/config/

# Create cache directory
RUN mkdir -p /app/cache

# Run as non-root user
RUN useradd -m -u 1000 rootstream && \
    chown -R rootstream:rootstream /app
USER rootstream

# Start client
CMD ["/app/rootstream-client"]
