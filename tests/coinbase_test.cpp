#include <iostream>
#include <string>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// Perform a simple HTTP GET request to test connectivity to Coinbase
int main() {
    try {
        // Set up the I/O context
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const host = "api-public.sandbox.exchange.coinbase.com";
        auto const port = "443";
        auto const results = resolver.resolve(host, port);

        std::cout << "Resolving " << host << ":" << port << "..." << std::endl;

        // Connect to the server
        stream.connect(results);
        std::cout << "Connected to " << host << std::endl;

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, "/products", 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        std::cout << "Sending HTTP request..." << std::endl;

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Write the message to standard out
        std::cout << "Response: " << res << std::endl;

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

        // If we get here then the connection is closed gracefully
        std::cout << "Test completed successfully!" << std::endl;
        return EXIT_SUCCESS;

    } catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}