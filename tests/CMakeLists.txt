cmake_minimum_required(VERSION 3.15)

# Test executable
add_executable(clunk_tests
    orderbook_tests.cpp
)

# Link dependencies
target_link_libraries(clunk_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    Boost::system
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Add source files to include
target_sources(clunk_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/orderbook/order_book.cpp
    ${CMAKE_SOURCE_DIR}/src/orderbook/order.cpp
    ${CMAKE_SOURCE_DIR}/src/orderbook/price_level.cpp
)

# Include source directory
target_include_directories(clunk_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Register tests
include(GoogleTest)
gtest_discover_tests(clunk_tests)