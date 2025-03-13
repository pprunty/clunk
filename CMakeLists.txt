cmake_minimum_required(VERSION 3.15)
project(clunk VERSION 0.1.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compiler flags for performance
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O3 -march=native)
endif()

# Default install location that doesn't require admin privileges
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path" FORCE)
endif()

# Find required packages (using vcpkg)
find_package(Boost REQUIRED COMPONENTS system)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Main executable
add_executable(clunk
    src/main.cpp
    src/orderbook/order_book.cpp
    src/orderbook/order.cpp
    src/orderbook/price_level.cpp
    src/feed_handlers/coinbase_handler.cpp
    src/network/websocket_client.cpp
    src/visualization/console_visualizer.cpp
)

# Link dependencies
target_link_libraries(clunk PRIVATE
    Boost::system
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Add tests subdirectory
add_subdirectory(tests)

# Add benchmarks subdirectory
add_subdirectory(benchmarks)

# Installation rules
install(TARGETS clunk DESTINATION bin)