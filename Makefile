# Makefile for clunk project

TARGET = clunk
BUILD_DIR = build
CXXFLAGS = -O3 -Wall
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
	@echo "  install       - Install the project (requires sudo)"
	@echo "  run           - Build and run the executable"
	@echo "  publish-brew  - Example step to prepare/publish a new version to Homebrew"
	@echo "  help          - Show this help message (default target)"

# Build the project
build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. \
	    -DCMAKE_CXX_FLAGS="$(CXXFLAGS)" \
	    -DCMAKE_TOOLCHAIN_FILE="$(TOOLCHAIN_FILE)"
	cd $(BUILD_DIR) && $(MAKE)

# Clean build directory
clean:
	rm -rf $(BUILD_DIR)

# Run tests
test:
	cd $(BUILD_DIR) && ctest --output-on-failure

# Run the program
run: build
	cd $(BUILD_DIR) && ./$(TARGET)

# Publish (or update) the Homebrew formula
publish-brew:
	@echo "Preparing to publish version $(VERSION) to Homebrew..."
	@echo "1) Make sure you have a Git tag for version $(VERSION)."
	@echo "2) Create a tar.gz or zip archive of the source code."
	@echo "3) Calculate the SHA-256 of that archive using 'shasum -a 256 <file>'."
	@echo "4) Update Formula/clunk.rb to reflect the new URL and SHA256."
	@echo "5) Commit and push changes to your Homebrew tap (or local usage)."
	@echo "   brew install Formula/clunk.rb     # to test locally"
	@echo "Done! You may still need to 'brew tap <user>/<repo>' if it's a custom tap."

.PHONY: build clean test run help publish-brew
