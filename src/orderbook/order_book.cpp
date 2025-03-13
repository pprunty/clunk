#include "order_book.h"
#include <algorithm>
#include <chrono>
#include <limits>

namespace clunk {

OrderBook::OrderBook(const std::string& symbol)
    : symbol_(symbol) {
}

bool OrderBook::addOrder(const std::shared_ptr<Order>& order) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if order already exists
    if (orders_.find(order->getId()) != orders_.end()) {
        return false;
    }

    // Get or create price level
    double price = order->getPrice();
    OrderSide side = order->getSide();

    // Use if-else instead of ternary to handle different map types
    if (side == OrderSide::BUY) {
        auto it = bid_levels_.find(price);
        if (it == bid_levels_.end()) {
            // Create new price level
            auto level = std::make_unique<PriceLevel>(price);
            level->addOrder(order);
            bid_levels_[price] = std::move(level);
        } else {
            // Add to existing level
            it->second->addOrder(order);
        }
    } else {
        auto it = ask_levels_.find(price);
        if (it == ask_levels_.end()) {
            // Create new price level
            auto level = std::make_unique<PriceLevel>(price);
            level->addOrder(order);
            ask_levels_[price] = std::move(level);
        } else {
            // Add to existing level
            it->second->addOrder(order);
        }
    }

    // Add to orders map for quick lookup
    orders_[order->getId()] = order;

    // Notify subscribers
    notifyUpdate();

    return true;
}

bool OrderBook::removeOrder(const std::string& order_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto order_it = orders_.find(order_id);
    if (order_it == orders_.end()) {
        return false;
    }

    std::shared_ptr<Order> order = order_it->second;
    double price = order->getPrice();
    OrderSide side = order->getSide();

    // Use if-else instead of ternary to handle different map types
    if (side == OrderSide::BUY) {
        auto level_it = bid_levels_.find(price);
        if (level_it != bid_levels_.end()) {
            level_it->second->removeOrder(order_id);

            // Remove price level if empty
            if (level_it->second->isEmpty()) {
                bid_levels_.erase(level_it);
            }
        }
    } else {
        auto level_it = ask_levels_.find(price);
        if (level_it != ask_levels_.end()) {
            level_it->second->removeOrder(order_id);

            // Remove price level if empty
            if (level_it->second->isEmpty()) {
                ask_levels_.erase(level_it);
            }
        }
    }

    // Remove from orders map
    orders_.erase(order_it);

    // Notify subscribers
    notifyUpdate();

    return true;
}

bool OrderBook::modifyOrder(const std::string& order_id, double new_size) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto order_it = orders_.find(order_id);
    if (order_it == orders_.end()) {
        return false;
    }

    std::shared_ptr<Order> order = order_it->second;
    double price = order->getPrice();
    OrderSide side = order->getSide();

    // Use if-else instead of ternary to handle different map types
    bool success = false;
    if (side == OrderSide::BUY) {
        auto level_it = bid_levels_.find(price);
        if (level_it != bid_levels_.end()) {
            success = level_it->second->updateOrder(order_id, new_size);
        }
    } else {
        auto level_it = ask_levels_.find(price);
        if (level_it != ask_levels_.end()) {
            success = level_it->second->updateOrder(order_id, new_size);
        }
    }

    if (success) {
        // Notify subscribers
        notifyUpdate();
    }

    return success;
}

double OrderBook::getBestBid() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!bid_levels_.empty()) {
        return bid_levels_.begin()->first;
    }

    return 0.0;
}

double OrderBook::getBestAsk() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!ask_levels_.empty()) {
        return ask_levels_.begin()->first;
    }

    return std::numeric_limits<double>::max();
}

double OrderBook::getSpread() const {
    double best_bid = getBestBid();
    double best_ask = getBestAsk();

    if (best_bid > 0.0 && best_ask < std::numeric_limits<double>::max()) {
        return best_ask - best_bid;
    }

    return 0.0;
}

std::vector<std::pair<double, double>> OrderBook::getBidLevels(size_t depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<double, double>> result;

    size_t count = 0;
    for (const auto& [price, level] : bid_levels_) {
        if (count >= depth) break;
        result.emplace_back(price, level->getTotalSize());
        ++count;
    }

    return result;
}

std::vector<std::pair<double, double>> OrderBook::getAskLevels(size_t depth) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<double, double>> result;

    size_t count = 0;
    for (const auto& [price, level] : ask_levels_) {
        if (count >= depth) break;
        result.emplace_back(price, level->getTotalSize());
        ++count;
    }

    return result;
}

double OrderBook::getMidpointPrice() const {
    double best_bid = getBestBid();
    double best_ask = getBestAsk();

    if (best_bid > 0.0 && best_ask < std::numeric_limits<double>::max()) {
        return (best_bid + best_ask) / 2.0;
    }

    return 0.0;
}

std::shared_ptr<Order> OrderBook::getOrder(const std::string& order_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = orders_.find(order_id);
    if (it != orders_.end()) {
        return it->second;
    }

    return nullptr;
}

size_t OrderBook::getOrderCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return orders_.size();
}

size_t OrderBook::getBidLevelCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bid_levels_.size();
}

size_t OrderBook::getAskLevelCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ask_levels_.size();
}

void OrderBook::processL3Update(const std::string& type, const std::string& order_id,
                              OrderSide side, double price, double size) {
    if (type == "open" || type == "received") {
        // New order
        auto order = std::make_shared<Order>(
            order_id, side, price, size,
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            )
        );
        addOrder(order);
    } else if (type == "match" || type == "change") {
        // Update order size
        modifyOrder(order_id, size);
    } else if (type == "done") {
        // Remove order
        removeOrder(order_id);
    }
}

} // namespace clunk