#!/usr/bin/env bash
# ============================================================================
# Tascam US-122L Manager - Build Script
# ============================================================================
# Build the C++ version of the Tascam US-122L Manager
# ============================================================================

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Paths
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
SOURCE_DIR="${PROJECT_DIR}/src"

# Check dependencies
check_deps() {
    echo -e "${GREEN}[INFO]${NC} Checking dependencies..."

    local missing=0

    if ! command -v cmake &>/dev/null; then
        echo -e "${RED}[ERROR]${NC} cmake not found"
        missing=1
    fi

    if ! command -v ninja &>/dev/null && ! command -v make &>/dev/null; then
        echo -e "${RED}[ERROR]${NC} ninja or make not found"
        missing=1
    fi

    if ! pkg-config --exists Qt6Core Qt6Widgets; then
        echo -e "${RED}[ERROR]${NC} Qt6 not found"
        missing=1
    fi

    if ! pkg-config --exists jack; then
        echo -e "${RED}[ERROR]${NC} JACK not found"
        missing=1
    fi

    if ! pkg-config --exists libpipewire-0.3; then
        echo -e "${RED}[ERROR]${NC} PipeWire not found"
        missing=1
    fi

    if ! pkg-config --exists alsa; then
        echo -e "${RED}[ERROR]${NC} ALSA not found"
        missing=1
    fi

    if [ $missing -eq 1 ]; then
        echo -e "${YELLOW}[WARN]${NC} Some dependencies are missing. Install them with:"
        echo "  su -c 'pacman -S cmake ninja qt6-base jack pipewire alsa-lib'"
        exit 1
    fi

    echo -e "${GREEN}[OK]${NC} All dependencies found"
}

# Create build directory
setup_build() {
    echo -e "${GREEN}[INFO]${NC} Setting up build directory..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # Configure CMake
    echo -e "${GREEN}[INFO]${NC} Configuring CMake..."
    cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_GUI=ON \
        -DBUILD_STATIC=OFF \
        "$PROJECT_DIR"

    # Build
    echo -e "${GREEN}[INFO]${NC} Building..."
    cmake --build . --config Release -j$(nproc)

    # Install launcher to user-local bin (no sudo needed)
    LAUNCHER_SRC="${PROJECT_DIR}/scripts/tascam-jackd-launcher.py"
    LAUNCHER_DST="$HOME/.local/bin/tascam-jackd-launcher.py"
    if [ ! -f "$LAUNCHER_DST" ]; then
        mkdir -p "$HOME/.local/bin"
        cp "$LAUNCHER_SRC" "$LAUNCHER_DST"
        chmod +x "$LAUNCHER_DST"
        echo -e "${GREEN}[INFO]${NC} Launcher installed to $LAUNCHER_DST"
    fi

    # Optional install (requires sudo for system-wide install)
    if [ "${INSTALL:-0}" = "1" ]; then
        echo -e "${GREEN}[INFO]${NC} Installing system-wide..."
        su -c "cmake --install ."
        echo -e "${GREEN}[INFO]${NC} Binary installed to /usr/bin/tascam-us122l-manager"
    else
        echo -e "${YELLOW}[INFO]${NC} Build complete. Install with:"
        echo "  ./build.sh --install   (system-wide, requires sudo)"
        echo "  cmake --install build --prefix \$HOME/.local   (user-only)"
        echo "  build/tascam-us122l-manager                    (run directly from build dir)"
    fi
}

# Main
main() {
    echo "========================================"
    echo "  Tascam US-122L Manager - Build"
    echo "========================================"

    for arg in "$@"; do
        case "$arg" in
            --install) INSTALL=1 ;;
            *) echo -e "${YELLOW}[WARN]${NC} Unknown argument: $arg" ;;
        esac
    done

    check_deps
    setup_build

    echo -e "${GREEN}[OK]${NC} Done!"
}

main "$@"