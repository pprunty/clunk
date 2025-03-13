#pragma once

#include <string>
#include <functional>
#include <memory>

namespace clunk {

// Abstract base class for feed handlers
class FeedHandler {
public:
    // Constructor
    FeedHandler() = default;

    // Destructor
    virtual ~FeedHandler() = default;

    // Connect to the feed
    virtual void connect() = 0;

    // Disconnect from the feed
    virtual void disconnect() = 0;

    // Subscribe to a symbol
    virtual void subscribe(const std::string& symbol) = 0;

    // Unsubscribe from a symbol
    virtual void unsubscribe(const std::string& symbol) = 0;

    // Check if connected
    virtual bool isConnected() const = 0;
};

} // namespace clunk