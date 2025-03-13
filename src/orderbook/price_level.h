#pragma once

#include "order.h"
#include <unordered_map>
#include <memory>
#include <limits>

namespace clunk {

// PriceLevel class representing all orders at a specific price point
class PriceLevel {
public:
    // Constructor
    explicit PriceLevel(double price);

    // Getters
    double getPrice() const { return price_; }
    double getTotalSize() const { return total_size_; }
    size_t getOrderCount() const { return orders_.size(); }

    // Order management functions
    bool addOrder(const std::shared_ptr<Order>& order);
    bool removeOrder(const std::string& order_id);
    bool updateOrder(const std::string& order_id, double new_size);

    // Find order by ID
    std::shared_ptr<Order> findOrder(const std::string& order_id) const;

    // Check if level is empty
    bool isEmpty() const { return orders_.empty(); }

    // Comparison operators (for sorting in the order book)
    bool operator<(const PriceLevel& other) const { return price_ < other.price_; }
    bool operator==(const PriceLevel& other) const {
        return std::abs(price_ - other.price_) < std::numeric_limits<double>::epsilon();
    }

private:
    double price_;                                            // Price of this level
    double total_size_;                                       // Total size of all orders at this level
    std::unordered_map<std::string, std::shared_ptr<Order>> orders_;  // Map of order ID to order
};

} // namespace clunk