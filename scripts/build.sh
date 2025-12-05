#!/bin/bash
# Build script for apvlv using bash
# Usage: ./build.sh [debug clean test package deps help] [build_path]

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

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$(dirname "$SCRIPT_DIR")"

cleanup_vcpkg_locks() {
    if [ -n "$VCPKG_ROOT" ] && [ -d "$VCPKG_ROOT" ]; then
        print_status "Cleaning up vcpkg lock files..."
        rm -f "$VCPKG_ROOT/.vcpkg-root.lock" 2>/dev/null || true
        rm -f "$VCPKG_ROOT/vcpkg.lock" 2>/dev/null || true
        if [ -n "$VCPKG_DOWNLOADS" ] && [ -d "$VCPKG_DOWNLOADS" ]; then
            rm -f "$VCPKG_DOWNLOADS/.lock" 2>/dev/null || true
        fi
    fi
}

invoke_with_retry() {
    local max_attempts=5
    local delay_seconds=10
    local attempt=0
    local cmd="$1"
    
    while [ $attempt -lt $max_attempts ]; do
        attempt=$((attempt + 1))
        
        if eval "$cmd" 2>&1; then
            return 0
        fi
        
        print_warning "Attempt $attempt failed."
        
        if [ $attempt -lt $max_attempts ]; then
            print_status "Retrying in $delay_seconds seconds..."
            sleep $delay_seconds
        fi
    done
    
    print_error "Operation failed after $max_attempts attempts."
    return 1
}

invoke_vcpkg_with_retry() {
    local max_attempts=5
    local delay_seconds=15
    local attempt=0
    local cmd="$1"
    local lock_file="$VCPKG_ROOT/.vcpkg-root"
    
    while [ $attempt -lt $max_attempts ]; do
        attempt=$((attempt + 1))
        
        if [ -f "$lock_file.lock" ]; then
            print_warning "vcpkg appears to be running (lock file exists), waiting..."
            sleep 10
            if [ -f "$lock_file.lock" ] && [ $attempt -gt 2 ]; then
                print_warning "Removing potentially stale lock file..."
                rm -f "$lock_file.lock" 2>/dev/null || true
            fi
            continue
        fi
        
        if [ $attempt -gt 1 ]; then
            sleep 5
        fi
        
        if eval "$cmd" 2>&1; then
            return 0
        fi
        
        print_warning "vcpkg attempt $attempt failed."
        
        rm -f "$lock_file.lock" 2>/dev/null || true
        rm -f "$VCPKG_ROOT/vcpkg.lock" 2>/dev/null || true
        
        if [ $attempt -eq $((max_attempts - 1)) ]; then
            print_warning "Attempting aggressive lock cleanup before final retry..."
            cleanup_vcpkg_locks
            sleep 20
        fi
        
        if [ $attempt -lt $max_attempts ]; then
            print_status "Waiting $delay_seconds seconds before retry..."
            sleep $delay_seconds
        fi
    done
    
    print_error "vcpkg operation failed after $max_attempts attempts."
    return 1
}

