#include "feed_handlers/coinbase_handler.h"
#include "visualization/console_visualizer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>

// Global flag for handling Ctrl+C
std::atomic<bool> running(true);

// ANSI color codes for terminal output
namespace Color {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    const std::string BOLD = "\033[1m";
}

// Signal handler for Ctrl+C
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << Color::YELLOW << "\nShutting down..." << Color::RESET << std::endl;
        running = false;
    }
}

// Print the program banner
void printBanner() {
    std::cout << Color::CYAN << Color::BOLD;
    std::cout << "   ____  _     _   _ _   _ _  __" << std::endl;
    std::cout << "  / ___|| |   | | | | \\ | | |/ /" << std::endl;
    std::cout << " | |    | |   | | | |  \\| | ' / " << std::endl;
    std::cout << " | |___ | |___| |_| | |\\  | . \\ " << std::endl;
    std::cout << "  \\____||_____|\\___/|_| \\_|_|\\_\\" << std::endl;
    std::cout << Color::RESET << std::endl;
    
    std::cout << "C++ Low-Latency Unified Networked Orderbook Kit" << std::endl;
    std::cout << "Version 0.1.0" << std::endl;
    std::cout << std::endl;
}

// Print usage information
void printUsage(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help                 Display this help message" << std::endl;
    std::cout << "  -s, --symbol SYMBOL        Trading symbol (default: BTC-USD)" << std::endl;
    std::cout << "  -d, --depth DEPTH          Order book depth to display (default: 10)" << std::endl;
    std::cout << "  -r, --refresh RATE         Refresh rate in milliseconds (default: 500)" << std::endl;
    std::cout << "  -v, --verbose              Enable verbose output" << std::endl;
    std::cout << "  -c, --no-color-changes     Disable highlighting of price/size changes" << std::endl;
    std::cout << "  -t, --highlight-time TIME  Duration to highlight changes (default: 2 refreshes)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " -s ETH-USD" << std::endl;
    std::cout << "  " << program_name << " --symbol BTC-USD --depth 15 --refresh 1000" << std::endl;
    std::cout << "  " << program_name << " --symbol BTC-USD --no-color-changes" << std::endl;
    std::cout << std::endl;
}

// Parse command line arguments
struct ProgramOptions {
    std::string symbol = "BTC-USD";
    size_t depth = 10;
    int refresh_rate = 500;
    bool verbose = false;
    bool show_help = false;
    bool highlight_changes = true;
    int highlight_duration = 2;
};

ProgramOptions parseCommandLine(int argc, char* argv[]) {
    ProgramOptions options;
    
    std::vector<std::string> args(argv + 1, argv + argc);
    
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        
        if (arg == "-h" || arg == "--help") {
            options.show_help = true;
        } else if (arg == "-s" || arg == "--symbol") {
            if (i + 1 < args.size()) {
                options.symbol = args[++i];
            }
        } else if (arg == "-d" || arg == "--depth") {
            if (i + 1 < args.size()) {
                try {
                    options.depth = std::stoi(args[++i]);
                } catch (const std::exception& e) {
                    std::cerr << "Invalid depth value: " << args[i] << std::endl;
                }
            }
        } else if (arg == "-r" || arg == "--refresh") {
            if (i + 1 < args.size()) {
                try {
                    options.refresh_rate = std::stoi(args[++i]);
                } catch (const std::exception& e) {
                    std::cerr << "Invalid refresh rate value: " << args[i] << std::endl;
                }
            }
        } else if (arg == "-v" || arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "-c" || arg == "--no-color-changes") {
            options.highlight_changes = false;
        } else if (arg == "-t" || arg == "--highlight-time") {
            if (i + 1 < args.size()) {
                try {
                    options.highlight_duration = std::stoi(args[++i]);
                } catch (const std::exception& e) {
                    std::cerr << "Invalid highlight duration value: " << args[i] << std::endl;
                }
            }
        }
    }
    
    return options;
}

