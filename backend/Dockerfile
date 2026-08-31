# Multi-stage build for Cortex Code Intelligence Platform backend

# Stage 1: Builder
FROM ubuntu:24.04 AS builder

WORKDIR /build

# Install build dependencies including CA certificates
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    build-essential \
    cmake \
    gcc-13 \
    g++-13 \
    pkg-config \
    libspdlog-dev \
    libjsoncpp-dev \
    libhiredis-dev \
    libmysqlcppconn-dev \
    libssl-dev \
    uuid-dev \
    libc-ares-dev \
    zlib1g-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Install Drogon from source
RUN git clone https://github.com/drogonframework/drogon.git /drogon && \
    cd /drogon && \
    git submodule update --init && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-13 \
        -DBUILD_SQLITE=OFF -DBUILD_POSTGRESQL=OFF && \
    make -j$(nproc) && \
    make install && \
    cd / && rm -rf /drogon

# Copy source
COPY . /cortex-src
WORKDIR /cortex-src

# Build Cortex backend
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-13 && \
    cmake --build . --parallel

# Stage 2: Runtime
FROM ubuntu:24.04

WORKDIR /app

# Install runtime dependencies only
RUN apt-get update && apt-get install -y --no-install-recommends \
    libspdlog1.12 \
    libjsoncpp25 \
    libhiredis1.1.0 \
    libmysqlcppconn7t64 \
    libssl3 \
    libcares2 \
    zlib1g \
    ca-certificates \
    curl \
    git \
    && rm -rf /var/lib/apt/lists/*

# Copy binary and config from builder
COPY --from=builder /cortex-src/build/bin/cortex /app/cortex
COPY --from=builder /cortex-src/config/config.json /app/config/config.json

# Create logs directory
RUN mkdir -p /app/logs

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Run the application
CMD ["/app/cortex"]
