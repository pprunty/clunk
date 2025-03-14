#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <deque>
#include <mutex>

namespace clunk {

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

// Callback type for receiving messages
using MessageCallback = std::function<void(const std::string&)>;

// Simple WebSocket client for connecting to exchange APIs
class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
public:
    // Constructor
    WebSocketClient(const std::string& host, const std::string& port);

    // Destructor
    virtual ~WebSocketClient();

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
    
    // Set the path for WebSocket handshake
    void setPath(const std::string& path) {
        path_ = path;
    }

    // Check if connected
    bool isConnected() const {
        return connected_;
    }

    // Enable/disable verbose logging
    void setVerboseLogging(bool enabled);

private:
    std::string host_;
    std::string port_;
    std::string path_ = "/";

    net::io_context ioc_;
    ssl::context ssl_ctx_{ssl::context::tlsv12_client};
    std::unique_ptr<ssl::stream<tcp::socket>> ssl_stream_;

    std::thread io_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;

    MessageCallback message_callback_;

    // Message queue
    std::deque<std::string> send_queue_;
    std::mutex queue_mutex_;

    // Verbose logging flag
    bool verbose_logging_;

    // Connect to the server asynchronously
    void asyncConnect();

    // Read messages from the server
    void asyncRead();

    // Send messages from the queue
    void asyncSend();

    // Handle connection failure
    void handleError(const boost::system::error_code& ec, const char* what);

    // Process received message
    void processMessage(const std::string& message);
    
    // WebSocket-specific methods
    void performWebSocketHandshake();
    void readWebSocketResponse();
    std::string generateWebSocketKey();
};

} // namespace clunk