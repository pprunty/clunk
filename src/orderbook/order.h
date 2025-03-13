#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace clunk {

// Order side (buy or sell)
enum class OrderSide : uint8_t {
    BUY = 0,
    SELL = 1
};

// Order class representing a single order in the order book
class Order {
public:
    // Constructor
    Order(const std::string& id, OrderSide side, double price, double size,
          std::chrono::nanoseconds timestamp);

    // Copy and move constructors/assignments
    Order(const Order& other) = default;
    Order(Order&& other) noexcept = default;
    Order& operator=(const Order& other) = default;
    Order& operator=(Order&& other) noexcept = default;

    // Destructor
    ~Order() = default;

    // Getters
    const std::string& getId() const { return id_; }
    OrderSide getSide() const { return side_; }
    double getPrice() const { return price_; }
    double getSize() const { return size_; }
    std::chrono::nanoseconds getTimestamp() const { return timestamp_; }

    // Setters
    void setSize(double size) { size_ = size; }

    // Update order size (for partial fills)
    bool reduceSize(double amount);

    // Comparison operators (for sorting and containers)
    bool operator==(const Order& other) const { return id_ == other.id_; }
    bool operator<(const Order& other) const { return id_ < other.id_; }

private:
    std::string id_;                       // Unique order ID
    OrderSide side_;                       // BUY or SELL
    double price_;                         // Order price
    double size_;                          // Order size/quantity
    std::chrono::nanoseconds timestamp_;   // Timestamp when order was received
};

} // namespace clunk