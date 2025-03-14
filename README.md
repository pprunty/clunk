# Clunk: High-Frequency Trading Order Book Visualizer

<div align="center">
  <img src="https://raw.githubusercontent.com/pprunty/clunk/main/docs/logo.png" alt="Clunk Logo" width="300"/>
  <h3>C++ Low-Latency Unified Networked Orderbook Kit</h3>
  <p>Microsecond-level order book processing for professional quantitative trading</p>
</div>

## Overview

Clunk is a high-performance order book visualization system designed for quantitative trading applications. Built with C++ for optimal performance, Clunk connects to exchange market data feeds and provides real-time analysis with professional trading metrics.

### Key Features

- **Low-Latency Processing**: Built with C++ for microsecond-level processing times
- **Real-Time Market Data**: Direct connection to Coinbase exchange market data feeds
- **Advanced Market Microstructure Metrics**: 
  - Order book imbalance analysis
  - Market pressure indicators 
  - VWAP (Volume-Weighted Average Price)
  - Liquidity depth measurements
  - Price impact estimation
  - Spread analytics (absolute and basis points)
- **Performance Analytics**: 
  - Update frequency metrics
  - Processing latency measurements
- **Visual Change Detection**: Color-coded highlighting of price and size changes for tick-by-tick visualization
- **Cross-Platform**: Runs on macOS, Linux, and Windows

## Screenshots

![Clunk Order Book Visualization](https://raw.githubusercontent.com/pprunty/clunk/main/docs/screenshot.png)

## Market Metrics Explained

For quantitative traders, Clunk provides essential metrics to inform trading decisions:

| Metric | Description | Trading Significance |
|--------|-------------|----------------------|
| Order Book Imbalance | Ratio of bid volume to ask volume | Values above 1.0 indicate higher buying pressure; useful for predicting short-term price movements |
| Market Pressure | Normalized (-1.0 to 1.0) representation of order book imbalance | Positive values indicate buying pressure; negative values indicate selling pressure |
| Spread (bps) | Bid-ask spread in basis points (1bp = 0.01%) | Key liquidity indicator and cost metric for market makers |
| Liquidity Depth | Available volume within 0.5% of best bid/ask | Provides insight into market depth and potential slippage |
| Price Impact | Estimated price movement from a 1% market order | Useful for determining optimal order sizes and estimating execution cost |
| VWAP | Volume-Weighted Average Price for bids and asks | Important benchmark for algorithmic trading strategies |

## Quickstart Guide

### Installation Requirements

- C++17 compatible compiler
- CMake 3.15 or higher
- vcpkg package manager
- Boost libraries (installed via vcpkg)
- nlohmann_json (installed via vcpkg)
- OpenSSL (installed via vcpkg)

### Clone and Build

```bash
# Clone the repository
git clone https://github.com/pprunty/clunk.git
cd clunk

# Initialize vcpkg (if needed)
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh

# Build the project
make build

# Run with default symbol (BTC-USD)
make run

# Run with a specific symbol
make run-sym SYM=ETH-USD
```

### Command-Line Options

```
Usage: clunk [OPTIONS]

Options:
  -h, --help                 Display this help message
  -s, --symbol SYMBOL        Trading symbol (default: BTC-USD)
  -d, --depth DEPTH          Order book depth to display (default: 10)
  -r, --refresh RATE         Refresh rate in milliseconds (default: 500)
  -v, --verbose              Enable verbose output
  -c, --no-color-changes     Disable highlighting of price/size changes
  -t, --highlight-time TIME  Duration to highlight changes (default: 2 refreshes)
```

### Examples

```bash
# Run with ETH-BTC symbol and 15 levels of depth
./build/clunk -s ETH-BTC -d 15

# Run with faster refresh rate (200ms)
./build/clunk -s BTC-USD -r 200

# Run with more verbose logging
./build/clunk -s BTC-USD -v
```

## Technical Architecture

Clunk is designed with a focus on performance and modularity:

- **Feed Handlers**: Extensible system for connecting to different exchange APIs
- **Order Book**: High-performance price-time priority order book implementation
- **Visualization**: Real-time console-based visualization with microsecond-level metrics

### Performance Benchmarks

| Metric | Value |
|--------|-------|
| Order Processing Latency | <100 μs (average) |
| Order Book Update Rate | >10,000 updates/second |
| Memory Footprint | <50MB for standard market depth |

## For Quantitative Trading Teams

Clunk was designed with professional quantitative trading applications in mind. Key applications include:

- **Market Microstructure Analysis**: Study order flow patterns and market dynamics
- **Algorithm Development**: Test and visualize trading strategies in real-time
- **Liquidity Analysis**: Assess market depth and available liquidity
- **Trading Signal Generation**: Use order book imbalance and market pressure as signals
- **Execution Quality Analysis**: Evaluate trading costs and market impact

## Commands

- `make build` - Build the project
- `make run` - Run with default symbol (BTC-USD)
- `make run-sym SYM=ETH-USD` - Run with a specific symbol
- `make test` - Run the tests
- `make benchmark` - Run the benchmarks
- `make clean` - Clean the build directory

## License

MIT License

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.