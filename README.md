# BitTorrent Lite 🌊

A lightweight, fully functional BitTorrent client and seeder written in **C++23** from scratch — no third-party torrent libraries. Supports tracker-based peer discovery, decentralized **Local Peer Discovery (LPD)** via UDP multicast, piece-level parallel downloading, and `.torrent` file creation.

---

## Table of Contents

- [Features](#features)
- [How It Works](#how-it-works)
- [Dependencies](#dependencies)
- [Building with CMake](#building-with-cmake)
- [CLI Usage](#cli-usage)
- [Peer Discovery Modes](#peer-discovery-modes)
- [Example Workflows](#example-workflows)
- [Troubleshooting](#troubleshooting)

---

## Features

- ✅ **Bencode parser** — full `.torrent` file decoder
- ✅ **Torrent creation** — generate `.torrent` files from any local file
- ✅ **Tracker communication** — HTTP GET to announce/scrape endpoints
- ✅ **BitTorrent wire protocol** — handshake, bitfield, interested, choke/unchoke, request, piece, have, cancel
- ✅ **Pipelined block requests** — multiple in-flight requests per peer for speed
- ✅ **Local Peer Discovery (LPD)** — UDP multicast on `239.192.152.143:6771` (BEP-14)
- ✅ **Seeder mode** — serve a local file to other peers on the network
- ✅ **Concurrent peer threads** — each peer runs in its own thread with a shared `Controller`
- ✅ **SHA-1 integrity verification** — every downloaded piece is hash-verified via OpenSSL

---

## How It Works

### 1. Encoding (Creating a `.torrent`)
The `encode` command reads a local file in 32 KB chunks, computes a SHA-1 hash for each chunk (piece), and writes a bencoded `.torrent` file embedding the tracker URL, file metadata, piece length, and concatenated piece hashes.

### 2. Parsing
The bencoded `.torrent` file is read and decoded by a hand-written parser that extracts the announce URL, file name, file size, piece length, and the list of piece SHA-1 hashes.

### 3. Tracker Communication
The client computes the `info_hash` (SHA-1 of the raw bencoded `info` dict), then sends an HTTP GET request to the tracker's announce URL. The tracker responds with a compact peer list (6 bytes per peer: 4 IP + 2 port).

### 4. Peer Wire Protocol
For each discovered peer, a thread is spawned that:
1. Performs a 68-byte BitTorrent **handshake** (validates `info_hash`)
2. Exchanges **bitfields** (which pieces each side has)
3. Sends **Interested** and waits for **Unchoke**
4. Pipelines **Request** messages for 16 KB blocks within each piece
5. Receives **Piece** messages and writes blocks to a `Controller` (shared piece manager)
6. Sends **Have** messages after each completed piece
7. Exits cleanly when all pieces are downloaded

### 5. Local Peer Discovery (LPD)
Both the seeder and leecher simultaneously run two background threads:
- **Broadcaster** — every 60 seconds, sends a UDP multicast `BT-SEARCH` advertisement to `239.192.152.143:6771` containing the `info_hash` and TCP listening port.
- **Listener** — subscribes to the same multicast group, parses incoming `BT-SEARCH` messages, matches `info_hash`, ignores its own cookie, and directly connects to matching local peers — bypassing the tracker entirely.

### 6. Seeder Mode
The seeder loads the complete local data file into the `Controller` (which maps pieces → disk blocks). It listens for incoming connections, performs handshakes, and serves piece blocks on demand to any leecher that requests them.

---

## Dependencies

### macOS

| Dependency | Purpose | Install |
|---|---|---|
| **CMake ≥ 3.10** | Build system | `brew install cmake` |
| **OpenSSL** | SHA-1 hashing | `brew install openssl` |
| **pthreads / std::thread** | Concurrency | Bundled with system libc++ |
| **C++23 compiler** | Language standard | `xcode-select --install` or `brew install llvm` |

> **macOS note:** The `CMakeLists.txt` automatically sets `OPENSSL_ROOT_DIR` to `/opt/homebrew/opt/openssl` for Apple Silicon Homebrew installs. If your OpenSSL lives elsewhere, pass it manually (see build steps below).

---

### Linux

| Dependency | Purpose | Debian / Ubuntu | Fedora / RHEL | Arch / Manjaro |
|---|---|---|---|---|
| **CMake ≥ 3.10** | Build system | `apt install cmake` | `dnf install cmake` | `pacman -S cmake` |
| **OpenSSL dev headers** | SHA-1 hashing | `apt install libssl-dev` | `dnf install openssl-devel` | `pacman -S openssl` |
| **GCC ≥ 13 or Clang ≥ 17** | C++23 compiler | `apt install g++` | `dnf install gcc-c++` | `pacman -S gcc` |
| **pthreads** | Concurrency | Bundled with glibc | Bundled with glibc | Bundled with glibc |
| **build tools** | Make + linker | `apt install build-essential` | `dnf groupinstall "Development Tools"` | `pacman -S base-devel` |

> **Linux note:** On older distros (Ubuntu 22.04 or earlier), the default GCC may be older than 13.
> Install a newer version with `sudo apt install gcc-13 g++-13` and pass `-DCMAKE_CXX_COMPILER=g++-13` to CMake.

---

## Building with CMake

### macOS

#### Step 1 — Install dependencies

```bash
brew install cmake openssl
xcode-select --install    # installs Apple Clang with C++23 support
```

#### Step 2 — Enter the project directory

```bash
cd BitTorrent_Project
```

#### Step 3 — Create an out-of-source build directory

```bash
mkdir build && cd build
```

#### Step 4 — Configure

```bash
cmake ..
```

If CMake cannot locate OpenSSL automatically (common on Apple Silicon), point it explicitly:

```bash
cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
```

#### Step 5 — Compile

```bash
cmake --build . -j$(nproc)
```

#### macOS one-liner

```bash
mkdir -p build && cd build && cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) && cmake --build . -j$(nproc)
```

---

### Linux — Debian / Ubuntu

#### Step 1 — Install dependencies

```bash
sudo apt update
sudo apt install -y build-essential cmake libssl-dev
```

If your distro's default GCC is older than 13 (check with `g++ --version`):

```bash
sudo apt install -y gcc-13 g++-13
```

#### Step 2 — Enter the project directory

```bash
cd BitTorrent_Project
```

#### Step 3 — Create a build directory

```bash
mkdir build && cd build
```

#### Step 4 — Configure

```bash
cmake ..
```

If you installed GCC 13 manually, tell CMake to use it:

```bash
cmake .. -DCMAKE_CXX_COMPILER=g++-13
```

Build in Release mode (recommended — faster binary):

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

Build in Debug mode (with symbols):

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

#### Step 5 — Compile

```bash
cmake --build . -j$(nproc)
# or equivalently:
make -j$(nproc)
```

#### Debian / Ubuntu one-liner

```bash
mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . -j$(nproc)
```

---

### Linux — Fedora / RHEL / CentOS

#### Step 1 — Install dependencies

```bash
sudo dnf install -y gcc-c++ cmake openssl-devel
```

For RHEL / CentOS, also enable EPEL and PowerTools:

```bash
sudo dnf install -y epel-release
sudo dnf config-manager --set-enabled powertools   # CentOS 8
# or: sudo dnf config-manager --set-enabled crb    # CentOS 9 / RHEL 9
```

#### Step 2 — Configure and build

```bash
cd BitTorrent_Project
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

---

### Linux — Arch Linux / Manjaro

#### Step 1 — Install dependencies

```bash
sudo pacman -S --needed base-devel cmake openssl
```

#### Step 2 — Configure and build

```bash
cd BitTorrent_Project
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

---

The compiled binary will be at:

```
build/BitTorrentClient
```

### Common CMake flags (all platforms)

| Flag | Effect |
|---|---|
| `-DCMAKE_BUILD_TYPE=Release` | Optimised build, no debug symbols |
| `-DCMAKE_BUILD_TYPE=Debug` | Debug symbols, assertions enabled |
| `-DCMAKE_CXX_COMPILER=g++-13` | Force a specific compiler version |
| `-DOPENSSL_ROOT_DIR=<path>` | Point CMake to a non-standard OpenSSL install |

---

## CLI Usage

All commands follow this pattern:

```
./BitTorrentClient <command> [mode] <torrent_file> [data_file]
```

Run with no arguments or `--help` to see the full usage summary:

```bash
./BitTorrentClient --help
```

### Commands

| Command | Alias | Description |
|---|---|---|
| `encode` | — | Create a `.torrent` file from a local file |
| `info` | `decode` | Parse and display torrent metadata |
| `leecher` | `download` | Download a file using a `.torrent` |
| `seeder` | `seed` | Serve a local file to other peers |

### Optional `[mode]` argument (for `leecher` / `seeder`)

| Mode | Description |
|---|---|
| `general` | Use tracker + LPD (default) |
| `local` | Skip tracker, use LPD only (LAN-only, no internet required) |

---

### `encode` — Create a `.torrent` file

```bash
./BitTorrentClient encode <file>
```

**Example:**

```bash
./BitTorrentClient encode video.mp4
# Output: video.mp4.torrent
```

Reads `video.mp4` in 32 KB pieces, SHA-1 hashes each piece, and bencodes everything into `video.mp4.torrent` using a default public tracker.

---

### `info` — Inspect a `.torrent` file

```bash
./BitTorrentClient info <torrent_file>
```

**Example:**

```bash
./BitTorrentClient info video.mp4.torrent
```

Prints tracker URL, file name, file size, piece length, number of pieces, and all SHA-1 piece hashes.

---

### `leecher` / `download` — Download a file

```bash
# General mode (tracker + LPD)
./BitTorrentClient leecher general <torrent_file>
./BitTorrentClient download general video.mp4.torrent

# Local-only mode (LPD only, no tracker)
./BitTorrentClient leecher local <torrent_file>
./BitTorrentClient download local video.mp4.torrent

# Short form (defaults to general mode)
./BitTorrentClient leecher video.mp4.torrent
```

The downloaded file is saved using the name stored inside the `.torrent` file.

---

### `seeder` / `seed` — Seed a file

```bash
# General mode (registers with tracker + LPD)
./BitTorrentClient seeder general <torrent_file> <data_file>
./BitTorrentClient seed general video.mp4.torrent video.mp4

# Local-only mode (LPD only)
./BitTorrentClient seeder local <torrent_file> <data_file>
./BitTorrentClient seed local video.mp4.torrent video.mp4

# Short form (data file defaults to torrent's stored name)
./BitTorrentClient seeder video.mp4.torrent video.mp4
```

The seeder runs indefinitely, serving the file to any connecting peer. Press **Ctrl+C** to stop.


## Example Workflows

### Workflow A — Full tracker + LAN transfer

**Terminal 1 (Seeder):**
```bash
./BitTorrentClient encode myfile.zip
./BitTorrentClient seed general myfile.zip.torrent myfile.zip
```

**Terminal 2 (Leecher):**
```bash
./BitTorrentClient download general myfile.zip.torrent
```

---

### Workflow B — LAN-only (no tracker needed)

**Terminal 1 (Seeder):**
```bash
./BitTorrentClient seed local myfile.zip.torrent myfile.zip
```

**Terminal 2 (Leecher, same machine or same LAN):**
```bash
./BitTorrentClient download local myfile.zip.torrent
```

---

### Workflow C — Multi-instance test on one machine

```bash
# Build first
mkdir -p build && cd build && cmake .. && cmake --build . -j$(nproc)

# Terminal 1 — seed
./BitTorrentClient seed local ../video.mp4.torrent ../video.mp4

# Terminal 2 — leech
./BitTorrentClient download local ../video.mp4.torrent
```

> **Tip:** If both seeder and leecher run on the same machine, the LPD listener ignores its own broadcast using a unique cookie (`LPD_<timestamp>`), so only peer instances with different cookies will connect to each other.

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| `CMake Error: Could not find OpenSSL` (macOS) | `cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)` |
| `CMake Error: Could not find OpenSSL` (Linux) | `sudo apt install libssl-dev` / `sudo dnf install openssl-devel` |
| `error: use of undeclared identifier` / C++23 errors | Your compiler is too old. Install GCC ≥ 13 and pass `-DCMAKE_CXX_COMPILER=g++-13` |
| `[LPD] Failed to bind to 6771` | Another instance already holds the port — `SO_REUSEPORT` should handle this; check firewall rules |
| `[leecher] no peers from tracker` | Tracker may be down. Switch to `local` mode or use a different tracker URL |
| Port 6881 already in use | Kill previous instance: `lsof -ti:6881 \| xargs kill` (macOS/Linux) |
| Download stalls at 0% | The seeder may have disconnected. Restart seeder and re-run leecher |
| Multicast not working on Linux VMs | Some hypervisors block multicast by default — test on bare metal or allow multicast in the VM network settings |
