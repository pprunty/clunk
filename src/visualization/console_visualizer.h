#pragma once

#include "../orderbook/order_book.h"
#include <memory>
#include <thread>
#include <atomic>
#include <functional>

namespace clunk {

// Console-based order book visualizer
class ConsoleVisualizer {
public:
    // Constructor
    explicit ConsoleVisualizer(std::shared_ptr<OrderBook> order_book);

    // Destructor
    ~ConsoleVisualizer();

    // Start visualization
    void start(int refresh_rate_ms = 500);

    // Stop visualization
    void stop();

    // Set depth to display
    void setDepth(size_t depth) {
        depth_ = depth;
    }

    // Set refresh callback
    void setRefreshCallback(std::function<void()> callback) {
        refresh_callback_ = callback;
    }

private:
    std::shared_ptr<OrderBook> order_book_;
    std::atomic<bool> running_;
    std::thread viz_thread_;
    size_t depth_;
    std::function<void()> refresh_callback_;

    // Render the order book
    void render();
};

} // namespace clunk