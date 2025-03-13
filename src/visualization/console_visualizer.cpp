#include "console_visualizer.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>

namespace clunk {

ConsoleVisualizer::ConsoleVisualizer(std::shared_ptr<OrderBook> order_book)
    : order_book_(order_book), running_(false), depth_(10) {
}

ConsoleVisualizer::~ConsoleVisualizer() {
    stop();
}

void ConsoleVisualizer::start(int refresh_rate_ms) {
    if (running_) {
        return;
    }

    running_ = true;

    // Start the visualization thread
    viz_thread_ = std::thread([this, refresh_rate_ms]() {
        while (running_) {
            render();

            // Call the refresh callback if set
            if (refresh_callback_) {
                refresh_callback_();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(refresh_rate_ms));
        }
    });
}

void ConsoleVisualizer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (viz_thread_.joinable()) {
        viz_thread_.join();
    }
}

void ConsoleVisualizer::render() {
    if (!order_book_) {
        return;
    }

    // Clear the screen
    #ifdef _WIN32
    system("cls");
    #else
    system("clear");
    #endif

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::string time_str = std::ctime(&time);
    time_str.pop_back();  // Remove trailing newline

    // Print header
    std::cout << "===================================================================" << std::endl;
    std::cout << "Order Book: " << order_book_->getSymbol() << " - " << time_str << std::endl;
    std::cout << "===================================================================" << std::endl;

    // Get order book levels
    auto bids = order_book_->getBidLevels(depth_);
    auto asks = order_book_->getAskLevels(depth_);

    // Print order book statistics
    double best_bid = order_book_->getBestBid();
    double best_ask = order_book_->getBestAsk();
    double spread = order_book_->getSpread();
    double midpoint = order_book_->getMidpointPrice();

    std::cout << "Best Bid: " << std::fixed << std::setprecision(2) << best_bid;
    std::cout << " | Best Ask: " << std::fixed << std::setprecision(2) << best_ask;
    std::cout << " | Spread: " << std::fixed << std::setprecision(2) << spread;
    std::cout << " | Midpoint: " << std::fixed << std::setprecision(2) << midpoint << std::endl;

    std::cout << "Orders: " << order_book_->getOrderCount();
    std::cout << " | Bid Levels: " << order_book_->getBidLevelCount();
    std::cout << " | Ask Levels: " << order_book_->getAskLevelCount() << std::endl;
    std::cout << "===================================================================" << std::endl;

    // Calculate max size for bid and ask to scale bar widths
    double max_bid_size = 0.0;
    for (const auto& [price, size] : bids) {
        max_bid_size = std::max(max_bid_size, size);
    }

    double max_ask_size = 0.0;
    for (const auto& [price, size] : asks) {
        max_ask_size = std::max(max_ask_size, size);
    }

    double max_size = std::max(max_bid_size, max_ask_size);
    if (max_size <= 0.0) {
        max_size = 1.0;  // Avoid division by zero
    }

    // Print table header
    std::cout << std::setw(10) << "Bid Size" << " | ";
    std::cout << std::setw(10) << "Bid Price" << " | ";
    std::cout << std::setw(10) << "Ask Price" << " | ";
    std::cout << std::setw(10) << "Ask Size" << std::endl;
    std::cout << "-------------------------------------------------------------------" << std::endl;

    // Print bid and ask levels
    for (size_t i = 0; i < depth_; ++i) {
        // Print bid side
        if (i < bids.size()) {
            double bid_price = bids[i].first;
            double bid_size = bids[i].second;
            int bid_bar_width = static_cast<int>((bid_size / max_size) * 20);

            std::cout << std::setw(10) << std::fixed << std::setprecision(2) << bid_size << " | ";
            std::cout << std::setw(10) << std::fixed << std::setprecision(2) << bid_price << " | ";
        } else {
            std::cout << std::setw(10) << " " << " | ";
            std::cout << std::setw(10) << " " << " | ";
        }

        // Print ask side
        if (i < asks.size()) {
            double ask_price = asks[i].first;
            double ask_size = asks[i].second;
            int ask_bar_width = static_cast<int>((ask_size / max_size) * 20);

            std::cout << std::setw(10) << std::fixed << std::setprecision(2) << ask_price << " | ";
            std::cout << std::setw(10) << std::fixed << std::setprecision(2) << ask_size;
        } else {
            std::cout << std::setw(10) << " " << " | ";
            std::cout << std::setw(10) << " ";
        }

        std::cout << std::endl;
    }

    std::cout << "===================================================================" << std::endl;
}

} // namespace clunk