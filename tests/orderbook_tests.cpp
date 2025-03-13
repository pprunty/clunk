#include <gtest/gtest.h>
#include "orderbook/order_book.h"
#include <memory>
#include <chrono>

using namespace clunk;

// Helper function to create orders with a timestamp
std::shared_ptr<Order> createOrder(const std::string& id, OrderSide side, double price, double size) {
    return std::make_shared<Order>(
        id, side, price, size,
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        )
    );
}

// Test suite for the Order class
class OrderTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test order
        order_ = createOrder("test-order-1", OrderSide::BUY, 100.0, 1.5);
    }
    
    std::shared_ptr<Order> order_;
};

// Test basic order properties
TEST_F(OrderTests, BasicProperties) {
    EXPECT_EQ(order_->getId(), "test-order-1");
    EXPECT_EQ(order_->getSide(), OrderSide::BUY);
    EXPECT_DOUBLE_EQ(order_->getPrice(), 100.0);
    EXPECT_DOUBLE_EQ(order_->getSize(), 1.5);
}

// Test order size reduction
TEST_F(OrderTests, ReduceSize) {
    EXPECT_TRUE(order_->reduceSize(0.5));
    EXPECT_DOUBLE_EQ(order_->getSize(), 1.0);
    
    // Try to reduce by too much
    EXPECT_FALSE(order_->reduceSize(2.0));
    EXPECT_DOUBLE_EQ(order_->getSize(), 1.0);
    
    // Try to reduce by a negative amount
    EXPECT_FALSE(order_->reduceSize(-0.5));
    EXPECT_DOUBLE_EQ(order_->getSize(), 1.0);
}

// Test suite for the PriceLevel class
class PriceLevelTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test price level
        level_ = std::make_unique<PriceLevel>(100.0);
        
        // Create some test orders
        order1_ = createOrder("test-order-1", OrderSide::BUY, 100.0, 1.5);
        order2_ = createOrder("test-order-2", OrderSide::BUY, 100.0, 2.5);
    }
    
    std::unique_ptr<PriceLevel> level_;
    std::shared_ptr<Order> order1_;
    std::shared_ptr<Order> order2_;
};

// Test adding orders to a price level
TEST_F(PriceLevelTests, AddOrder) {
    EXPECT_TRUE(level_->addOrder(order1_));
    EXPECT_EQ(level_->getOrderCount(), 1);
    EXPECT_DOUBLE_EQ(level_->getTotalSize(), 1.5);
    
    EXPECT_TRUE(level_->addOrder(order2_));
    EXPECT_EQ(level_->getOrderCount(), 2);
    EXPECT_DOUBLE_EQ(level_->getTotalSize(), 4.0);
    
    // Try to add an order at the wrong price
    auto wrong_price_order = createOrder("test-order-3", OrderSide::BUY, 101.0, 1.0);
    EXPECT_FALSE(level_->addOrder(wrong_price_order));
    
    // Try to add a duplicate order
    EXPECT_FALSE(level_->addOrder(order1_));
}

// Test removing orders from a price level
TEST_F(PriceLevelTests, RemoveOrder) {
    EXPECT_TRUE(level_->addOrder(order1_));
    EXPECT_TRUE(level_->addOrder(order2_));
    
    EXPECT_TRUE(level_->removeOrder("test-order-1"));
    EXPECT_EQ(level_->getOrderCount(), 1);
    EXPECT_DOUBLE_EQ(level_->getTotalSize(), 2.5);
    
    // Try to remove a non-existent order
    EXPECT_FALSE(level_->removeOrder("non-existent"));
    
    EXPECT_TRUE(level_->removeOrder("test-order-2"));
    EXPECT_EQ(level_->getOrderCount(), 0);
    EXPECT_DOUBLE_EQ(level_->getTotalSize(), 0.0);
    EXPECT_TRUE(level_->isEmpty());
}

// Test updating orders in a price level
TEST_F(PriceLevelTests, UpdateOrder) {
    EXPECT_TRUE(level_->addOrder(order1_));
    
    EXPECT_TRUE(level_->updateOrder("test-order-1", 3.0));
    EXPECT_DOUBLE_EQ(level_->getTotalSize(), 3.0);
    
    // Try to update a non-existent order
    EXPECT_FALSE(level_->updateOrder("non-existent", 1.0));
}

// Test suite for the OrderBook class
class OrderBookTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test order book
        book_ = std::make_unique<OrderBook>("BTC-USD");
        
        // Create some test orders
        bid1_ = createOrder("bid-1", OrderSide::BUY, 100.0, 1.5);
        bid2_ = createOrder("bid-2", OrderSide::BUY, 99.0, 2.5);
        ask1_ = createOrder("ask-1", OrderSide::SELL, 101.0, 1.0);
        ask2_ = createOrder("ask-2", OrderSide::SELL, 102.0, 2.0);
    }
    
    std::unique_ptr<OrderBook> book_;
    std::shared_ptr<Order> bid1_;
    std::shared_ptr<Order> bid2_;
    std::shared_ptr<Order> ask1_;
    std::shared_ptr<Order> ask2_;
};

