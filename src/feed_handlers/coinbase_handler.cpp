#include "coinbase_handler.h"
#include <iostream>
#include <chrono>

namespace clunk {

using json = nlohmann::json;

CoinbaseHandler::CoinbaseHandler() {
    // Create the websocket client
    ws_client_ = std::make_shared<WebSocketClient>(kHost, kPort);

    // Set up the message callback
    ws_client_->setMessageCallback([this](const std::string& message) {
        handleMessage(message);
    });
}

CoinbaseHandler::~CoinbaseHandler() {
    disconnect();
}

void CoinbaseHandler::connect() {
    ws_client_->connect();
}

void CoinbaseHandler::disconnect() {
    if (ws_client_) {
        ws_client_->disconnect();
    }
}

bool CoinbaseHandler::isConnected() const {
    return ws_client_ && ws_client_->isConnected();
}

void CoinbaseHandler::subscribe(const std::string& symbol) {
    if (!isConnected()) {
        std::cerr << "Not connected, cannot subscribe to " << symbol << std::endl;
        return;
    }

    // Create order book if it doesn't exist
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (order_books_.find(symbol) == order_books_.end()) {
            order_books_[symbol] = std::make_shared<OrderBook>(symbol);
        }
    }

    // Create subscription message
    json subscription = {
        {"type", "subscribe"},
        {"product_ids", {symbol}},
        {"channels", {"full"}}  // "full" is Coinbase's L3 order book feed
    };

    // Send subscription message
    ws_client_->send(subscription.dump());
}

void CoinbaseHandler::unsubscribe(const std::string& symbol) {
    if (!isConnected()) {
        return;
    }

    // Create unsubscription message
    json unsubscription = {
        {"type", "unsubscribe"},
        {"product_ids", {symbol}},
        {"channels", {"full"}}
    };

    // Send unsubscription message
    ws_client_->send(unsubscription.dump());

    // Remove order book
    {
        std::lock_guard<std::mutex> lock(mutex_);
        order_books_.erase(symbol);
    }
}

std::shared_ptr<OrderBook> CoinbaseHandler::getOrderBook(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = order_books_.find(symbol);
    if (it != order_books_.end()) {
        return it->second;
    }
    return nullptr;
}

void CoinbaseHandler::handleMessage(const std::string& message) {
    try {
        // Parse the JSON message
        json j = json::parse(message);

        // Get the message type
        std::string type = j["type"];

        if (type == "snapshot") {
            processSnapshot(j);
        } else if (type == "l3update" || type == "received" || type == "open" ||
                  type == "done" || type == "match" || type == "change") {
            processL3Update(j);
        } else if (type == "error") {
            std::cerr << "Coinbase API error: " << j["message"] << std::endl;
        } else if (type == "subscriptions") {
            std::cout << "Subscribed to channels: " << j["channels"] << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
    }
}

void CoinbaseHandler::processSnapshot(const nlohmann::json& j) {
    // Get the product ID (symbol)
    std::string symbol = j["product_id"];

    // Get the order book
    auto order_book = getOrderBook(symbol);
    if (!order_book) {
        return;
    }

    // Process bids
    for (const auto& bid : j["bids"]) {
        // Each bid is [price, size, order_id]
        double price = std::stod(bid[0].get<std::string>());
        double size = std::stod(bid[1].get<std::string>());
        std::string order_id = bid[2];

        // Create and add the order
        auto order = std::make_shared<Order>(
            order_id, OrderSide::BUY, price, size,
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            )
        );
        order_book->addOrder(order);
    }

    // Process asks
    for (const auto& ask : j["asks"]) {
        // Each ask is [price, size, order_id]
        double price = std::stod(ask[0].get<std::string>());
        double size = std::stod(ask[1].get<std::string>());
        std::string order_id = ask[2];

        // Create and add the order
        auto order = std::make_shared<Order>(
            order_id, OrderSide::SELL, price, size,
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            )
        );
        order_book->addOrder(order);
    }
}

void CoinbaseHandler::processL3Update(const nlohmann::json& j) {
    // Get the product ID (symbol)
    std::string symbol = j["product_id"];

    // Get the order book
    auto order_book = getOrderBook(symbol);
    if (!order_book) {
        return;
    }

    // Get the message type
    std::string type = j["type"];

    // Get the order ID
    std::string order_id = j["order_id"];

    if (type == "received" || type == "open") {
        // New order
        std::string side_str = j["side"];
        OrderSide side = convertOrderSide(side_str);
        double price = std::stod(j["price"].get<std::string>());
        double size = std::stod(j["size"].get<std::string>());

        // Process as a new order
        order_book->processL3Update("open", order_id, side, price, size);
    } else if (type == "done") {
        // Order removed
        order_book->processL3Update("done", order_id, OrderSide::BUY, 0.0, 0.0);
    } else if (type == "match") {
        // Order matched (partial or full fill)
        std::string maker_order_id = j["maker_order_id"];
        std::string taker_order_id = j["taker_order_id"];
        double size = std::stod(j["size"].get<std::string>());

        // Process both maker and taker orders
        auto maker_order = order_book->getOrder(maker_order_id);
        if (maker_order) {
            double new_size = maker_order->getSize() - size;
            if (new_size <= 0) {
                // Fully filled, remove order
                order_book->processL3Update("done", maker_order_id, maker_order->getSide(), 0.0, 0.0);
            } else {
                // Partially filled, update size
                order_book->processL3Update("change", maker_order_id, maker_order->getSide(),
                                           maker_order->getPrice(), new_size);
            }
        }

        // For taker order, we usually don't have it in the book yet,
        // so we can just ignore it or treat it as a market order
    } else if (type == "change") {
        // Order size changed
        double new_size = std::stod(j["new_size"].get<std::string>());

        // Get the order to determine the side
        auto order = order_book->getOrder(order_id);
        if (order) {
            order_book->processL3Update("change", order_id, order->getSide(),
                                       order->getPrice(), new_size);
        }
    }
}

OrderSide CoinbaseHandler::convertOrderSide(const std::string& side) {
    if (side == "buy") {
        return OrderSide::BUY;
    } else {
        return OrderSide::SELL;
    }
}

} // namespace clunk