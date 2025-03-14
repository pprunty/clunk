#pragma once

#include "../orderbook/order_book.h"
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <sstream>
#include <map>
#include <unordered_map>
#include <deque>
#include <chrono>

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
    
    // Enable/disable highlighting of changes
    void setChangeHighlighting(bool enabled) {
        highlight_changes_ = enabled;
    }
    
    // Set how long changes should be highlighted (in refreshes)
    void setChangeHighlightDuration(int duration) {
        change_highlight_duration_ = duration;
    }

private:
    std::shared_ptr<OrderBook> order_book_;
    std::atomic<bool> running_;
    std::thread viz_thread_;
    size_t depth_;
    std::function<void()> refresh_callback_;
    int refresh_rate_ms_ = 500;  // Store refresh rate for display
    
    // Options for change highlighting
    bool highlight_changes_ = true;
    int change_highlight_duration_ = 2;  // Number of refreshes to highlight changes
    
    // Data structures to track previous state for change highlighting
    std::map<double, double> prev_bids_;  // Previous bid price -> size
    std::map<double, double> prev_asks_;  // Previous ask price -> size
    double prev_best_bid_ = 0.0;
    double prev_best_ask_ = 0.0;
    
    // HFT metrics
    double order_book_imbalance_ = 0.0;     // Ratio of bid volume to ask volume
    double vwap_bid_ = 0.0;                 // Volume-weighted average price for bids
    double vwap_ask_ = 0.0;                 // Volume-weighted average price for asks
    double market_pressure_ = 0.0;          // Measure of buying vs. selling pressure (-1.0 to 1.0)
    double price_impact_1pct_ = 0.0;        // Estimated price impact of 1% of daily volume
    double bid_liquidity_depth_ = 0.0;      // Cumulative liquidity within 0.5% of best bid
    double ask_liquidity_depth_ = 0.0;      // Cumulative liquidity within 0.5% of best ask
    double spread_bps_ = 0.0;               // Spread in basis points
    
    // Update rate tracking
    int updates_since_last_refresh_ = 0;
    std::deque<int> update_rates_;          // Stores update rates for moving average
    double avg_update_rate_ = 0.0;          // Average updates per second
    
    // Latency tracking
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    std::deque<double> processing_latencies_ms_;  // Stores recent processing latencies in ms
    double avg_processing_latency_ms_ = 0.0;      // Average processing latency in ms
    TimePoint last_update_time_;                  // Time of last update
    
    // Track how long we've been highlighting each price level
    std::unordered_map<double, int> bid_highlight_timers_;
    std::unordered_map<double, int> ask_highlight_timers_;
    
    // For tracking new, updated, deleted price levels
    enum class PriceChangeType {
        NO_CHANGE,
        NEW_PRICE,
        INCREASED_SIZE,
        DECREASED_SIZE,
        DELETED_PRICE
    };
    
    // Get the type of change for a price level
    PriceChangeType getPriceChangeType(double price, double size, const std::map<double, double>& prev_levels);
    
    // Determine the appropriate color for a price or size based on its change
    std::string getPriceColorCode(double price, PriceChangeType change_type);
    std::string getSizeColorCode(double price, PriceChangeType change_type);
    
    // Update highlight timers and previous state
    void updateHighlightTimers();
    void updatePreviousState(const std::vector<std::pair<double, double>>& bids,
                            const std::vector<std::pair<double, double>>& asks);
                            
    // Calculate HFT metrics
    void calculateHFTMetrics(const std::vector<std::pair<double, double>>& bids,
                            const std::vector<std::pair<double, double>>& asks);
    
    // Update latency and update rate metrics
    void updatePerformanceMetrics();

    // Render the order book
    void render();
    
    // Render HFT metrics section
    void renderHFTMetrics(std::stringstream& output);
    
    // Helper methods for formatting
    std::string formatPrice(double price, PriceChangeType change_type);
    std::string formatSize(double size, PriceChangeType change_type);
    std::string formatPercentage(double value, bool include_sign = false);
    std::string formatLatency(double latency_ms);
    void renderProgressBar(std::stringstream& ss, double value, double max_value, int width, bool is_bid);
};

} // namespace clunk