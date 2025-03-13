#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <functional>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <deque>
#include <mutex>

namespace clunk {

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// Callback type for receiving messages
using MessageCallback = std::function<void(const std::string&)>;

// WebSocket client for connecting to exchange APIs
class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
public:
    // Constructor
    WebSocketClient(const std::string& host, const std::string& port);

    // Destructor
    ~WebSocketClient();

    // Connect to the server
    void connect();

    // Disconnect from the server
    void disconnect();

    // Send a message
    void send(const std::string& message);

    // Set the message callback
    void setMessageCallback(MessageCallback callback) {
        message_callback_ = callback;
    }

    // Check if connected
    bool isConnected() const {
        return connected_;
    }

private:
    std::string host_;
    std::string port_;

    net::io_context ioc_;
    std::unique_ptr<websocket::stream<tcp::socket>> ws_;

    std::thread io_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;

    MessageCallback message_callback_;

    // Message queue
    std::deque<std::string> send_queue_;
    std::mutex queue_mutex_;

    // Connect to the server asynchronously
    void asyncConnect();

    // Read messages from the server
    void asyncRead();

    // Send messages from the queue
    void asyncSend();

    // Handle connection failure
    void handleError(beast::error_code ec, const char* what);

    // Process received message
    void processMessage(const std::string& message);
};

} // namespace clunk