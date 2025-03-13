#pragma once

#include "feed_handler.h"
#include "../network/websocket_client.h"
#include "../orderbook/order_book.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <map>
#include <mutex>

namespace clunk {

// Handler for Coinbase's L3 market data feed
class CoinbaseHandler : public FeedHandler {
public:
    // Constructor
    CoinbaseHandler();

    // Destructor
    ~CoinbaseHandler() override;

    // Connect to the feed
    void connect() override;

    // Disconnect from the feed
    void disconnect() override;

    // Subscribe to a symbol
    void subscribe(const std::string& symbol) override;

    // Unsubscribe from a symbol
    void unsubscribe(const std::string& symbol) override;

    // Check if connected
    bool isConnected() const override;

    // Get an order book for a symbol
    std::shared_ptr<OrderBook> getOrderBook(const std::string& symbol);

private:
    // Coinbase API details
    static constexpr const char* kHost = "ws-feed.exchange.coinbase.com";
    static constexpr const char* kPort = "443";

    // Websocket client
    std::shared_ptr<WebSocketClient> ws_client_;

    // Order books by symbol
    std::map<std::string, std::shared_ptr<OrderBook>> order_books_;

    // Mutex for protecting order books
    std::mutex mutex_;

    // Handle a message from the feed
    void handleMessage(const std::string& message);

    // Process different message types
    void processSnapshot(const nlohmann::json& json);
    void processL3Update(const nlohmann::json& json);

    // Convert Coinbase order side to internal order side
    OrderSide convertOrderSide(const std::string& side);
};

} // namespace clunk