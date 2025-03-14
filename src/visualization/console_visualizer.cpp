#include "console_visualizer.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <algorithm>
#include <sstream>

namespace clunk {

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
    
    // Bright versions for highlighting changes
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_GREEN = "\033[92m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_BLUE = "\033[94m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN = "\033[96m";
    const std::string BRIGHT_WHITE = "\033[97m";
    
    // Background colors for more intense highlighting
    const std::string BG_RED = "\033[41m";
    const std::string BG_GREEN = "\033[42m";
}

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
    refresh_rate_ms_ = refresh_rate_ms;

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

ConsoleVisualizer::PriceChangeType ConsoleVisualizer::getPriceChangeType(
    double price, double size, const std::map<double, double>& prev_levels) {
    
    // If we're not highlighting changes, always return NO_CHANGE
    if (!highlight_changes_) {
        return PriceChangeType::NO_CHANGE;
    }
    
    auto it = prev_levels.find(price);
    
    // If price level didn't exist before
    if (it == prev_levels.end()) {
        return PriceChangeType::NEW_PRICE;
    }
    
    // Price level existed, check if size changed
    if (size > it->second) {
        return PriceChangeType::INCREASED_SIZE;
    } else if (size < it->second) {
        return PriceChangeType::DECREASED_SIZE;
    }
    
    // Size didn't change
    return PriceChangeType::NO_CHANGE;
}

std::string ConsoleVisualizer::getPriceColorCode(double price, PriceChangeType change_type) {
    switch (change_type) {
        case PriceChangeType::NEW_PRICE:
            return Color::BRIGHT_YELLOW;
        case PriceChangeType::INCREASED_SIZE:
            return Color::BRIGHT_GREEN;
        case PriceChangeType::DECREASED_SIZE:
            return Color::BRIGHT_RED;
        case PriceChangeType::DELETED_PRICE:
            return Color::BRIGHT_MAGENTA;
        case PriceChangeType::NO_CHANGE:
        default:
            return "";  // No special color
    }
}

std::string ConsoleVisualizer::getSizeColorCode(double price, PriceChangeType change_type) {
    switch (change_type) {
        case PriceChangeType::NEW_PRICE:
            return Color::BRIGHT_YELLOW;
        case PriceChangeType::INCREASED_SIZE:
            return Color::BRIGHT_GREEN;
        case PriceChangeType::DECREASED_SIZE:
            return Color::BRIGHT_RED;
        case PriceChangeType::DELETED_PRICE:
            return Color::BRIGHT_MAGENTA;
        case PriceChangeType::NO_CHANGE:
        default:
            return "";  // No special color
    }
}

void ConsoleVisualizer::updateHighlightTimers() {
    // Update all bid highlight timers
    for (auto it = bid_highlight_timers_.begin(); it != bid_highlight_timers_.end();) {
        it->second--;
        if (it->second <= 0) {
            it = bid_highlight_timers_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update all ask highlight timers
    for (auto it = ask_highlight_timers_.begin(); it != ask_highlight_timers_.end();) {
        it->second--;
        if (it->second <= 0) {
            it = ask_highlight_timers_.erase(it);
        } else {
            ++it;
        }
    }
}

void ConsoleVisualizer::updatePreviousState(
    const std::vector<std::pair<double, double>>& bids,
    const std::vector<std::pair<double, double>>& asks) {
    
    // Update previous bid levels
    prev_bids_.clear();
    for (const auto& [price, size] : bids) {
        prev_bids_[price] = size;
    }
    
    // Update previous ask levels
    prev_asks_.clear();
    for (const auto& [price, size] : asks) {
        prev_asks_[price] = size;
    }
    
    // Update previous best bid and ask
    prev_best_bid_ = bids.empty() ? 0.0 : bids[0].first;
    prev_best_ask_ = asks.empty() ? 0.0 : asks[0].first;
}

std::string ConsoleVisualizer::formatPrice(double price, PriceChangeType change_type) {
    std::stringstream ss;
    
    // Apply change highlighting if needed
    std::string color_code = getPriceColorCode(price, change_type);
    if (!color_code.empty()) {
        ss << color_code;
    }
    
    ss << std::fixed << std::setprecision(2) << price;
    
    // Reset color if we applied one
    if (!color_code.empty()) {
        ss << Color::RESET;
    }
    
    return ss.str();
}

std::string ConsoleVisualizer::formatSize(double size, PriceChangeType change_type) {
    std::stringstream ss;
    
    // Apply change highlighting if needed
    std::string color_code = getSizeColorCode(size, change_type);
    if (!color_code.empty()) {
        ss << color_code;
    }
    
    // Format the size
    if (size >= 10000) {
        ss << std::fixed << std::setprecision(1) << (size / 1000) << "K";
    } else if (size >= 1000) {
        ss << std::fixed << std::setprecision(2) << (size / 1000) << "K";
    } else if (size >= 100) {
        ss << std::fixed << std::setprecision(1) << size;
    } else {
        ss << std::fixed << std::setprecision(size < 1 ? 5 : 2) << size;
    }
    
    // Reset color if we applied one
    if (!color_code.empty()) {
        ss << Color::RESET;
    }
    
    return ss.str();
}

void ConsoleVisualizer::renderProgressBar(std::stringstream& ss, double value, double max_value, 
                                        int width, bool is_bid) {
    int filled_width = static_cast<int>((value / max_value) * width);
    filled_width = std::min(filled_width, width);
    
    if (is_bid) {
        ss << Color::GREEN;
        for (int i = 0; i < filled_width; ++i) {
            ss << "█";
        }
        ss << Color::RESET;
    } else {
        ss << Color::RED;
        for (int i = 0; i < filled_width; ++i) {
            ss << "█";
        }
        ss << Color::RESET;
    }
}

void ConsoleVisualizer::calculateHFTMetrics(
    const std::vector<std::pair<double, double>>& bids,
    const std::vector<std::pair<double, double>>& asks) {
    
    if (bids.empty() || asks.empty()) {
        return;
    }
    
    // Get best bid and ask prices
    double best_bid = bids[0].first;
    double best_ask = asks[0].first;
    double mid_price = (best_bid + best_ask) / 2.0;
    
    // Calculate spread in basis points (1 bp = 0.01%)
    spread_bps_ = ((best_ask - best_bid) / mid_price) * 10000.0;
    
    // Calculate bid and ask totals
    double total_bid_volume = 0.0;
    double total_ask_volume = 0.0;
    double total_bid_value = 0.0;  // price * volume
    double total_ask_value = 0.0;  // price * volume
    
    // Calculate bid and ask liquidity depth (within 0.5% of best levels)
    double bid_boundary = best_bid * 0.995;  // 0.5% below best bid
    double ask_boundary = best_ask * 1.005;  // 0.5% above best ask
    bid_liquidity_depth_ = 0.0;
    ask_liquidity_depth_ = 0.0;
    
    // Process bid side
    for (const auto& [price, size] : bids) {
        total_bid_volume += size;
        total_bid_value += price * size;
        
        // Add to liquidity depth if within boundary
        if (price >= bid_boundary) {
            bid_liquidity_depth_ += size;
        }
    }
    
    // Process ask side
    for (const auto& [price, size] : asks) {
        total_ask_volume += size;
        total_ask_value += price * size;
        
        // Add to liquidity depth if within boundary
        if (price <= ask_boundary) {
            ask_liquidity_depth_ += size;
        }
    }
    
    // Calculate order book imbalance (ratio of bid to ask volume)
    if (total_ask_volume > 0) {
        order_book_imbalance_ = total_bid_volume / total_ask_volume;
    } else {
        order_book_imbalance_ = 1.0;  // Neutral if no asks
    }
    
    // Calculate market pressure (-1.0 to 1.0, where positive means buy pressure)
    // A simple formula based on order book imbalance
    market_pressure_ = (order_book_imbalance_ - 1.0) / (order_book_imbalance_ + 1.0);
    
    // Calculate Volume-Weighted Average Price (VWAP)
    if (total_bid_volume > 0) {
        vwap_bid_ = total_bid_value / total_bid_volume;
    }
    
    if (total_ask_volume > 0) {
        vwap_ask_ = total_ask_value / total_ask_volume;
    }
    
    // Simple price impact estimation (very simplified model)
    // Estimate how much a 1% market order would move the price
    double market_order_size = (total_bid_volume + total_ask_volume) * 0.01;
    double cumulative_volume = 0.0;
    
    // For a buy market order (walks up the ask side)
    double impact_price = best_ask;
    for (const auto& [price, size] : asks) {
        cumulative_volume += size;
        impact_price = price;
        if (cumulative_volume >= market_order_size) {
            break;
        }
    }
    
    // Price impact in percentage
    price_impact_1pct_ = ((impact_price - best_ask) / best_ask) * 100.0;
}

void ConsoleVisualizer::updatePerformanceMetrics() {
    // Calculate time since last update
    auto current_time = std::chrono::high_resolution_clock::now();
    
    if (last_update_time_.time_since_epoch().count() > 0) {
        // Calculate processing latency in milliseconds
        double latency_ms = std::chrono::duration<double, std::milli>(
            current_time - last_update_time_).count();
            
        // Add to the latency queue (keep last 20 measurements)
        processing_latencies_ms_.push_back(latency_ms);
        if (processing_latencies_ms_.size() > 20) {
            processing_latencies_ms_.pop_front();
        }
        
        // Calculate average latency
        if (!processing_latencies_ms_.empty()) {
            double total_latency = 0.0;
            for (double lat : processing_latencies_ms_) {
                total_latency += lat;
            }
            avg_processing_latency_ms_ = total_latency / processing_latencies_ms_.size();
        }
    }
    
    // Update last update time
    last_update_time_ = current_time;
    
    // Calculate update rate (per second)
    updates_since_last_refresh_++;
    
    // Every second, record the update rate
    static auto last_rate_update = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_rate_update).count();
    
    if (duration >= 1000) {  // 1 second has passed
        double updates_per_second = (updates_since_last_refresh_ * 1000.0) / duration;
        
        // Add to the update rate queue (keep last 5 measurements)
        update_rates_.push_back(static_cast<int>(updates_per_second));
        if (update_rates_.size() > 5) {
            update_rates_.pop_front();
        }
        
        // Calculate average update rate
        if (!update_rates_.empty()) {
            int total_rate = 0;
            for (int rate : update_rates_) {
                total_rate += rate;
            }
            avg_update_rate_ = static_cast<double>(total_rate) / update_rates_.size();
        }
        
        // Reset counters
        updates_since_last_refresh_ = 0;
        last_rate_update = now;
    }
}

std::string ConsoleVisualizer::formatPercentage(double value, bool include_sign) {
    std::stringstream ss;
    
    if (include_sign && value > 0) {
        ss << "+";
    }
    
    ss << std::fixed << std::setprecision(2) << value << "%";
    return ss.str();
}

std::string ConsoleVisualizer::formatLatency(double latency_ms) {
    std::stringstream ss;
    
    if (latency_ms < 1.0) {
        // Show in microseconds
        ss << std::fixed << std::setprecision(1) << (latency_ms * 1000.0) << " μs";
    } else if (latency_ms < 1000.0) {
        // Show in milliseconds
        ss << std::fixed << std::setprecision(2) << latency_ms << " ms";
    } else {
        // Show in seconds
        ss << std::fixed << std::setprecision(3) << (latency_ms / 1000.0) << " s";
    }
    
    return ss.str();
}

void ConsoleVisualizer::renderHFTMetrics(std::stringstream& output) {
    output << "───────────────────────────────────────────────────────────────────────────\n";
    output << Color::BOLD << "HFT Metrics:\n" << Color::RESET;
    
    // First row: Imbalance, Spread, Market Pressure
    output << "Book Imbalance: ";
    if (order_book_imbalance_ > 1.05) {
        output << Color::GREEN;
    } else if (order_book_imbalance_ < 0.95) {
        output << Color::RED;
    }
    output << std::fixed << std::setprecision(2) << order_book_imbalance_ << "x" << Color::RESET;
    
    output << " | Spread: " << std::fixed << std::setprecision(1) << spread_bps_ << " bps";
    
    output << " | Market Pressure: ";
    if (market_pressure_ > 0.05) {
        output << Color::GREEN;
    } else if (market_pressure_ < -0.05) {
        output << Color::RED;
    }
    output << std::fixed << std::setprecision(2) << market_pressure_ << Color::RESET;
    
    output << " | Est. 1% Impact: " << formatPercentage(price_impact_1pct_) << "\n";
    
    // Second row: VWAP, Liquidity Depth
    output << "VWAP (Bid/Ask): " << std::fixed << std::setprecision(2) << vwap_bid_ 
           << " / " << vwap_ask_;
    
    output << " | Liquidity Depth (0.5%): " << std::fixed << std::setprecision(2) 
           << bid_liquidity_depth_ << " / " << ask_liquidity_depth_;
    
    // Performance metrics
    output << " | Updates: " << std::fixed << std::setprecision(1) << avg_update_rate_ << "/s";
    
    output << " | Latency: ";
    if (avg_processing_latency_ms_ < 1.0) {
        output << Color::GREEN;
    } else if (avg_processing_latency_ms_ > 10.0) {
        output << Color::RED;
    }
    output << formatLatency(avg_processing_latency_ms_) << Color::RESET << "\n";
}

void ConsoleVisualizer::render() {
    if (!order_book_) {
        return;
    }

    // Update performance metrics
    updatePerformanceMetrics();

    // Instead of clearing the screen, move cursor to the beginning and update in-place
    // Store the current cursor position so we can restore it at the end
    std::cout << "\033[s"; // Save cursor position
    std::cout << "\033[H"; // Move to top-left corner
    
    std::stringstream output;

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::string time_str = std::ctime(&time);
    time_str.pop_back();  // Remove trailing newline

    // Print header with project name and symbol
    output << Color::CYAN << Color::BOLD
           << "┌─────────────────────────────────────────────────────────────────────────┐\n"
           << "│                   CLUNK - Order Book Visualization                      │\n"
           << "└─────────────────────────────────────────────────────────────────────────┘"
           << Color::RESET << "\n";

    output << "Symbol: " << Color::BOLD << Color::YELLOW << order_book_->getSymbol() 
           << Color::RESET << " | Time: " << time_str << "\n";

    // Get order book levels
    auto bids = order_book_->getBidLevels(depth_);
    auto asks = order_book_->getAskLevels(depth_);

    // Calculate HFT metrics
    calculateHFTMetrics(bids, asks);

    // Print order book statistics
    double best_bid = order_book_->getBestBid();
    double best_ask = order_book_->getBestAsk();
    double spread = order_book_->getSpread();
    double spread_percent = (best_ask > 0) ? (spread / best_ask) * 100.0 : 0.0;
    double midpoint = order_book_->getMidpointPrice();

    // Determine if best bid/ask changed
    std::string best_bid_color = Color::GREEN;
    std::string best_ask_color = Color::RED;
    
    if (highlight_changes_) {
        if (best_bid > prev_best_bid_) {
            best_bid_color = Color::BRIGHT_GREEN;
        } else if (best_bid < prev_best_bid_) {
            best_bid_color = Color::BRIGHT_RED;
        }
        
        if (best_ask > prev_best_ask_) {
            best_ask_color = Color::BRIGHT_RED;
        } else if (best_ask < prev_best_ask_) {
            best_ask_color = Color::BRIGHT_GREEN;
        }
    }

    output << "───────────────────────────────────────────────────────────────────────────\n";
    output << "Market Summary:\n";
    output << "Best Bid: " << best_bid_color << std::fixed << std::setprecision(2) << best_bid << Color::RESET;
    output << " | Best Ask: " << best_ask_color << std::fixed << std::setprecision(2) << best_ask << Color::RESET;
    output << " | Spread: " << std::fixed << std::setprecision(2) << spread << " (" << std::fixed << std::setprecision(3) << spread_percent << "%)";
    output << " | Midpoint: " << Color::CYAN << std::fixed << std::setprecision(2) << midpoint << Color::RESET << "\n";

    output << "Orders: " << order_book_->getOrderCount();
    output << " | Bid Levels: " << order_book_->getBidLevelCount();
    output << " | Ask Levels: " << order_book_->getAskLevelCount() << "\n";
    
    // Render HFT metrics section
    renderHFTMetrics(output);
    
    output << "───────────────────────────────────────────────────────────────────────────\n";

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

    // Calculate depth statistics
    double bid_size_total = 0.0;
    double ask_size_total = 0.0;
    
    for (const auto& [price, size] : bids) {
        bid_size_total += size;
    }
    
    for (const auto& [price, size] : asks) {
        ask_size_total += size;
    }

    // Print table header with improved formatting
    output << Color::BOLD 
           << std::left << std::setw(12) << "BIDS" 
           << std::setw(20) << " " 
           << "│ " 
           << std::setw(12) << "ASKS" 
           << Color::RESET << "\n";
           
    output << std::left << std::setw(8) << "Size" 
           << std::setw(10) << "Price" 
           << std::setw(14) << "Depth" 
           << "│ " 
           << std::setw(10) << "Price" 
           << std::setw(8) << "Size" 
           << std::setw(14) << "Depth" 
           << "\n";
           
    output << "───────────────────────────────────┼───────────────────────────────────────\n";

    // Print bid and ask levels with visual depth indicator and change highlighting
    const int bar_width = 10;
    
    for (size_t i = 0; i < depth_; ++i) {
        std::stringstream line;
        
        // Print bid side
        if (i < bids.size()) {
            double bid_price = bids[i].first;
            double bid_size = bids[i].second;
            double cumulative_bid = 0.0;
            
            // Calculate cumulative size up to this level
            for (size_t j = 0; j <= i; ++j) {
                cumulative_bid += bids[j].second;
            }
            
            // Determine change type for this price level
            PriceChangeType bid_change = getPriceChangeType(bid_price, bid_size, prev_bids_);
            
            // If this price level has a change, update its highlight timer
            if (bid_change != PriceChangeType::NO_CHANGE) {
                bid_highlight_timers_[bid_price] = change_highlight_duration_;
            }
            
            // Format bid side with appropriate highlighting
            line << std::left 
                 << std::setw(8) << formatSize(bid_size, bid_change)
                 << std::setw(10) << formatPrice(bid_price, bid_change);
                 
            // Add visual depth indicator
            line << std::setw(3) << " ";
            renderProgressBar(line, cumulative_bid, bid_size_total, bar_width, true);
            line << std::setw(1) << " ";
        } else {
            line << std::setw(32) << " ";
        }
        
        // Center divider
        line << "│ ";
        
        // Print ask side with change highlighting
        if (i < asks.size()) {
            double ask_price = asks[i].first;
            double ask_size = asks[i].second;
            double cumulative_ask = 0.0;
            
            // Calculate cumulative size up to this level
            for (size_t j = 0; j <= i; ++j) {
                cumulative_ask += asks[j].second;
            }
            
            // Determine change type for this price level
            PriceChangeType ask_change = getPriceChangeType(ask_price, ask_size, prev_asks_);
            
            // If this price level has a change, update its highlight timer
            if (ask_change != PriceChangeType::NO_CHANGE) {
                ask_highlight_timers_[ask_price] = change_highlight_duration_;
            }
            
            // Format ask side with appropriate highlighting
            line << std::left
                 << std::setw(10) << formatPrice(ask_price, ask_change)
                 << std::setw(8) << formatSize(ask_size, ask_change);
                 
            // Add visual depth indicator
            line << std::setw(3) << " ";
            renderProgressBar(line, cumulative_ask, ask_size_total, bar_width, false);
        }
        
        output << line.str() << "\n";
    }

    output << "───────────────────────────────────────────────────────────────────────────\n";
    output << "Visualization updates every " << refresh_rate_ms_ << "ms. Press Ctrl+C to exit.\n";
    if (highlight_changes_) {
        output << Color::BRIGHT_GREEN << "▲" << Color::RESET << " = Increased  ";
        output << Color::BRIGHT_RED << "▼" << Color::RESET << " = Decreased  ";
        output << Color::BRIGHT_YELLOW << "●" << Color::RESET << " = New Level\n";
    }

    // Clear any potential old content - fill lines with spaces up to a reasonable screen width
    // This avoids artifacts from previous renders that might be longer
    std::string clear_line(80, ' ');
    for (size_t i = 0; i < 5; i++) {
        output << clear_line << "\n";
    }

    // Output the entire visualization at once to reduce flickering
    std::cout << output.str();
    
    // Flush output to ensure it's displayed
    std::cout.flush();
    
    // Update highlight timers after rendering
    updateHighlightTimers();
    
    // Store current state for next comparison
    updatePreviousState(bids, asks);
}

} // namespace clunk