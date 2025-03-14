#include "websocket_client.h"
#include <iostream>
#include <openssl/err.h>
#include <string>
#include <regex>
#include <random>

namespace clunk {

WebSocketClient::WebSocketClient(const std::string& host, const std::string& port)
    : host_(host), port_(port), running_(false), connected_(false), verbose_logging_(false) {
    
    // Set up SSL context options with maximum compatibility
    ssl_ctx_.set_options(
        ssl::context::default_workarounds |
        ssl::context::no_sslv2 |
        ssl::context::no_sslv3 |
        ssl::context::tlsv12_client |
        ssl::context::tlsv13_client
    );
    
    // Disable certificate verification for testing
    ssl_ctx_.set_verify_mode(ssl::verify_none);
    
    // Enable SNI (Server Name Indication)
    SSL_CTX_set_tlsext_servername_callback(ssl_ctx_.native_handle(), nullptr);
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
        try {
            // Reset the io_context to make sure it's not in an error state
            ioc_.restart();

            // Create a new SSL stream
            ssl_stream_ = std::make_unique<ssl::stream<tcp::socket>>(ioc_, ssl_ctx_);

            // Connect asynchronously
            asyncConnect();

            // Run the I/O context
            ioc_.run();
        } catch (const std::exception& e) {
            std::cerr << "Error in connection thread: " << e.what() << std::endl;
        }
    });
}

void WebSocketClient::disconnect() {
    if (!running_) {
        return;
    }

    running_ = false;
    connected_ = false;

    // Close the SSL connection
    if (ssl_stream_) {
        boost::system::error_code ec;

        // Perform a graceful close if possible
        try {
            ssl_stream_->shutdown(ec);
            if (ec && ec != net::error::eof) {
                if (verbose_logging_) {
                    std::cerr << "Error closing SSL stream: " << ec.message() << std::endl;
                }
            }
        } catch (const std::exception& e) {
            if (verbose_logging_) {
                std::cerr << "Exception during SSL shutdown: " << e.what() << std::endl;
            }
        }
    }

    // Stop the I/O context
    try {
        ioc_.stop();
    } catch (const std::exception& e) {
        if (verbose_logging_) {
            std::cerr << "Exception stopping io_context: " << e.what() << std::endl;
        }
    }

    // Wait for the I/O thread to finish
    if (io_thread_.joinable()) {
        io_thread_.join();
    }

    // Reset the SSL stream
    ssl_stream_.reset();
}

