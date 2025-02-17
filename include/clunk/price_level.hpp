#pragma once
#include "types.hpp"
#include <boost/intrusive/list.hpp>

namespace clunk {

class PriceLevel {
public:
    using OrderList = boost::intrusive::list<Order,
        boost::intrusive::member_hook<Order,
            boost::intrusive::list_member_hook<>,
            &Order::price_level_hook>>;

    PriceLevel(Price price) : price_(price), total_qty_(0) {}

    void add_order(Order* order) {
        orders_.push_back(*order);
        total_qty_ += order->quantity;
    }

    void remove_order(Order* order) {
        orders_.erase(orders_.iterator_to(*order));
        total_qty_ -= order->quantity;
    }

    // ... Other methods

private:
    Price price_;
    Quantity total_qty_;
    OrderList orders_;
};

} // namespace clunk