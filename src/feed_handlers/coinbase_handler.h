#pragma once

#include "feed_handler.h"
#include "network/websocket_client.h"
#include "orderbook/order_book.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>

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

    // Enable/disable verbose logging
    void setVerboseLogging(bool enabled);

    // Get an order book for a symbol
    std::shared_ptr<OrderBook> getOrderBook(const std::string& symbol);

private:
    // Coinbase API details - Using public WebSocket feed
    // The wss:// prefix is handled by the WebSocket client
    static constexpr const char* kHost = "ws-feed.exchange.coinbase.com";
    static constexpr const char* kPort = "443";
    static constexpr const char* kPath = "/ws";

    // Websocket client
    std::shared_ptr<WebSocketClient> ws_client_;

    // Order books by symbol
    std::unordered_map<std::string, std::shared_ptr<OrderBook>> order_books_;

    // Mutex for protecting order books
    std::mutex mutex_;

    // Verbose logging flag
    bool verbose_logging_;

    // Handle a message from the feed
    void handleMessage(const std::string& message);

    // Process different message types
    void processSnapshot(const nlohmann::json& json);
    void processL3Update(const nlohmann::json& json);
    void processTicker(const nlohmann::json& json);
    void processL2Update(const nlohmann::json& json);

    // Convert Coinbase order side to internal order side
    OrderSide convertOrderSide(const std::string& side);
};

} // namespace clunk