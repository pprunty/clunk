#include "price_level.h"

namespace clunk {

PriceLevel::PriceLevel(double price)
    : price_(price), total_size_(0.0) {
}

bool PriceLevel::addOrder(const std::shared_ptr<Order>& order) {
    // Verify the order is at the correct price level
    if (std::abs(order->getPrice() - price_) > std::numeric_limits<double>::epsilon()) {
        return false;
    }

    // Check if order already exists
    if (orders_.find(order->getId()) != orders_.end()) {
        return false;
    }

    // Add the order
    orders_[order->getId()] = order;
    total_size_ += order->getSize();

    return true;
}

bool PriceLevel::removeOrder(const std::string& order_id) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }

    // Subtract order size from total
    total_size_ -= it->second->getSize();

    // Remove the order
    orders_.erase(it);

    return true;
}

bool PriceLevel::updateOrder(const std::string& order_id, double new_size) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }

    double old_size = it->second->getSize();
    it->second->setSize(new_size);

    // Update total size
    total_size_ = total_size_ - old_size + new_size;

    return true;
}

std::shared_ptr<Order> PriceLevel::findOrder(const std::string& order_id) const {
    auto it = orders_.find(order_id);
    if (it != orders_.end()) {
        return it->second;
    }
    return nullptr;
}

} // namespace clunk