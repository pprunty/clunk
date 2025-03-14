#pragma once

#include "order.h"
#include "price_level.h"
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <functional>

namespace clunk {

// Callback types for order book updates
using OrderBookUpdateCallback = std::function<void()>;

// OrderBook class implementing a limit order book
class OrderBook {
public:
    // Constructor
    explicit OrderBook(const std::string& symbol);

    // Symbol getter
    const std::string& getSymbol() const { return symbol_; }

    // Order management functions
    bool addOrder(const std::shared_ptr<Order>& order);
    bool removeOrder(const std::string& order_id);
    bool modifyOrder(const std::string& order_id, double new_size);

    // Get best bid and ask
    double getBestBid() const;
    double getBestAsk() const;

    // Get bid-ask spread
    double getSpread() const;

    // Get order book depth at specified levels
    std::vector<std::pair<double, double>> getBidLevels(size_t depth) const;
    std::vector<std::pair<double, double>> getAskLevels(size_t depth) const;

    // Get midpoint price
    double getMidpointPrice() const;

    // Get order by ID
    std::shared_ptr<Order> getOrder(const std::string& order_id) const;

    // Get statistics
    size_t getOrderCount() const;
    size_t getBidLevelCount() const;
    size_t getAskLevelCount() const;

    // Set update callback
    void setUpdateCallback(OrderBookUpdateCallback callback) {
        update_callback_ = callback;
    }

    // Process an L3 update (add, modify, remove)
    void processL3Update(const std::string& type, const std::string& order_id,
                        OrderSide side, double price, double size);

    // Clear all orders from the order book
    void clear();

private:
    std::string symbol_;                                // Instrument symbol

    // Using maps for bid and ask levels ensures price ordering
    // For bids, we use reverse ordering to get highest bids first
    std::map<double, std::unique_ptr<PriceLevel>, std::greater<>> bid_levels_;
    std::map<double, std::unique_ptr<PriceLevel>> ask_levels_;

    // Map to quickly look up orders by ID
    std::unordered_map<std::string, std::shared_ptr<Order>> orders_;

    // Mutex for thread safety
    mutable std::mutex mutex_;

    // Callback for order book updates
    OrderBookUpdateCallback update_callback_;

    // Notify subscribers of updates
    void notifyUpdate() {
        if (update_callback_) {
            update_callback_();
        }
    }
};

} // namespace clunk