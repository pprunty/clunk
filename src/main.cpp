#include "feed_handlers/coinbase_handler.h"
#include "visualization/console_visualizer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>

// Global flag for handling Ctrl+C
std::atomic<bool> running(true);

// Signal handler for Ctrl+C
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nShutting down..." << std::endl;
        running = false;
    }
}

int main(int argc, char* argv[]) {
    // Set up signal handling
    std::signal(SIGINT, signal_handler);

    // Parse command line arguments
    std::string symbol = "BTC-USD";  // Default symbol
    if (argc > 1) {
        symbol = argv[1];
    }

    std::cout << "Clunk - C++ Low-Latency Unified Networked Orderbook Kit" << std::endl;
    std::cout << "Starting with symbol: " << symbol << std::endl;

    try {
        // Create the Coinbase handler
        clunk::CoinbaseHandler handler;

        // Connect to Coinbase
        std::cout << "Connecting to Coinbase..." << std::endl;
        handler.connect();

        // Wait for connection to establish
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (!handler.isConnected()) {
            std::cerr << "Failed to connect to Coinbase" << std::endl;
            return 1;
        }

        std::cout << "Connected to Coinbase" << std::endl;

        // Subscribe to the symbol
        std::cout << "Subscribing to " << symbol << "..." << std::endl;
        handler.subscribe(symbol);

        // Wait for initial data
        std::cout << "Waiting for initial data..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Get the order book
        auto order_book = handler.getOrderBook(symbol);
        if (!order_book) {
            std::cerr << "Failed to get order book for " << symbol << std::endl;
            return 1;
        }

        // Create the console visualizer
        clunk::ConsoleVisualizer visualizer(order_book);

        // Start visualization
        visualizer.start(500);  // Refresh every 500ms

        // Main loop - keep running until Ctrl+C
        std::cout << "Press Ctrl+C to exit" << std::endl;
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Clean up
        std::cout << "Unsubscribing from " << symbol << "..." << std::endl;
        handler.unsubscribe(symbol);

        std::cout << "Stopping visualization..." << std::endl;
        visualizer.stop();

        std::cout << "Disconnecting from Coinbase..." << std::endl;
        handler.disconnect();

        std::cout << "Shutdown complete" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}