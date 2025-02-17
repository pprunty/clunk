#include "clunk/clunk.hpp"
#include <iostream>

// 1. Include the nlohmann/json header
#include <nlohmann/json.hpp>

namespace clunk {

ClunkApp::ClunkApp() {
    // Constructor
}

ClunkApp::~ClunkApp() {
    // Destructor
}

void ClunkApp::run() {
    // Simple log to show the app is running
    std::cout << "[clunk] Running the app..." << std::endl;

    // 2. Create a JSON object
    nlohmann::json j;
    j["app"]       = "clunk";
    j["version"]   = "0.1.0";
    j["author"]    = "you";
    j["features"]  = { "market_data", "order_book_management", "benchmark_tools" };

    // 3. Dump it as a string to log
    std::cout << "[clunk] JSON payload: " << j.dump(4) << std::endl;
}

} // namespace clunk
