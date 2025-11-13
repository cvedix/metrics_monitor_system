#!/bin/bash
# Build script for creating .deb package for arm64

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "Building .deb package for arm64"
echo "=========================================="
echo ""

# Check if we're on arm64 or cross-compiling
ARCH=$(dpkg --print-architecture)
echo "Current architecture: $ARCH"

if [ "$ARCH" != "arm64" ]; then
    echo "Warning: Not on arm64. You may need to cross-compile."
    echo "For cross-compilation, install: gcc-aarch64-linux-gnu g++-aarch64-linux-gnu"
fi

# Check dependencies
echo ""
echo "Checking build dependencies..."
MISSING_DEPS=""
if ! command -v debhelper &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS debhelper"
fi
if ! command -v fakeroot &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS fakeroot"
fi
if ! command -v cmake &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS cmake"
fi
if ! command -v g++ &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS g++"
fi
if ! command -v git &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS git"
fi

if [ -n "$MISSING_DEPS" ]; then
    echo "Installing missing dependencies:$MISSING_DEPS..."
    sudo apt-get update
    sudo apt-get install -y $MISSING_DEPS build-essential
fi

# Check and install build dependencies from control file
echo ""
echo "Checking build dependencies from debian/control..."
if ! dpkg-checkbuilddeps 2>&1 | grep -q "Unmet build dependencies"; then
    echo "All build dependencies satisfied."
else
    echo "Installing unmet build dependencies..."
    sudo apt-get update
    sudo apt-get install -y $(dpkg-checkbuilddeps 2>&1 | grep "Unmet build dependencies" | sed 's/.*: //' | tr -d ',')
fi

# Clean previous builds
echo ""
echo "Cleaning previous builds..."
rm -rf debian/metrics-monitor-system
rm -rf debian/files
rm -rf debian/*.substvars
rm -rf debian/*.log
rm -rf ../*.deb
rm -rf ../*.buildinfo
rm -rf ../*.changes

# Setup dependencies
echo ""
echo "Setting up dependencies..."
./setup_dependencies.sh

# Build the package
echo ""
echo "Building Debian package..."

# Determine architecture
ARCH=$(dpkg --print-architecture)
if [ "$ARCH" = "arm64" ]; then
    # Native build
    echo "Native build for arm64..."
    dpkg-buildpackage -b -us -uc
else
    # Cross-compile for arm64
    echo "Cross-compiling for arm64..."
    dpkg-buildpackage -b -us -uc -aarm64
fi

# Check if build was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "Build successful!"
    echo "=========================================="
    echo ""
    echo "Package files created:"
    ls -lh ../metrics-monitor-system_*.deb 2>/dev/null || echo "No .deb file found in parent directory"
    echo ""
    echo "To install on another arm64 system:"
    echo "  sudo dpkg -i metrics-monitor-system_*.deb"
    echo "  sudo apt-get install -f  # Install missing dependencies"
    echo ""
else
    echo ""
    echo "Build failed! Check the error messages above."
    exit 1
fi