detect_system_qt() {
    SYSTEM_QT_FOUND=0
    SYSTEM_QT_PREFIX=""

    if [ "${USE_VCPKG_QT:-0}" = "1" ]; then
        print_status "USE_VCPKG_QT=1 set, skipping system Qt detection"
        return
    fi

    print_status "Checking for system Qt6 installation..."

    if command -v qmake6 &> /dev/null; then
        SYSTEM_QT_PREFIX=$(qmake6 -query QT_INSTALL_PREFIX 2>/dev/null)
        if [ -n "$SYSTEM_QT_PREFIX" ] && [ -d "$SYSTEM_QT_PREFIX" ]; then
            print_status "Found system Qt6 via qmake6: $SYSTEM_QT_PREFIX"
            SYSTEM_QT_FOUND=1
            return
        fi
    fi

    if command -v qmake &> /dev/null; then
        local qt_version=$(qmake -query QT_VERSION 2>/dev/null)
        if [[ "$qt_version" == 6.* ]]; then
            SYSTEM_QT_PREFIX=$(qmake -query QT_INSTALL_PREFIX 2>/dev/null)
            if [ -n "$SYSTEM_QT_PREFIX" ] && [ -d "$SYSTEM_QT_PREFIX" ]; then
                print_status "Found system Qt6 via qmake: $SYSTEM_QT_PREFIX"
                SYSTEM_QT_FOUND=1
                return
            fi
        fi
    fi

    if command -v pkg-config &> /dev/null; then
        if pkg-config --exists Qt6Core 2>/dev/null; then
            local qt6_libdir=$(pkg-config --variable=libdir Qt6Core 2>/dev/null)
            if [ -n "$qt6_libdir" ]; then
                SYSTEM_QT_PREFIX=$(dirname "$qt6_libdir")
                print_status "Found system Qt6 via pkg-config: $SYSTEM_QT_PREFIX"
                SYSTEM_QT_FOUND=1
                return
            fi
        fi
    fi

    local common_qt_paths=(
        "/usr/lib/qt6"
        "/usr/lib64/qt6"
        "/usr/share/qt6"
        "/usr"
        "/usr/local"
    )

    for qt_path in "${common_qt_paths[@]}"; do
        if [ -f "$qt_path/lib/cmake/Qt6/Qt6Config.cmake" ] || \
           [ -f "$qt_path/lib64/cmake/Qt6/Qt6Config.cmake" ] || \
           [ -f "/usr/lib/cmake/Qt6/Qt6Config.cmake" ]; then
            SYSTEM_QT_PREFIX="$qt_path"
            print_status "Found system Qt6 in: $SYSTEM_QT_PREFIX"
            SYSTEM_QT_FOUND=1
            return
        fi
    done

    if [ -f "/usr/lib/cmake/Qt6/Qt6Config.cmake" ] || \
       [ -f "/usr/lib64/cmake/Qt6/Qt6Config.cmake" ]; then
        SYSTEM_QT_PREFIX="/usr"
        print_status "Found system Qt6 CMake config in /usr"
        SYSTEM_QT_FOUND=1
        return
    fi

    print_warning "System Qt6 not found, will use vcpkg Qt"
}

check_system_qt_deps() {
    if ! command -v pkg-config &> /dev/null; then
        print_warning "pkg-config not found; cannot validate system Qt dependencies."
        return
    fi

    local missing=0

    if ! pkg-config --exists Qt6Core 2>/dev/null; then
        print_warning "Missing system Qt6 pkg-config entry (Qt6Core)."
        missing=1
    fi

    if ! pkg-config --exists quazip1-qt6 2>/dev/null; then
        print_warning "Missing system QuaZIP for Qt6 (pkg-config: quazip1-qt6)."
        missing=1
    fi

    if ! pkg-config --exists libcmark 2>/dev/null; then
        print_warning "Missing system cmark (pkg-config: libcmark)."
        missing=1
    fi

    if [ "$missing" = "1" ]; then
        print_warning "System Qt mode selected, but some required system packages are missing."
        print_warning "Please install distro packages providing: Qt6, quazip-qt6, cmark."
        print_warning "Or force vcpkg Qt by setting USE_VCPKG_QT=1."
    fi
}

execute_build() {
    case "${ACTION,,}" in
        "clean")
            print_status "Cleaning build directory..."
            clean_build
            exit 0
            ;;
        "test")
            print_status "Running tests..."
            run_tests
            exit 0
            ;;
        "deps")
            print_status "Setting up vcpkg and installing dependencies..."
            ensure_vcpkg_and_deps
            print_status "Dependencies installed successfully."
            exit 0
            ;;
    esac

    check_tools

    print_status "Ensuring vcpkg and dependencies are installed..."
    ensure_vcpkg_and_deps

    print_status "Creating build directory if it doesn't exist: $BUILD_DIR"
    mkdir -p "$BUILD_DIR" || print_warning "Failed to create build directory, continuing..."

    clean_build
    configure

    print_status "Building $BUILD_TYPE version..."
    build

    if [ "${ACTION,,}" = "package" ]; then
        print_status "Creating package..."
        create_package
    fi

    find_executable

    print_status "Build completed successfully!"
}

