#include "clunk/order_book.hpp"
#include <chrono>
#include <vector>

void benchmark_order_add() {
    OrderBook book;
    constexpr int N = 1'000'000;

    std::vector<Order> orders;
    orders.reserve(N);
    for(int i = 0; i < N; ++i) {
        orders.emplace_back(Order{
            .id = static_cast<OrderId>(i),
            .price = 1000 + (i % 100),
            .quantity = 100,
            .side = (i % 2) ? Side::Buy : Side::Sell,
            .type = OrderType::Limit,
            .timestamp = 0
        });
    }

    auto start = std::chrono::high_resolution_clock::now();
    for(auto& order : orders) {
        book.add_order(std::move(order));
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "Per order latency: " << ns.count()/N << "ns\n";
}