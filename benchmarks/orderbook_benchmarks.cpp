#include <benchmark/benchmark.h>
#include "orderbook/order_book.h"
#include <random>
#include <string>
#include <chrono>
#include <memory>
#include <vector>

using namespace clunk;

// Helper function to create an order
std::shared_ptr<Order> createBenchOrder(const std::string& id, OrderSide side, double price, double size) {
    return std::make_shared<Order>(
        id, side, price, size,
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        )
    );
}

// Benchmark adding a single order
static void BM_AddOrder(benchmark::State& state) {
    OrderBook book("BTC-USD");
    int order_id = 0;

    for (auto _ : state) {
        std::string id = "order-" + std::to_string(order_id++);
        auto order = createBenchOrder(id, OrderSide::BUY, 100.0, 1.0);
        book.addOrder(order);
    }
}
BENCHMARK(BM_AddOrder);

// Benchmark removing a single order
static void BM_RemoveOrder(benchmark::State& state) {
    OrderBook book("BTC-USD");
    std::vector<std::string> order_ids;

    // Add some orders first
    for (int i = 0; i < state.range(0); ++i) {
        std::string id = "order-" + std::to_string(i);
        auto order = createBenchOrder(id, OrderSide::BUY, 100.0, 1.0);
        book.addOrder(order);
        order_ids.push_back(id);
    }

    int index = 0;
    for (auto _ : state) {
        book.removeOrder(order_ids[index % order_ids.size()]);
        ++index;
    }
}
BENCHMARK(BM_RemoveOrder)->Arg(1000);

// Benchmark modifying a single order
static void BM_ModifyOrder(benchmark::State& state) {
    OrderBook book("BTC-USD");
    std::vector<std::string> order_ids;

    // Add some orders first
    for (int i = 0; i < state.range(0); ++i) {
        std::string id = "order-" + std::to_string(i);
        auto order = createBenchOrder(id, OrderSide::BUY, 100.0, 1.0);
        book.addOrder(order);
        order_ids.push_back(id);
    }

    int index = 0;
    for (auto _ : state) {
        book.modifyOrder(order_ids[index % order_ids.size()], 2.0);
        ++index;
    }
}
BENCHMARK(BM_ModifyOrder)->Arg(1000);

// Benchmark getting the best bid/ask
static void BM_GetBestBidAsk(benchmark::State& state) {
    OrderBook book("BTC-USD");

    // Set up a realistic order book
    std::mt19937 rng(42);  // Fixed seed for reproducibility
    std::uniform_real_distribution<> price_dist(9000.0, 11000.0);
    std::uniform_real_distribution<> size_dist(0.1, 10.0);

    // Add some bid orders
    for (int i = 0; i < state.range(0) / 2; ++i) {
        std::string id = "bid-" + std::to_string(i);
        double price = price_dist(rng);
        double size = size_dist(rng);
        auto order = createBenchOrder(id, OrderSide::BUY, price, size);
        book.addOrder(order);
    }

    // Add some ask orders
    for (int i = 0; i < state.range(0) / 2; ++i) {
        std::string id = "ask-" + std::to_string(i);
        double price = price_dist(rng);
        double size = size_dist(rng);
        auto order = createBenchOrder(id, OrderSide::SELL, price, size);
        book.addOrder(order);
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(book.getBestBid());
        benchmark::DoNotOptimize(book.getBestAsk());
    }
}
BENCHMARK(BM_GetBestBidAsk)->Arg(1000)->Arg(10000)->Arg(100000);

// Benchmark getting the spread
static void BM_GetSpread(benchmark::State& state) {
    OrderBook book("BTC-USD");

    // Set up a realistic order book (same as above)
    std::mt19937 rng(42);
    std::uniform_real_distribution<> price_dist(9000.0, 11000.0);
    std::uniform_real_distribution<> size_dist(0.1, 10.0);

    for (int i = 0; i < state.range(0) / 2; ++i) {
        std::string id = "bid-" + std::to_string(i);
        double price = price_dist(rng);
        double size = size_dist(rng);
        auto order = createBenchOrder(id, OrderSide::BUY, price, size);
        book.addOrder(order);
    }

    for (int i = 0; i < state.range(0) / 2; ++i) {
        std::string id = "ask-" + std::to_string(i);
        double price = price_dist(rng);
        double size = size_dist(rng);
        auto order = createBenchOrder(id, OrderSide::SELL, price, size);
        book.addOrder(order);
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(book.getSpread());
    }
}
BENCHMARK(BM_GetSpread)->Arg(1000)->Arg(10000);

