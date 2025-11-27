#!/bin/bash

echo "Testing Firmware Update API..."

# Start server in background
./build/metrics_monitor_system &
SERVER_PID=$!

echo "Server started with PID: $SERVER_PID"
sleep 2

# Test case 1: Valid request
echo "Sending valid request..."
curl -v -X POST http://localhost:8080/v1/core/firmware/command \
     -H "Content-Type: application/json" \
     -d '{"action": "update", "url": "http://example.com/firmware.deb"}'

echo -e "\n\n"

# Test case 2: Invalid action
echo "Sending invalid action..."
curl -v -X POST http://localhost:8080/v1/core/firmware/command \
     -H "Content-Type: application/json" \
     -d '{"action": "delete", "url": "http://example.com/firmware.deb"}'

echo -e "\n\n"

# Test case 3: Missing URL
echo "Sending missing URL..."
curl -v -X POST http://localhost:8080/v1/core/firmware/command \
     -H "Content-Type: application/json" \
     -d '{"action": "update"}'

echo -e "\n\n"

# Kill server
kill $SERVER_PID
echo "Server stopped."