void WebSocketClient::send(const std::string& message) {
    if (!connected_) {
        if (verbose_logging_) {
            std::cerr << "Not connected, cannot send message" << std::endl;
        }
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

void WebSocketClient::setVerboseLogging(bool enabled) {
    verbose_logging_ = enabled;
}

void WebSocketClient::asyncConnect() {
    if (verbose_logging_) {
        std::cout << "Resolving hostname: " << host_ << ":" << port_ << std::endl;
    }

    // Resolve the host name
    tcp::resolver resolver(ioc_);

    // Using sync resolve for simplicity, but could be made async
    boost::system::error_code ec;
    auto const results = resolver.resolve(host_, port_, ec);

    if (ec) {
        std::cerr << "Error resolving " << host_ << ":" << port_ << " - " << ec.message() << std::endl;
        return handleError(ec, "resolve");
    }

    if (verbose_logging_) {
        std::cout << "Connecting to " << results.begin()->endpoint() << std::endl;
    }

    // Make the TCP connection
    net::async_connect(
        ssl_stream_->lowest_layer(),
        results.begin(),
        results.end(),
        [self = shared_from_this()](boost::system::error_code ec, tcp::resolver::iterator) {
            if (ec) {
                std::cerr << "TCP connection failed: " << ec.message() << std::endl;
                return self->handleError(ec, "connect");
            }

            if (self->verbose_logging_) {
                std::cout << "TCP connected, performing SSL handshake" << std::endl;
                std::cout << "Using TLS version: " << SSL_get_version(self->ssl_stream_->native_handle()) << std::endl;
            }
            
            // Set SNI hostname (very important for some servers)
            if (!SSL_set_tlsext_host_name(self->ssl_stream_->native_handle(), self->host_.c_str())) {
                std::cerr << "Failed to set SNI hostname" << std::endl;
            }
            
            // Perform SSL handshake
            self->ssl_stream_->async_handshake(
                ssl::stream_base::client,
                [self](boost::system::error_code ec) {
                    if (ec) {
                        std::cerr << "SSL handshake failed: " << ec.message() << std::endl;
                        if (self->verbose_logging_) {
                            std::cerr << "SSL error code: " << ERR_get_error() << std::endl;
                            std::cerr << "SSL error string: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
                        }
                        return self->handleError(ec, "ssl_handshake");
                    }

                    if (self->verbose_logging_) {
                        std::cout << "SSL handshake complete, performing WebSocket handshake" << std::endl;
                        std::cout << "Using cipher: " << SSL_get_cipher(self->ssl_stream_->native_handle()) << std::endl;
                    }

                    // Perform WebSocket handshake
                    self->performWebSocketHandshake();
                }
            );
        }
    );
}

void WebSocketClient::performWebSocketHandshake() {
    // Generate a random WebSocket key
    std::string key = generateWebSocketKey();
    
    // Construct the WebSocket upgrade request
    std::string request = 
        "GET " + path_ + " HTTP/1.1\r\n"
        "Host: " + host_ + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: " + key + "\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36\r\n"
        "Accept: */*\r\n"
        "Accept-Language: en-US,en;q=0.9\r\n"
        "Cache-Control: no-cache\r\n"
        "Pragma: no-cache\r\n"
        "Origin: https://pro.coinbase.com\r\n"
        "Sec-WebSocket-Protocol: json\r\n\r\n";

    // Send the request
    net::async_write(
        *ssl_stream_,
        net::buffer(request),
        [self = shared_from_this()](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
            if (ec) {
                std::cerr << "Failed to send WebSocket upgrade request: " << ec.message() << std::endl;
                return self->handleError(ec, "websocket_request");
            }

            // Read the response
            self->readWebSocketResponse();
        }
    );
}

void WebSocketClient::readWebSocketResponse() {
    auto buffer = std::make_shared<std::vector<char>>(2048);
    
    ssl_stream_->async_read_some(
        net::buffer(*buffer),
        [self = shared_from_this(), buffer](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                std::cerr << "Failed to read WebSocket response: " << ec.message() << std::endl;
                return self->handleError(ec, "websocket_response");
            }

            std::string response(buffer->data(), bytes_transferred);
            if (self->verbose_logging_) {
                std::cout << "WebSocket response: " << response << std::endl;
            }

            // Very basic check for successful handshake
            if (response.find("HTTP/1.1 101") != std::string::npos && 
                response.find("Upgrade: websocket") != std::string::npos) {
                self->connected_ = true;
                std::cout << "WebSocket connected to " << self->host_ << self->path_ << std::endl;

                // Start reading messages
                self->asyncRead();
            } else {
                std::cerr << "WebSocket handshake failed. Response: " << response << std::endl;
                boost::system::error_code err = boost::system::errc::make_error_code(boost::system::errc::protocol_error);
                self->handleError(err, "websocket_handshake");
            }
        }
    );
}

std::string WebSocketClient::generateWebSocketKey() {
    // Generate 16 random bytes
    std::vector<uint8_t> random_bytes(16);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : random_bytes) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    // Base64 encode the random bytes
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string encoded;
    encoded.reserve(24); // Base64 encoding of 16 bytes requires 24 characters (without padding)
    
    for (size_t i = 0; i < random_bytes.size(); i += 3) {
        uint32_t triplet = 0;
        for (size_t j = 0; j < 3 && i + j < random_bytes.size(); ++j) {
            triplet |= static_cast<uint32_t>(random_bytes[i + j]) << (16 - j * 8);
        }
        
        encoded.push_back(base64_chars[(triplet >> 18) & 0x3F]);
        encoded.push_back(base64_chars[(triplet >> 12) & 0x3F]);
        
        if (i + 1 < random_bytes.size()) {
            encoded.push_back(base64_chars[(triplet >> 6) & 0x3F]);
        } else {
            encoded.push_back('=');
        }
        
        if (i + 2 < random_bytes.size()) {
            encoded.push_back(base64_chars[triplet & 0x3F]);
        } else {
            encoded.push_back('=');
        }
    }
    
    return encoded;
}