// Main function
int main(int argc, char* argv[]) {
    // Set up signal handling
    std::signal(SIGINT, signal_handler);
    
    // Print banner
    printBanner();
    
    // Parse command line arguments
    ProgramOptions options = parseCommandLine(argc, argv);
    
    // If help flag is set, print usage and exit
    if (options.show_help) {
        printUsage(argv[0]);
        return 0;
    }

    std::cout << "Starting with symbol: " << Color::YELLOW << options.symbol << Color::RESET << std::endl;
    std::cout << "Order book depth: " << options.depth << std::endl;
    std::cout << "Refresh rate: " << options.refresh_rate << " ms" << std::endl;
    std::cout << "Change highlighting: " << (options.highlight_changes ? "enabled" : "disabled") << std::endl;
    if (options.highlight_changes) {
        std::cout << "Highlight duration: " << options.highlight_duration << " refreshes" << std::endl;
    }
    std::cout << "Using Coinbase Exchange for market data" << std::endl;
    std::cout << std::endl;

    try {
        // Create the Coinbase handler
        clunk::CoinbaseHandler handler;
        
        // Set verbose logging based on command line option
        handler.setVerboseLogging(options.verbose);
        
        if (options.verbose) {
            std::cout << "Created Coinbase handler with verbose logging enabled" << std::endl;
        }

        // Connect to Coinbase
        std::cout << "Connecting to Coinbase..." << std::endl;
        handler.connect();

        // Wait for connection to establish
        std::cout << "Waiting for connection to establish..." << std::endl;

        // Try for up to 15 seconds (30 attempts * 500ms)
        int attempts = 0;
        const int max_attempts = 30;
        bool connection_successful = false;

        while (!handler.isConnected() && attempts < max_attempts) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            attempts++;

            if (attempts % 5 == 0) {
                std::cout << "Connection attempt " << attempts << "/" << max_attempts << "..." << std::endl;
            }
            
            if (handler.isConnected()) {
                connection_successful = true;
                break;
            }
        }

        if (!connection_successful) {
            std::cerr << Color::RED << "Failed to connect to Coinbase after " << max_attempts << " attempts" << Color::RESET << std::endl;
            std::cerr << "Please check your internet connection and verify that the Coinbase service is available." << std::endl;
            std::cerr << "You may also need to check if your network allows WebSocket connections to port 443." << std::endl;
            return 1;
        }

        std::cout << Color::GREEN << "Connected to Coinbase" << Color::RESET << std::endl;

        // Subscribe to the symbol
        std::cout << "Subscribing to " << Color::YELLOW << options.symbol << Color::RESET << "..." << std::endl;
        handler.subscribe(options.symbol);

        // Wait for initial data
        std::cout << "Waiting for initial data (this may take a moment)..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Get the order book
        auto order_book = handler.getOrderBook(options.symbol);
        if (!order_book) {
            std::cerr << Color::RED << "No order book found for " << options.symbol << Color::RESET << std::endl;
            std::cerr << "This symbol may not be available in the Coinbase API." << std::endl;
            std::cerr << "Try a different symbol like ETH-BTC or BTC-USD." << std::endl;
            handler.disconnect();
            return 1;
        }

        // Create the console visualizer
        clunk::ConsoleVisualizer visualizer(order_book);
        
        // Apply user options to visualizer
        visualizer.setDepth(options.depth);
        visualizer.setChangeHighlighting(options.highlight_changes);
        visualizer.setChangeHighlightDuration(options.highlight_duration);
        
        // Start visualization
        visualizer.start(options.refresh_rate);

        // Main loop - keep running until Ctrl+C
        std::cout << "Press " << Color::BOLD << "Ctrl+C" << Color::RESET << " to exit" << std::endl;
        
        // Track execution time for performance metrics
        auto start_time = std::chrono::high_resolution_clock::now();
        
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Check if we're still connected
            if (!handler.isConnected()) {
                std::cout << Color::YELLOW << "Connection to Coinbase lost, attempting to reconnect..." << Color::RESET << std::endl;

                // Try to reconnect
                handler.connect();

                // Wait a bit for the connection to establish
                std::this_thread::sleep_for(std::chrono::seconds(2));

                if (handler.isConnected()) {
                    std::cout << Color::GREEN << "Reconnected to Coinbase, resubscribing to " << options.symbol << "..." << Color::RESET << std::endl;
                    handler.subscribe(options.symbol);
                }
            }
        }

        // Calculate execution time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
        
        // Clean up
        std::cout << "Unsubscribing from " << options.symbol << "..." << std::endl;
        handler.unsubscribe(options.symbol);

        std::cout << "Stopping visualization..." << std::endl;
        visualizer.stop();

        std::cout << "Disconnecting from Coinbase..." << std::endl;
        handler.disconnect();

        std::cout << Color::GREEN << "Shutdown complete" << Color::RESET << std::endl;
        std::cout << "Session duration: " << duration << " seconds" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << Color::RED << "Error: " << e.what() << Color::RESET << std::endl;
        return 1;
    }

    return 0;
}