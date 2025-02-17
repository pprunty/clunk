cmake_minimum_required(VERSION 3.14)
project(clunk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Important: point to the "include/" folder
#
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# If you prefer more modern usage, do this instead:
add_executable(clunk src/main.cpp src/clunk.cpp)
target_include_directories(clunk PRIVATE
     "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_definitions(clunk PRIVATE
    USE_JEMALLOC=1
)

# (Optional) find_package calls if you’re using nlohmann_json, etc.
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(clunk PRIVATE nlohmann_json::nlohmann_json)