// Benchmark getting the midpoint price
static void BM_GetMidpointPrice(benchmark::State& state) {
    OrderBook book("BTC-USD");

    // Set up a realistic order book (same as above)
    std::mt19937 rng(42);
    std::uniform_real_distribution<> price_dist(9000.0, 11000.0);
    std::uniform_real_distribution<> size_dist(0.1, 10.0);

    for (int i = 0; i < state.range(0) / 2; ++i) {
        std::string id = "bid-" + std::to_string(i);
        double price = price_dist(rng);
        double size = size_dist(rng);
        auto order = createBenchOrder(id, OrderSide::BUY, price, size);
        book.addOrder(order);
    }

    for (int i = 0; i < state.range(0) / 2; ++i) {
        std::string id = "ask-" + std::to_string(i);
        double price = price_dist(rng);
        double size = size_dist(rng);
        auto order = createBenchOrder(id, OrderSide::SELL, price, size);
        book.addOrder(order);
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(book.getMidpointPrice());
    }
}
BENCHMARK(BM_GetMidpointPrice)->Arg(1000)->Arg(10000);

// Benchmark getting top N levels
static void BM_GetLevels(benchmark::State& state) {
    OrderBook book("BTC-USD");

    // Set up a realistic order book (same as above)
    std::mt19937 rng(42);
    std::uniform_real_distribution<> price_dist(9000.0, 11000.0);
    std::uniform_real_distribution<> size_dist(0.1, 10.0);

    for (int i = 0; i < state.range(0); ++i) {
        std::string id = "bid-" + std::to_string(i);
        double price = price_dist(rng);
        double size = size_dist(rng);
        auto order = createBenchOrder(id, OrderSide::BUY, price, size);
        book.addOrder(order);
    }

    for (int i = 0; i < state.range(0); ++i) {
        std::string id = "ask-" + std::to_string(i);
        double price = price_dist(rng);
        double size = size_dist(rng);
        auto order = createBenchOrder(id, OrderSide::SELL, price, size);
        book.addOrder(order);
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(book.getBidLevels(10));
        benchmark::DoNotOptimize(book.getAskLevels(10));
    }
}
BENCHMARK(BM_GetLevels)->Arg(1000)->Arg(10000);

// Benchmark L3 update processing
static void BM_ProcessL3Update(benchmark::State& state) {
    OrderBook book("BTC-USD");

    // Generate a sequence of L3 updates
    std::vector<std::tuple<std::string, std::string, OrderSide, double, double>> updates;

    // Add some orders
    for (int i = 0; i < 1000; ++i) {
        std::string id = "order-" + std::to_string(i);
        OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        double price = 10000.0 + (i % 100);
        double size = 1.0 + (i % 10);

        updates.emplace_back("open", id, side, price, size);
    }

    // Modify some orders
    for (int i = 0; i < 500; ++i) {
        std::string id = "order-" + std::to_string(i);
        OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;
        double price = 10000.0 + (i % 100);
        double size = 0.5 + (i % 5);

        updates.emplace_back("change", id, side, price, size);
    }

    // Remove some orders
    for (int i = 500; i < 1000; ++i) {
        std::string id = "order-" + std::to_string(i);
        OrderSide side = (i % 2 == 0) ? OrderSide::BUY : OrderSide::SELL;

        updates.emplace_back("done", id, side, 0.0, 0.0);
    }

    int index = 0;
    for (auto _ : state) {
        const auto& [type, id, side, price, size] = updates[index % updates.size()];
        book.processL3Update(type, id, side, price, size);
        ++index;
    }
}
BENCHMARK(BM_ProcessL3Update);

BENCHMARK_MAIN();