// Test adding orders to the order book
TEST_F(OrderBookTests, AddOrder) {
    EXPECT_TRUE(book_->addOrder(bid1_));
    EXPECT_TRUE(book_->addOrder(bid2_));
    EXPECT_TRUE(book_->addOrder(ask1_));
    EXPECT_TRUE(book_->addOrder(ask2_));
    
    EXPECT_EQ(book_->getOrderCount(), 4);
    EXPECT_EQ(book_->getBidLevelCount(), 2);
    EXPECT_EQ(book_->getAskLevelCount(), 2);
    
    // Try to add a duplicate order
    EXPECT_FALSE(book_->addOrder(bid1_));
}

// Test removing orders from the order book
TEST_F(OrderBookTests, RemoveOrder) {
    EXPECT_TRUE(book_->addOrder(bid1_));
    EXPECT_TRUE(book_->addOrder(bid2_));
    
    EXPECT_TRUE(book_->removeOrder("bid-1"));
    EXPECT_EQ(book_->getOrderCount(), 1);
    EXPECT_EQ(book_->getBidLevelCount(), 1);
    
    // Try to remove a non-existent order
    EXPECT_FALSE(book_->removeOrder("non-existent"));
}

// Test modifying orders in the order book
TEST_F(OrderBookTests, ModifyOrder) {
    EXPECT_TRUE(book_->addOrder(bid1_));
    
    EXPECT_TRUE(book_->modifyOrder("bid-1", 3.0));
    
    // Try to modify a non-existent order
    EXPECT_FALSE(book_->modifyOrder("non-existent", 1.0));
}

// Test getting the best bid and ask
TEST_F(OrderBookTests, BestBidAsk) {
    EXPECT_DOUBLE_EQ(book_->getBestBid(), 0.0);
    EXPECT_DOUBLE_EQ(book_->getBestAsk(), std::numeric_limits<double>::max());
    
    EXPECT_TRUE(book_->addOrder(bid1_));
    EXPECT_TRUE(book_->addOrder(bid2_));
    EXPECT_TRUE(book_->addOrder(ask1_));
    EXPECT_TRUE(book_->addOrder(ask2_));
    
    EXPECT_DOUBLE_EQ(book_->getBestBid(), 100.0);
    EXPECT_DOUBLE_EQ(book_->getBestAsk(), 101.0);
    EXPECT_DOUBLE_EQ(book_->getSpread(), 1.0);
    EXPECT_DOUBLE_EQ(book_->getMidpointPrice(), 100.5);
}

// Test getting bid and ask levels
TEST_F(OrderBookTests, GetLevels) {
    EXPECT_TRUE(book_->addOrder(bid1_));
    EXPECT_TRUE(book_->addOrder(bid2_));
    EXPECT_TRUE(book_->addOrder(ask1_));
    EXPECT_TRUE(book_->addOrder(ask2_));
    
    auto bid_levels = book_->getBidLevels(10);
    EXPECT_EQ(bid_levels.size(), 2);
    EXPECT_DOUBLE_EQ(bid_levels[0].first, 100.0);
    EXPECT_DOUBLE_EQ(bid_levels[0].second, 1.5);
    EXPECT_DOUBLE_EQ(bid_levels[1].first, 99.0);
    EXPECT_DOUBLE_EQ(bid_levels[1].second, 2.5);
    
    auto ask_levels = book_->getAskLevels(10);
    EXPECT_EQ(ask_levels.size(), 2);
    EXPECT_DOUBLE_EQ(ask_levels[0].first, 101.0);
    EXPECT_DOUBLE_EQ(ask_levels[0].second, 1.0);
    EXPECT_DOUBLE_EQ(ask_levels[1].first, 102.0);
    EXPECT_DOUBLE_EQ(ask_levels[1].second, 2.0);
}

// Test processing L3 updates
TEST_F(OrderBookTests, ProcessL3Update) {
    // Process an open order
    book_->processL3Update("open", "bid-1", OrderSide::BUY, 100.0, 1.5);
    EXPECT_EQ(book_->getOrderCount(), 1);
    EXPECT_DOUBLE_EQ(book_->getBestBid(), 100.0);
    
    // Process a done order
    book_->processL3Update("done", "bid-1", OrderSide::BUY, 0.0, 0.0);
    EXPECT_EQ(book_->getOrderCount(), 0);
    EXPECT_DOUBLE_EQ(book_->getBestBid(), 0.0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}