# Clunk (C++ Low-Latency Unified Networked Orderbook Kit)


Objective:

Create a high-performance order book system that can process L3 market data feeds, with a focus on:

- Microsecond-level latency
- Memory efficiency
- Exchange-specific optimizations
- Real-time visualization

do not implement a full exchange, but rather a system that can process L3 market data feeds from multiple exchanges.

connect to Coinbase sandbox and provide a simple order book visualization with relevant benchmarks for each section of the code.
You can write a connection to coinbase's L3 market data feed. If you can properly handle an L3 FIX connection and save the data for multiple markets into an efficient orderbook and present benchmarks for every relevant section of your code that'll be a great thing to show off.

## Commands

make build - to build the project
make run - to run with default symbol (BTC-USD)
make run-sym SYM=ETH-USD - to run with a specific symbol
make test - to run the tests
make benchmark - to run the benchmarks