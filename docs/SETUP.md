# Setup Guide

## System Requirements

| Tool | Version | Install |
|---|---|---|
| GCC / G++ | 13+ | `sudo apt install gcc-13 g++-13` |
| CMake | 3.20+ | `sudo apt install cmake` |
| Drogon | 1.8.7 | See below |
| spdlog | Any | `sudo apt install libspdlog-dev` |
| jsoncpp | Any | `sudo apt install libjsoncpp-dev` |
| MySQL Connector/C++ | 1.1.x | `sudo apt install libmysqlcppconn-dev` |
| git | Any | `sudo apt install git` |
| Node.js | 20+ | See below |
| Docker | Any | Optional — for MySQL |

---

## Backend Setup

### 1. Install system packages (Ubuntu 24.04)

```bash
sudo apt update
sudo apt install -y cmake gcc-13 g++-13 \
  libspdlog-dev libjsoncpp-dev \
  libmysqlcppconn-dev git
```

### 2. Install Drogon

```bash
# Dependencies
sudo apt install -y libjsoncpp-dev libssl-dev uuid-dev zlib1g-dev

# Build from source
git clone https://github.com/drogonframework/drogon.git
cd drogon && git submodule update --init
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
cd ../..
```

### 3. Build the backend

```bash
cd backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

Produces: `backend/build/bin/cortex`

### 4. Configure (optional)

Edit `backend/config/config.json`:

```json
{
  "server": {
    "host": "127.0.0.1",
    "port": 8080,
    "threads": 4
  },
  "logging": {
    "level": "info",
    "file": "logs/cortex.log"
  }
}
```

| Key | Default | Description |
|---|---|---|
| `server.host` | `127.0.0.1` | Bind address |
| `server.port` | `8080` | HTTP port |
| `server.threads` | `4` | Drogon worker threads |
| `logging.level` | `info` | trace, debug, info, warn, error |
| `logging.file` | `logs/cortex.log` | Rotating log file path |

### 5. Run the backend

```bash
cd backend
./build/bin/cortex
```

Expected output:
```
[info] Dependency graph built successfully
[info] Registered route: GET /health
[info] Registered route: POST /repositories
[info] Registered route: GET /jobs/{jobId}
[info] Registered route: GET /analysis/{jobId}
[info] Background job worker started
[info] Starting HTTP server...
```

---

## Frontend Setup

### 1. Install Node.js 20+

```bash
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt install -y nodejs
```

### 2. Install dependencies and run

```bash
cd frontend
npm install
npm run dev
```

Frontend available at **http://localhost:3000**

The Vite dev server proxies all API calls (`/repositories`, `/jobs`, `/analysis`, `/health`) to `http://127.0.0.1:8080` — no CORS configuration needed.

---

## MySQL Setup (Optional)

By default the backend uses in-memory storage. To enable MySQL persistence:

### Using Docker Compose (recommended)

```bash
docker-compose up -d
```

This starts MySQL 8 with:
- Database: `cortex`
- User: `cortex` / Password: `cortex`
- Port: `3306`
- Auto-applies migration: `backend/database/migrations/001_create_jobs.sql`

### Manual MySQL setup

```bash
# Install MySQL
sudo apt install -y mysql-server

# Create database and user
sudo mysql -e "
  CREATE DATABASE IF NOT EXISTS cortex;
  CREATE USER IF NOT EXISTS 'cortex'@'localhost' IDENTIFIED BY 'cortex';
  GRANT ALL PRIVILEGES ON cortex.* TO 'cortex'@'localhost';
  FLUSH PRIVILEGES;
"

# Apply migration
mysql -u cortex -pcortex cortex < backend/database/migrations/001_create_jobs.sql
```

When MySQL is running on `localhost:3306`, the backend connects automatically and logs:
```
[info] Using MySQLJobRepository
```

When MySQL is unavailable, the backend logs a warning and continues:
```
[warning] MySQL unavailable — falling back to InMemoryJobRepository
```

---

## Running Both Services

**Terminal 1:**
```bash
cd backend && ./build/bin/cortex
```

**Terminal 2:**
```bash
cd frontend && npm run dev
```

**Stop all:**
```bash
bash stop.sh
```

---

## Troubleshooting

### `Address already in use (errno=98)` on port 8080

Another instance is running. Kill it:
```bash
pkill -f 'build/bin/cortex'
```

### Frontend port 3000 already in use

Vite will automatically try port 3001, 3002, etc. Check the terminal output for the actual URL.

### `cppconn/connection.h: No such file or directory`

Install MySQL Connector/C++ development headers:
```bash
sudo apt install libmysqlcppconn-dev
```

### `git clone` fails in worker

- Confirm `git` is installed: `which git`
- Confirm the repository is public
- Check network access from WSL

### Logs

```bash
# Real-time backend logs
tail -f backend/logs/cortex.log

# Check worker clone output
ls /tmp/cortex-workspace/
```