execute_in_build_dir() {
    local cmd="$1"
    cd "$BUILD_DIR"
    if ! eval "$cmd"; then
        print_error "Command failed: $cmd"
        exit 1
    fi
    cd "$SRC_DIR"
}

execute_cmake_command() {
    local cmake_args=("$@")
    if [ "$SINGLE_CONFIG" = "false" ]; then
        cmake_args+=("--config" "$BUILD_TYPE")
    fi

    print_status "Running CMake command"
    if [ ! -d "$BUILD_DIR" ]; then
        print_error "Build directory not found: $BUILD_DIR"
        exit 1
    fi

    if ! ( cd "$BUILD_DIR" && cmake "${cmake_args[@]}" ); then
        print_error "CMake command failed"
        exit 1
    fi
}

find_and_run_test() {
    find_file_in_configs "testNote" "$BUILD_DIR"
    if [ "$FILE_FOUND" = "1" ]; then
        for config in "$BUILD_TYPE" "Release" "Debug"; do
            if [ -f "$BUILD_DIR/$config/testNote" ]; then
                "$BUILD_DIR/$config/testNote"
                if [ $? -ne 0 ]; then
                    print_error "Test execution failed."
                    exit 1
                fi
                return
            fi
        done
        if [ -f "$BUILD_DIR/testNote" ]; then
            "$BUILD_DIR/testNote"
            if [ $? -ne 0 ]; then
                print_error "Test execution failed."
                exit 1
            fi
        fi
    fi
}

ensure_vcpkg_and_deps() {
    setup_vcpkg
    install_dependencies
}

check_tools() {
    if ! command -v git &> /dev/null; then
        print_error "Git not found."
        exit 1
    fi
    
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found."
        exit 1
    fi
}

