# Clunk (C++ Low-Latency Unified Networked Orderbook Kit)


Objective:

Create a high-performance order book system that can process L3 market data feeds, with a focus on:

- Microsecond-level latency
- Memory efficiency
- Exchange-specific optimizations
- Real-time visualization

do not implement a full exchange, but rather a system that can process L3 market data feeds from multiple exchanges.

connect to Coinbase sandbox and provide a simple order book visualization with relevant benchmarks for each section of the code.

---




Real-time visualization

NUMA-aware memory allocation
tiny pointers
c++ json
GPU offloading (CUDA) for:
Historical data replay
Monte Carlo simulations
RDMA support for network layer

git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # or bootstrap-vcpkg.bat on Windows
./vcpkg integrate install

---

Networking:

QuickFIX 
Boost.Beast (Websocket & HTTP)

Data Parsing & Serialization:
nlohmann/json or RapidJSON
FlatBuffers / Cap’n Proto (Optional, advanced)

Useful if you want a lower-latency, schema-based approach for internal data serialization.
Could be overkill for a first pass, but it demonstrates knowledge of zero-copy or near-zero-copy protocols.

Concurrency & Data Structures

You can write a connection to coinbase's L3 market data feed. If you can properly handle an L3 FIX connection and save the data for multiple markets into an efficient orderbook and present benchmarks for every relevant section of your code that'll be a great thing to show off.