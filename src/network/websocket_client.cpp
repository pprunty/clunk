#include "websocket_client.h"
#include <iostream>

namespace clunk {

WebSocketClient::WebSocketClient(const std::string& host, const std::string& port)
    : host_(host), port_(port), running_(false), connected_(false) {
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

void WebSocketClient::connect() {
    if (running_) {
        return;
    }

    running_ = true;
    io_thread_ = std::thread([this]() {
        // Create a new stream
        ws_ = std::make_unique<websocket::stream<tcp::socket>>(ioc_);

        // Set up the SSL context for the client
        ws_->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

        // Connect asynchronously
        asyncConnect();

        // Run the I/O context
        ioc_.run();
    });
}

void WebSocketClient::disconnect() {
    if (!running_) {
        return;
    }

    running_ = false;
    connected_ = false;

    // Close the WebSocket connection
    if (ws_) {
        beast::error_code ec;
        ws_->close(websocket::close_code::normal, ec);
        if (ec) {
            std::cerr << "Error closing WebSocket: " << ec.message() << std::endl;
        }
    }

    // Stop the I/O context
    ioc_.stop();

    // Wait for the I/O thread to finish
    if (io_thread_.joinable()) {
        io_thread_.join();
    }

    // Reset the WebSocket stream
    ws_.reset();
}

void WebSocketClient::send(const std::string& message) {
    if (!connected_) {
        std::cerr << "Not connected, cannot send message" << std::endl;
        return;
    }

    // Add message to the send queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        send_queue_.push_back(message);
    }

    // Start sending if not already sending
    if (send_queue_.size() == 1) {
        asyncSend();
    }
}

void WebSocketClient::asyncConnect() {
    // Resolve the host name
    tcp::resolver resolver(ioc_);
    auto const results = resolver.resolve(host_, port_);

    // Make the connection on the IP address we get from a lookup
    net::async_connect(
        ws_->next_layer(),
        results.begin(),
        results.end(),
        [self = shared_from_this()](beast::error_code ec, tcp::resolver::iterator) {
            if (ec) {
                return self->handleError(ec, "connect");
            }

            // Perform the WebSocket handshake
            self->ws_->async_handshake(
                self->host_,
                "/",
                [self](beast::error_code ec) {
                    if (ec) {
                        return self->handleError(ec, "handshake");
                    }

                    // Connection established
                    self->connected_ = true;
                    std::cout << "WebSocket connected to " << self->host_ << std::endl;

                    // Start reading messages
                    self->asyncRead();
                }
            );
        }
    );
}

void WebSocketClient::asyncRead() {
    // Allocate a buffer for the message
    auto buffer = std::make_shared<beast::flat_buffer>();

    // Read a message into our buffer
    ws_->async_read(
        *buffer,
        [self = shared_from_this(), buffer](beast::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                if (ec == websocket::error::closed) {
                    self->connected_ = false;
                    std::cout << "WebSocket connection closed" << std::endl;
                } else {
                    self->handleError(ec, "read");
                }
                return;
            }

            // Convert to string
            std::string message = beast::buffers_to_string(buffer->data());

            // Process the message
            self->processMessage(message);

            // Continue reading if still connected
            if (self->connected_ && self->running_) {
                self->asyncRead();
            }
        }
    );
}

void WebSocketClient::asyncSend() {
    if (!connected_ || !running_) {
        return;
    }

    std::string message;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (send_queue_.empty()) {
            return;
        }
        message = send_queue_.front();
    }

    // Send the message
    ws_->async_write(
        net::buffer(message),
        [self = shared_from_this(), message](beast::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                return self->handleError(ec, "write");
            }

            // Remove the message from the queue
            {
                std::lock_guard<std::mutex> lock(self->queue_mutex_);
                self->send_queue_.pop_front();

                // Continue sending if there are more messages
                if (!self->send_queue_.empty()) {
                    self->asyncSend();
                }
            }
        }
    );
}

void WebSocketClient::handleError(beast::error_code ec, const char* what) {
    if (ec == net::error::operation_aborted) {
        // Operation was cancelled - likely during shutdown
        return;
    }

    std::cerr << "WebSocket error in " << what << ": " << ec.message() << std::endl;

    if (connected_) {
        connected_ = false;

        // Try to reconnect after a delay
        std::thread([self = shared_from_this()]() {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (self->running_) {
                std::cout << "Attempting to reconnect..." << std::endl;
                self->asyncConnect();
            }
        }).detach();
    }
}

void WebSocketClient::processMessage(const std::string& message) {
    // Call the message callback if set
    if (message_callback_) {
        message_callback_(message);
    }
}

} // namespace clunk