#include "clunk/order_book.hpp"
#include <algorithm>
#include <immintrin.h>

namespace clunk {

OrderBook::OrderBook() {
    bids_.set_deallocate_on_destroy(false);
    asks_.set_deallocate_on_destroy(false);
}

bool OrderBook::add_order(Order&& order) {
    // Reject duplicate IDs
    if(orders_.find(order.id) != orders_.end()) return false;

    // Allocate from pool
    Order* new_order = order_pool_.construct(std::move(order));
    orders_[new_order->id] = new_order;

    // Branchless dispatch
    (new_order->side == Side::Buy)
        ? process_order<Side::Buy>(new_order)
        : process_order<Side::Sell>(new_order);

    update_best_prices();
    return true;
}

template<Side S>
void OrderBook::process_order(Order* order) {
    auto& levels = (S == Side::Buy) ? bids_ : asks_;
    constexpr auto cmp = (S == Side::Buy)
        ? std::greater<>()
        : std::less<>();

    // Find or create price level
    auto it = levels.find(order->price);
    if(it == levels.end()) {
        it = levels.emplace(order->price, PriceLevel(order->price)).first;
    }

    it->second.add_order(order);

    // Add cross-side matching logic here
    // Implement FIFO matching algorithm
    // Handle different order types (IOC/FOK)
}

void OrderBook::update_best_prices() const {
    cached_best_bid_ = bids_.empty() ? 0 : bids_.rbegin()->first;
    cached_best_ask_ = asks_.empty() ? 0 : asks_.begin()->first;
}

// ... Implement modify/cancel with similar optimizations

} // namespace clunk