# Makefile for clunk project

TARGET = clunk
BUILD_DIR = build
CXXFLAGS = -O3 -Wall -std=c++17
VERSION = 0.1.0  # Sync this with project(clunk VERSION x.y.z) or parse from CMake if desired

# Path to the vcpkg toolchain file (adjust if your vcpkg folder is elsewhere).
TOOLCHAIN_FILE = $(shell pwd)/vcpkg/scripts/buildsystems/vcpkg.cmake

# Default target
.DEFAULT_GOAL := help

# Print help message
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "                | |           | |"
	@echo "             ___| |_   _ _ __ | | __"
	@echo "            / __| | | | | '_ \\| |/ /"
	@echo "           | (__| | |_| | | | |   < "
	@echo "            \\___|_|\\__,_|_| |_|_|\\_\\"
	@echo ""
	@echo "Targets:"
	@echo "  build         - Configure and build the project (using vcpkg toolchain)"
	@echo "  clean         - Remove build artifacts"
	@echo "  test          - Run tests using ctest"
	@echo "  benchmark     - Run benchmarks"
	@echo "  install       - Install the project to BUILD_DIR/install"
	@echo "  install-sudo  - Install the project to system directories (requires sudo)"
	@echo "  run           - Build and run the executable with default symbol (BTC-USD)"
	@echo "  run-sym       - Build and run with a specific symbol (e.g., make run-sym SYM=ETH-USD)"
	@echo "  coinbase-test - Build and run a simple connectivity test to Coinbase sandbox"
	@echo "  help          - Show this help message (default target)"

# Build the project
build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. \
		-DCMAKE_CXX_FLAGS="$(CXXFLAGS)" \
		-DCMAKE_TOOLCHAIN_FILE="$(TOOLCHAIN_FILE)"
	cd $(BUILD_DIR) && cmake --build .

# Clean build directory
clean:
	rm -rf $(BUILD_DIR)

# Run tests
test: build
	cd $(BUILD_DIR) && ctest --output-on-failure

# Run benchmarks
benchmark: build
	cd $(BUILD_DIR) && ./benchmarks/clunk_benchmarks

# Install the project to the build directory
install: build
	cd $(BUILD_DIR) && cmake --install .

# Install the project to system directories (requires sudo)
install-sudo: build
	cd $(BUILD_DIR) && sudo cmake --install . --prefix /usr/local

# Run the program with default symbol
run: build
	cd $(BUILD_DIR) && ./$(TARGET)

# Run with specific symbol
run-sym: build
	cd $(BUILD_DIR) && ./$(TARGET) $(SYM)

# Build and run the Coinbase connectivity test
coinbase-test:
	mkdir -p $(BUILD_DIR)
	g++ -o $(BUILD_DIR)/coinbase_test src/coinbase_test.cpp -std=c++17 -I$(BUILD_DIR)/vcpkg_installed/arm64-osx/include -L$(BUILD_DIR)/vcpkg_installed/arm64-osx/lib -lboost_system
	$(BUILD_DIR)/coinbase_test

.PHONY: build clean test benchmark install install-sudo run run-sym coinbase-test help