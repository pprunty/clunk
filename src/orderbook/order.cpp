#include "order.h"
#include <algorithm>

namespace clunk {

Order::Order(const std::string& id, OrderSide side, double price, double size,
             std::chrono::nanoseconds timestamp)
    : id_(id), side_(side), price_(price), size_(size), timestamp_(timestamp) {
}

bool Order::reduceSize(double amount) {
    if (amount <= 0 || amount > size_) {
        return false;
    }

    size_ -= amount;
    return true;
}

} // namespace clunk