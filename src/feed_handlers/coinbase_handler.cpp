#include "coinbase_handler.h"
#include <iostream>
#include <chrono>

namespace clunk {

using json = nlohmann::json;

CoinbaseHandler::CoinbaseHandler() : verbose_logging_(false) {
    // Create the websocket client
    ws_client_ = std::make_shared<WebSocketClient>(kHost, kPort);

    // Set up the message callback
    ws_client_->setMessageCallback([this](const std::string& message) {
        handleMessage(message);
    });
    
    // Set the path for WebSocket handshake
    ws_client_->setPath(kPath);
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

void CoinbaseHandler::setVerboseLogging(bool enabled) {
    verbose_logging_ = enabled;
    if (ws_client_) {
        ws_client_->setVerboseLogging(enabled);
    }
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

    // Create subscription message for Coinbase public feed
    // Subscribe to level2 for full order book depth
    json subscription = {
        {"type", "subscribe"},
        {"product_ids", {symbol}},
        {"channels", {"level2", "ticker", "heartbeat"}}
    };

    // Send subscription message
    if (verbose_logging_) {
        std::cout << "Sending subscription: " << subscription.dump() << std::endl;
    }
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
        {"channels", {"level2", "heartbeat", "ticker"}}
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
        if (verbose_logging_) {
            std::cout << "Received message type: " << type << std::endl;
        }

        if (type == "snapshot") {
            processSnapshot(j);
        } else if (type == "l2update") {
            // Process level2 updates
            processL2Update(j);
        } else if (type == "ticker") {
            // Process ticker data
            if (verbose_logging_) {
                std::cout << "Received ticker: " << j.dump(2) << std::endl;
            }
            
            // For ticker data, update the order book with best bid/ask
            if (j.contains("product_id") && 
                j.contains("best_bid") && j.contains("best_ask") &&
                j.contains("best_bid_size") && j.contains("best_ask_size")) {
                
                processTicker(j);
            }
        } else if (type == "l3update" || type == "received" || type == "open" ||
                  type == "done" || type == "match" || type == "change") {
            processL3Update(j);
        } else if (type == "error") {
            std::cerr << "Coinbase API error: " << j["message"] << std::endl;
        } else if (type == "subscriptions") {
            if (verbose_logging_) {
                std::cout << "Subscribed to channels: " << j.dump(2) << std::endl;
            }
        } else {
            // Handle other message types
            if (verbose_logging_) {
                std::cout << "Unhandled message type: " << type << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
    }
}

void CoinbaseHandler::processSnapshot(const nlohmann::json& j) {
    if (verbose_logging_) {
        std::cout << "Processing snapshot: " << j.dump(2) << std::endl;
    }

    // Check if product_id exists in this message
    if (!j.contains("product_id")) {
        std::cerr << "Snapshot missing product_id field" << std::endl;
        return;
    }

    // Get the product ID (symbol)
    std::string symbol = j["product_id"];

    // Get the order book
    auto order_book = getOrderBook(symbol);
    if (!order_book) {
        std::cerr << "Order book not found for symbol: " << symbol << std::endl;
        return;
    }

    // Check if bids and asks fields are in the message
    if (!j.contains("bids") || !j.contains("asks")) {
        std::cerr << "Snapshot missing bids or asks fields" << std::endl;
        return;
    }

    try {
        // Process bids
        for (const auto& bid : j["bids"]) {
            // Each bid is [price, size, order_id]
            double price = std::stod(bid[0].get<std::string>());
            double size = std::stod(bid[1].get<std::string>());
            std::string order_id = bid.size() > 2 ? bid[2].get<std::string>() : "bid-" + bid[0].get<std::string>();

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
            std::string order_id = ask.size() > 2 ? ask[2].get<std::string>() : "ask-" + ask[0].get<std::string>();

            // Create and add the order
            auto order = std::make_shared<Order>(
                order_id, OrderSide::SELL, price, size,
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                )
            );
            order_book->addOrder(order);
        }

        if (verbose_logging_) {
            std::cout << "Processed snapshot with " << j["bids"].size() << " bids and "
                    << j["asks"].size() << " asks" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing snapshot data: " << e.what() << std::endl;
    }
}

void CoinbaseHandler::processL3Update(const nlohmann::json& j) {
    // In sandbox, we might not receive L3 updates, but let's keep the code for future use
    if (verbose_logging_) {
        std::cout << "Processing L3 update: " << j.dump(2) << std::endl;
    }

    // These are the fields we expect in L3 updates
    if (!j.contains("product_id")) {
        std::cerr << "L3 update missing product_id field" << std::endl;
        return;
    }

    // Get the product ID (symbol)
    std::string symbol = j["product_id"];

    // Get the order book
    auto order_book = getOrderBook(symbol);
    if (!order_book) {
        std::cerr << "Order book not found for symbol: " << symbol << std::endl;
        return;
    }

    // Get the message type
    std::string type = j["type"];

    try {
        // Different message types have different fields
        if (type == "received" || type == "open") {
            // Check if we have all the required fields
            if (!j.contains("order_id") || !j.contains("side") || !j.contains("price") || !j.contains("size")) {
                std::cerr << "Missing fields in received/open message" << std::endl;
                return;
            }

            // New order
            std::string order_id = j["order_id"];
            std::string side_str = j["side"];
            OrderSide side = convertOrderSide(side_str);
            double price = std::stod(j["price"].get<std::string>());
            double size = std::stod(j["size"].get<std::string>());

            // Process as a new order
            order_book->processL3Update("open", order_id, side, price, size);

        } else if (type == "done") {
            // Check if we have the order_id field
            if (!j.contains("order_id")) {
                std::cerr << "Missing order_id in done message" << std::endl;
                return;
            }

            // Order removed
            std::string order_id = j["order_id"];
            order_book->processL3Update("done", order_id, OrderSide::BUY, 0.0, 0.0);

        } else if (type == "match") {
            // Check if we have all the required fields
            if (!j.contains("maker_order_id") || !j.contains("size")) {
                std::cerr << "Missing fields in match message" << std::endl;
                return;
            }

            // Order matched (partial or full fill)
            std::string maker_order_id = j["maker_order_id"];
            double size = std::stod(j["size"].get<std::string>());

            // Process the maker order
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

        } else if (type == "change") {
            // Check if we have all the required fields
            if (!j.contains("order_id") || !j.contains("new_size")) {
                std::cerr << "Missing fields in change message" << std::endl;
                return;
            }

            // Order size changed
            std::string order_id = j["order_id"];
            double new_size = std::stod(j["new_size"].get<std::string>());

            // Get the order to determine the side
            auto order = order_book->getOrder(order_id);
            if (order) {
                order_book->processL3Update("change", order_id, order->getSide(),
                                           order->getPrice(), new_size);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing L3 update: " << e.what() << std::endl;
    }
}

void CoinbaseHandler::processTicker(const nlohmann::json& j) {
    // Get the product ID (symbol)
    std::string symbol = j["product_id"];

    // Get the order book
    auto order_book = getOrderBook(symbol);
    if (!order_book) {
        std::cerr << "Order book not found for symbol: " << symbol << std::endl;
        return;
    }

    try {
        // Helper function to handle both string and numeric values in JSON
        auto getDoubleValue = [](const json& value) -> double {
            if (value.is_string()) {
                return std::stod(value.get<std::string>());
            } else if (value.is_number()) {
                return value.get<double>();
            }
            throw std::runtime_error("Value is neither string nor number");
        };

        // Get best bid and ask
        double best_bid_price = getDoubleValue(j["best_bid"]);
        double best_bid_size = getDoubleValue(j["best_bid_size"]);
        double best_ask_price = getDoubleValue(j["best_ask"]);
        double best_ask_size = getDoubleValue(j["best_ask_size"]);

        // Generate a unique sequence ID
        std::string sequence_str;
        if (j["sequence"].is_number()) {
            sequence_str = std::to_string(j["sequence"].get<int64_t>());
        } else {
            sequence_str = j["sequence"].get<std::string>();
        }

        // Update the order book with best bid
        auto bid_order = std::make_shared<Order>(
            "bid-" + sequence_str, 
            OrderSide::BUY, 
            best_bid_price, 
            best_bid_size,
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            )
        );
        
        // Update the order book with best ask
        auto ask_order = std::make_shared<Order>(
            "ask-" + sequence_str, 
            OrderSide::SELL, 
            best_ask_price, 
            best_ask_size,
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            )
        );

        // Clear existing orders and add new ones
        // This is a simplified approach - in a real system, you'd want to maintain the full order book
        order_book->clear();
        order_book->addOrder(bid_order);
        order_book->addOrder(ask_order);
        
        if (verbose_logging_) {
            std::cout << "Updated order book: Best bid=" << best_bid_price 
                    << " (" << best_bid_size << "), Best ask=" 
                    << best_ask_price << " (" << best_ask_size << ")" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing ticker data: " << e.what() << std::endl;
    }
}

OrderSide CoinbaseHandler::convertOrderSide(const std::string& side) {
    if (side == "buy") {
        return OrderSide::BUY;
    } else {
        return OrderSide::SELL;
    }
}

void CoinbaseHandler::processL2Update(const nlohmann::json& j) {
    if (verbose_logging_) {
        std::cout << "Processing L2 update: " << j.dump(2) << std::endl;
    }

    // Check required fields
    if (!j.contains("product_id") || !j.contains("changes")) {
        std::cerr << "L2 update missing required fields" << std::endl;
        return;
    }

    // Get the product ID (symbol)
    std::string symbol = j["product_id"];

    // Get the order book
    auto order_book = getOrderBook(symbol);
    if (!order_book) {
        std::cerr << "Order book not found for symbol: " << symbol << std::endl;
        return;
    }

    try {
        // Process the changes
        for (const auto& change : j["changes"]) {
            // Each change is [side, price, size]
            std::string side_str = change[0];
            double price = std::stod(change[1].get<std::string>());
            double size = std::stod(change[2].get<std::string>());
            
            OrderSide side = (side_str == "buy") ? OrderSide::BUY : OrderSide::SELL;
            
            // Generate a unique order ID based on side and price
            std::string order_id = (side == OrderSide::BUY ? "bid-" : "ask-") + change[1].get<std::string>();
            
            // If size is 0, remove the price level
            if (size <= 0.0) {
                order_book->removeOrdersByPrice(price, side);
            } else {
                // Either add a new order or update existing one
                auto existing_orders = order_book->getOrdersByPrice(price, side);
                
                if (existing_orders.empty()) {
                    // Add new order
                    auto order = std::make_shared<Order>(
                        order_id, side, price, size,
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        )
                    );
                    order_book->addOrder(order);
                } else {
                    // Update existing order
                    for (auto& order : existing_orders) {
                        if (order->getOrderId() == order_id) {
                            order_book->updateOrderSize(order_id, size);
                            break;
                        }
                    }
                }
            }
        }

        if (verbose_logging_) {
            std::cout << "Processed L2 update with " << j["changes"].size() << " changes" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing L2 update: " << e.what() << std::endl;
    }
}

} // namespace clunk