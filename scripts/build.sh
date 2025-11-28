#!/bin/bash
# Simple build script for apvlv

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check dependencies
check_dependencies() {
    print_status "Checking dependencies..."

    if ! command -v cmake &> /dev/null; then
        print_error "cmake not found"
        exit 1
    fi

    if ! command -v make &> /dev/null; then
        print_error "make not found"
        exit 1
    fi

    print_status "Dependencies OK"
}

# Clean build directory
clean_build() {
    print_status "Cleaning build directory..."
    rm -rf build
    mkdir -p build
}

# Configure project
configure_project() {
    print_status "Configuring project..."
    cd build

    local build_type="Release"
    if [[ "$1" == "debug" ]]; then
        build_type="Debug"
    fi

    cmake .. -DCMAKE_BUILD_TYPE=$build_type
    cd ..
}

# Build project
build_project() {
    print_status "Building project..."
    cd build
    make -j$(nproc)
    cd ..
}

# Run tests
run_tests() {
    print_status "Running tests..."
    cd build
    if [[ -f "testNote" ]]; then
        ./testNote
    else
        print_warning "No tests found"
    fi
    cd ..
}

# Main script
main() {
    local build_type="release"

    while [[ $# -gt 0 ]]; do
        case $1 in
            debug)
                build_type="debug"
                shift
                ;;
            clean)
                clean_build
                exit 0
                ;;
            test)
                run_tests
                exit 0
                ;;
            help)
                echo "Usage: $0 [debug|clean|test|help]"
                echo "  debug  - Build in debug mode"
                echo "  clean  - Clean build directory"
                echo "  test   - Run tests"
                echo "  help   - Show this help"
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done

    check_dependencies
    clean_build
    configure_project $build_type
    build_project

    print_status "Build completed successfully!"
    print_status "Executable: build/apvlv"
}

main "$@"
