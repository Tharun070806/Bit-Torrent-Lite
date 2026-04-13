cmake_minimum_required(VERSION 3.10)

# Name of the project and specify C++ language
project(BitTorrentClient VERSION 1.0 LANGUAGES CXX)

# Enforce the C++23 Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# macOS specific hint for OpenSSL Homebrew installation
if(APPLE)
    set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl)
endif()

# Dynamically find installed system libraries (Cross-platform)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# List all the source files needed to compile
set(SOURCES
    src/main.cpp
    src/parser/decoder.cpp
    src/utils/hasher.cpp
    src/utils/helper.cpp
    src/requestor/requestor.cpp
    src/utils/socket_utils.cpp
    src/listener/listener.cpp
    src/peer/peer.cpp
    src/peer/peer_messaging.cpp
    src/peer/peer_handshake.cpp
    src/lpd/lpd.cpp
    src/leecher/leecher.cpp
    src/seeder/seeder.cpp
)

# Tell CMake to build an executable file from the sources
add_executable(BitTorrentClient ${SOURCES})

# Link the dynamically found libraries (OpenSSL and Threads) to the executable
target_link_libraries(BitTorrentClient PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