void WebSocketClient::asyncRead() {
    if (!running_ || !connected_) {
        return;
    }

    // Allocate a buffer for the message
    auto buffer = std::make_shared<std::vector<char>>(16384);

    // Read a message into our buffer
    ssl_stream_->async_read_some(
        net::buffer(*buffer),
        [self = shared_from_this(), buffer](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                if (ec == net::error::eof) {
                    self->connected_ = false;
                    if (self->verbose_logging_) {
                        std::cout << "WebSocket connection closed" << std::endl;
                    }
                } else {
                    self->handleError(ec, "read");
                }
                return;
            }

            // Convert to string
            std::string message(buffer->data(), bytes_transferred);

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

    // WebSocket framing - creating a simple frame for text data
    std::vector<uint8_t> frame;
    
    // First byte: FIN bit (1) + opcode (1 for text)
    frame.push_back(0x81);
    
    // Second byte onwards: payload length + masking
    size_t payload_length = message.size();
    
    if (payload_length < 126) {
        // Mask bit (1) + 7-bit payload length
        frame.push_back(static_cast<uint8_t>(0x80 | payload_length));
    } else if (payload_length <= 0xFFFF) {
        // Mask bit (1) + 126
        frame.push_back(0x80 | 126);
        // 16-bit length
        frame.push_back(static_cast<uint8_t>((payload_length >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(payload_length & 0xFF));
    } else {
        // Mask bit (1) + 127
        frame.push_back(0x80 | 127);
        // 64-bit length
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<uint8_t>((payload_length >> (i * 8)) & 0xFF));
        }
    }
    
    // Masking key (4 bytes)
    std::array<uint8_t, 4> mask_key = {0x12, 0x34, 0x56, 0x78}; // Fixed for simplicity
    for (uint8_t byte : mask_key) {
        frame.push_back(byte);
    }
    
    // Apply masking to payload
    for (size_t i = 0; i < message.size(); ++i) {
        frame.push_back(message[i] ^ mask_key[i % 4]);
    }
    
    // Send the WebSocket frame
    net::async_write(
        *ssl_stream_,
        net::buffer(frame),
        [self = shared_from_this(), message](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
            if (ec) {
                self->handleError(ec, "write");
                return;
            }

            if (self->verbose_logging_) {
                std::cout << "Sent: " << message << std::endl;
            }

            // Remove the sent message and send the next one if available
            {
                std::lock_guard<std::mutex> lock(self->queue_mutex_);
                self->send_queue_.pop_front();
                
                if (!self->send_queue_.empty()) {
                    // Send the next message
                    self->asyncSend();
                }
            }
        }
    );
}

void WebSocketClient::handleError(const boost::system::error_code& ec, const char* what) {
    std::cerr << "Error in " << what << ": " << ec.message() << std::endl;
    
    if (connected_) {
        connected_ = false;
        std::cerr << "Connection lost" << std::endl;
    }
}

void WebSocketClient::processMessage(const std::string& message) {
    // Simplified WebSocket frame parsing
    if (message.empty()) {
        return;
    }
    
    // Extract payload from WebSocket frame - improved implementation
    size_t header_size = 2;
    uint8_t first_byte = static_cast<uint8_t>(message[0]);
    uint8_t second_byte = static_cast<uint8_t>(message[1]);
    bool is_masked = second_byte & 0x80;
    uint8_t payload_len = second_byte & 0x7F;
    size_t real_payload_len = 0;
    
    if (payload_len == 126) {
        // 16-bit length
        if (message.size() < 4) {
            if (verbose_logging_) {
                std::cerr << "Incomplete WebSocket frame" << std::endl;
            }
            return;
        }
        header_size += 2;
        real_payload_len = (static_cast<uint16_t>(static_cast<uint8_t>(message[2])) << 8) | 
                           static_cast<uint16_t>(static_cast<uint8_t>(message[3]));
    } else if (payload_len == 127) {
        // 64-bit length (we only use the lowest 32 bits)
        if (message.size() < 10) {
            if (verbose_logging_) {
                std::cerr << "Incomplete WebSocket frame" << std::endl;
            }
            return;
        }
        header_size += 8;
        real_payload_len = 0;
        for (size_t i = 0; i < 8; ++i) {
            real_payload_len = (real_payload_len << 8) | static_cast<uint8_t>(message[2 + i]);
        }
    } else {
        real_payload_len = payload_len;
    }
    
    // If masked, we need to account for the 4-byte mask
    if (is_masked) {
        header_size += 4;
    }
    
    if (message.size() < header_size + real_payload_len) {
        if (verbose_logging_) {
            std::cerr << "Incomplete WebSocket frame: expected " << (header_size + real_payload_len) 
                  << " bytes, got " << message.size() << " bytes" << std::endl;
        }
        return;
    }
    
    // Extract and process the payload
    std::string payload;
    
    if (is_masked) {
        // Apply masking if needed
        std::array<uint8_t, 4> mask;
        for (size_t i = 0; i < 4; ++i) {
            mask[i] = static_cast<uint8_t>(message[header_size - 4 + i]);
        }
        
        payload.resize(real_payload_len);
        for (size_t i = 0; i < real_payload_len; ++i) {
            if (header_size + i < message.size()) {
                payload[i] = message[header_size + i] ^ mask[i % 4];
            } else {
                if (verbose_logging_) {
                    std::cerr << "Frame boundary error in masked payload" << std::endl;
                }
                return;
            }
        }
    } else {
        // No masking
        if (header_size + real_payload_len <= message.size()) {
            payload = message.substr(header_size, real_payload_len);
        } else {
            if (verbose_logging_) {
                std::cerr << "Frame boundary error in unmasked payload" << std::endl;
            }
            return;
        }
    }
    
    // Check if this is a control frame (opcode 0x8, 0x9, or 0xA)
    uint8_t opcode = first_byte & 0x0F;
    if (opcode == 0x8) {
        // Close frame
        if (verbose_logging_) {
            std::cout << "Received WebSocket close frame" << std::endl;
        }
        connected_ = false;
        return;
    } else if (opcode == 0x9) {
        // Ping frame, respond with pong
        if (verbose_logging_) {
            std::cout << "Received WebSocket ping" << std::endl;
        }
        // Send pong with the same payload
        std::vector<uint8_t> pong_frame;
        pong_frame.push_back(0x8A); // FIN bit + Pong opcode
        
        if (payload.size() < 126) {
            pong_frame.push_back(static_cast<uint8_t>(payload.size()));
        } else if (payload.size() <= 0xFFFF) {
            pong_frame.push_back(126);
            pong_frame.push_back(static_cast<uint8_t>((payload.size() >> 8) & 0xFF));
            pong_frame.push_back(static_cast<uint8_t>(payload.size() & 0xFF));
        }
        
        for (auto c : payload) {
            pong_frame.push_back(static_cast<uint8_t>(c));
        }
        
        net::async_write(
            *ssl_stream_,
            net::buffer(pong_frame),
            [](boost::system::error_code, std::size_t) {}
        );
        return;
    } else if (opcode == 0xA) {
        // Pong frame
        if (verbose_logging_) {
            std::cout << "Received WebSocket pong" << std::endl;
        }
        return;
    }
    
    // Process the text or binary payload
    if (message_callback_ && !payload.empty()) {
        try {
            if (verbose_logging_) {
                std::cout << "Received payload (" << payload.size() << " bytes): " 
                      << (payload.size() > 100 ? payload.substr(0, 100) + "..." : payload) << std::endl;
            }
            message_callback_(payload);
        } catch (const std::exception& e) {
            std::cerr << "Error in message callback: " << e.what() << std::endl;
        }
    }
}

} // namespace clunk