setup_vcpkg() {
    git config --global --add safe.directory "$VCPKG_ROOT" 2>/dev/null || true

    local VCPKG_CLONE_URL="https://github.com/microsoft/vcpkg"

    if [ ! -d "$VCPKG_ROOT/.git" ]; then
        print_status "Cloning vcpkg..."
        echo
        if ! invoke_with_retry "git clone --depth 1 '$VCPKG_CLONE_URL' '$VCPKG_ROOT'"; then
            print_error "Failed to clone vcpkg after multiple attempts."
            exit 1
        fi
    else
        print_status "Updating vcpkg..."
        echo
        if ! invoke_with_retry "git -C '$VCPKG_ROOT' pull --ff-only"; then
            print_error "Failed to update vcpkg after multiple attempts."
            echo
            exit 1
        fi
    fi

    if [ ! -f "$VCPKG_ROOT/vcpkg" ]; then
        print_status "Bootstrapping vcpkg..."
        echo
        
        cleanup_vcpkg_locks
        
        if ! invoke_vcpkg_with_retry "cd '$VCPKG_ROOT' && ./bootstrap-vcpkg.sh"; then
            print_error "Failed to bootstrap vcpkg after multiple attempts."
            exit 1
        fi
    fi

    if [ ! -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then
        print_error "vcpkg toolchain file not found."
        exit 1
    fi
}



install_dependencies() {
    print_status "Installing vcpkg dependencies..."
    
    cleanup_vcpkg_locks

    detect_system_qt

    local vcpkg_args=(
        "install"
        "--triplet=$VCPKG_TRIPLET"
        "--clean-after-build"
        "--x-install-root=$VCPKG_INSTALLED_DIR"
    )

    if [ "$APVLV_WITH_OCR" = "1" ]; then
        vcpkg_args+=("--x-feature=ocr")
    fi
    if [ "$APVLV_WITH_MUPDF" = "1" ]; then
        vcpkg_args+=("--x-feature=mupdf")
    fi

    local manifest_root="$SRC_DIR"
    if [ "$SYSTEM_QT_FOUND" = "1" ]; then
        manifest_root="$SRC_DIR/vcpkg-manifests/system-qt"
        check_system_qt_deps
        print_status "Using system Qt. vcpkg manifest root: $manifest_root"
    else
        print_status "Using vcpkg Qt. vcpkg manifest root: $manifest_root"
    fi

    export VCPKG_INSTALLED_DIR

    if ! invoke_vcpkg_with_retry "cd '$SRC_DIR' && '$VCPKG_ROOT/vcpkg' ${vcpkg_args[*]} --x-manifest-root='$manifest_root'"; then
        print_error "Failed to install vcpkg dependencies after multiple attempts."
        exit 1
    fi
}


clean_build() {
    print_status "Cleaning build directory: $BUILD_DIR"
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        if [ $? -ne 0 ]; then
            print_error "Failed to remove build directory."
            exit 1
        fi
    fi
    mkdir -p "$BUILD_DIR" || {
        print_error "Failed to create build directory."
        exit 1
    }
}

configure() {
    local cmake_cmd="cmake \"$SRC_DIR\""

    cmake_cmd+=" -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    cmake_cmd+=" -DVCPKG_TARGET_TRIPLET=$VCPKG_TRIPLET"

    if [ "$SYSTEM_QT_FOUND" = "1" ]; then
        print_status "Configuring CMake to use system Qt from: $SYSTEM_QT_PREFIX"
        cmake_cmd+=" -DCMAKE_PREFIX_PATH=\"$SYSTEM_QT_PREFIX;$VCPKG_INSTALLED_DIR/$VCPKG_TRIPLET\""
        cmake_cmd+=" -DVCPKG_MANIFEST_MODE=OFF"
    fi

    local vcpkg_manifest_features=()
    if [ "$APVLV_WITH_MUPDF" = "1" ]; then
        cmake_cmd+=" -DAPVLV_WITH_MUPDF=ON"
        vcpkg_manifest_features+=("mupdf")
    fi
    if [ "$APVLV_WITH_OCR" = "1" ]; then
        cmake_cmd+=" -DAPVLV_WITH_OCR=ON"
        vcpkg_manifest_features+=("ocr")
    fi

    if [ ${#vcpkg_manifest_features[@]} -gt 0 ] && [ "$SYSTEM_QT_FOUND" != "1" ]; then
        cmake_cmd+=" -DVCPKG_MANIFEST_FEATURES=$(IFS=';'; echo "${vcpkg_manifest_features[*]}")"
    fi

    if command -v ninja &> /dev/null; then
        cmake_cmd+=' -G "Ninja"'
    else
        cmake_cmd+=' -G "Unix Makefiles"'
    fi

    execute_in_build_dir "$cmake_cmd"
}

build() {
    execute_cmake_command "--build" "." "--parallel"
}

create_package() {
    execute_cmake_command "--build" "." "--target" "package" "--parallel"
}

run_tests() {
    find_and_run_test
}

find_executable() {
    find_file_in_configs "apvlv" "$BUILD_DIR"
}

find_file_in_configs() {
    local filename="$1"
    local search_base="$2"
    
    FILE_FOUND=0
    for config in "$BUILD_TYPE" "Release" "Debug"; do
        if [ -f "$search_base/$config/$filename" ]; then
            print_status "Found: $search_base/$config/$filename"
            FILE_FOUND=1
            return
        fi
    done

    if [ -f "$search_base/$filename" ]; then
        print_status "Found: $search_base/$filename"
        FILE_FOUND=1
    fi

    if [ "$FILE_FOUND" = "0" ]; then
        print_warning "Could not locate $filename."
    fi
}

show_help() {
    echo "Usage: build.sh [debug clean test package deps help] [build_path]"
    echo

    echo "Options:"
    echo "    debug    - Build in debug mode"
    echo "    clean    - Clean build directory"
    echo "    test     - Run tests"
    echo "    package  - Create installer package"
    echo "    deps     - Install vcpkg dependencies only"
    echo "    help     - Show this help message"
    echo "    build_path - Optional path for global build directory (default: ~/build/apvlv)"
    echo

    echo "Environment Variables:"
    echo "    GLOBAL_BUILD_DIR  - Path to global build directory"
    echo "    VCPKG_ROOT        - Path to vcpkg installation (default: \$GLOBAL_BUILD_DIR/vcpkg)"
    echo "    VCPKG_TARGET_TRIPLET - Target triplet for vcpkg"
    echo "    APVLV_WITH_MUPDF  - Set to 1 to enable MuPDF support"
    echo "    APVLV_WITH_OCR    - Set to 1 to enable OCR support"
    echo "    USE_VCPKG_QT      - Set to 1 to force using vcpkg Qt instead of system Qt"
    echo "    "
    echo "vcpkg Manifests:"
    echo "    Default: uses $SRC_DIR/vcpkg.json (includes qtbase)"
    echo "    System Qt mode: uses $SRC_DIR/vcpkg-manifests/system-qt/vcpkg.json (no qtbase)"
    echo

    echo "Qt Detection:"
    echo "    The script will automatically detect system Qt6 installation."
    echo "    If found, system Qt will be used instead of vcpkg Qt."
    echo "    Set USE_VCPKG_QT=1 to force using vcpkg Qt."
    echo

    echo "Examples:"
    echo "    build.sh                      - Build release version in default directory"
    echo "    build.sh debug                - Build debug version in default directory"
    echo "    build.sh ~/apvlv_build        - Build release version in specified directory"
    echo "    build.sh debug ~/apvlv_build  - Build debug version in specified directory"
    echo "    build.sh deps                 - Install dependencies only"
    echo "    build.sh package              - Create installer"
    echo "    USE_VCPKG_QT=1 build.sh       - Force using vcpkg Qt"
}

#-------------------# MAIN SCRIPT# -------------------------------

# Set default values
BUILD_TYPE="${BUILD_TYPE:-Release}"
ACTION="${ACTION:-build}"
GLOBAL_BUILD_DIR="${GLOBAL_BUILD_DIR:-$HOME/build/apvlv}"
VCPKG_ROOT="${VCPKG_ROOT:-$GLOBAL_BUILD_DIR/vcpkg}"
BUILD_DIR="${BUILD_DIR:-$GLOBAL_BUILD_DIR/build}"
VCPKG_INSTALLED_DIR="${VCPKG_INSTALLED_DIR:-$GLOBAL_BUILD_DIR/vcpkg_installed}"
VCPKG_TRIPLET="${VCPKG_TARGET_TRIPLET:-x64-linux}"

# Environment variables for features
APVLV_WITH_MUPDF="${APVLV_WITH_MUPDF:-0}"
APVLV_WITH_OCR="${APVLV_WITH_OCR:-0}"
USE_VCPKG_QT="${USE_VCPKG_QT:-0}"

# System Qt detection results (will be set by detect_system_qt)
SYSTEM_QT_FOUND=0
SYSTEM_QT_PREFIX=""

# Determine if we're using a single-config generator
SINGLE_CONFIG="true"
if command -v ninja &> /dev/null; then
    SINGLE_CONFIG="false"
fi

# Parse arguments
while [ $# -gt 0 ]; do
    case "${1,,}" in
        "debug")
            BUILD_TYPE="Debug"
            ;;
        "clean")
            ACTION="clean"
            ;;
        "test")
            ACTION="test"
            ;;
        "package")
            ACTION="package"
            ;;
        "deps")
            ACTION="deps"
            ;;
        "help")
            show_help
            exit 0
            ;;

        *)
            # Assume it's a build path
            GLOBAL_BUILD_DIR="$1"
            VCPKG_ROOT="$GLOBAL_BUILD_DIR/vcpkg"
            BUILD_DIR="$GLOBAL_BUILD_DIR/build"
            VCPKG_INSTALLED_DIR="$GLOBAL_BUILD_DIR/vcpkg_installed"
            ;;
    esac
    shift
done

# Adjust triplet based on build type
if [ "$BUILD_TYPE" = "Debug" ]; then
    if [[ ! "$VCPKG_TRIPLET" =~ -debug$ ]]; then
        VCPKG_TRIPLET="${VCPKG_TRIPLET}-debug"
    fi
else
    if [[ "$VCPKG_TRIPLET" =~ -debug$ ]]; then
        VCPKG_TRIPLET="${VCPKG_TRIPLET%-debug}"
    fi
fi

print_status "Build type: $BUILD_TYPE"
print_status "VCPKG triplet: $VCPKG_TRIPLET"
print_status "Using build directory: $GLOBAL_BUILD_DIR"

execute_build

exit 0
