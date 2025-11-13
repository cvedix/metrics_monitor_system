#!/bin/bash

# Script to setup dependencies for metrics_monitor_system

set -e

echo "Setting up dependencies..."

# Create third_party directory
mkdir -p third_party
cd third_party

# Clone hwinfo if not exists
if [ ! -d "hwinfo" ]; then
    echo "Cloning hwinfo..."
    git clone https://github.com/lfreist/hwinfo.git
else
    echo "hwinfo already exists, skipping..."
fi

# Download cpp-httplib header file
mkdir -p cpp-httplib
if [ ! -f "cpp-httplib/httplib.h" ]; then
    echo "Downloading cpp-httplib..."
    wget -q https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h -O cpp-httplib/httplib.h
    echo "cpp-httplib downloaded successfully"
else
    echo "cpp-httplib already exists, skipping..."
fi

cd ..

echo "Dependencies setup complete!"
echo ""
echo "Next steps:"
echo "1. mkdir build && cd build"
echo "2. cmake .."
echo "3. cmake --build ."
echo "4. ./metrics_monitor_system [port]"

