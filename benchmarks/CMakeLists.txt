cmake_minimum_required(VERSION 3.15)

# Benchmark executable
add_executable(clunk_benchmarks
    orderbook_benchmarks.cpp
)

# Link dependencies
target_link_libraries(clunk_benchmarks PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    Boost::system
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Add source files to include
target_sources(clunk_benchmarks PRIVATE
    ${CMAKE_SOURCE_DIR}/src/orderbook/order_book.cpp
    ${CMAKE_SOURCE_DIR}/src/orderbook/order.cpp
    ${CMAKE_SOURCE_DIR}/src/orderbook/price_level.cpp
)

# Include source directory
target_include_directories(clunk_benchmarks PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Installation rules
install(TARGETS clunk_benchmarks DESTINATION bin)