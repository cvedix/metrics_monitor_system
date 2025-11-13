#!/bin/bash

# Test script to verify POST and GET endpoints work correctly

PORT=${1:-8080}
BASE_URL="http://localhost:${PORT}"

echo "=========================================="
echo "Testing POST and GET /v1/core/system/info"
echo "=========================================="
echo ""

# Test data
TEST_DATA='{
  "device": {
    "version": "2.0.0",
    "serial_number": "TEST123456",
    "model_type": "TEST_MODEL",
    "device_type": "TEST_DEVICE",
    "hardware_revision": "REV_B",
    "production_date": "2025-01-01",
    "warranty_period": "36",
    "build_date": "Jan 1 2025 10:00:00",
    "mode": "test"
  },
  "endpoint_port": "9999",
  "instances": ["test-instance-1", "test-instance-2"]
}'

echo "1. Testing GET before POST (should show default values)"
echo "------------------------------------------------------"
curl -s "${BASE_URL}/v1/core/system/info" | python3 -m json.tool 2>/dev/null | grep -A 15 '"device"' | head -20
echo ""
echo ""

echo "2. Testing POST /v1/core/system/info (register device info)"
echo "-----------------------------------------------------------"
RESPONSE=$(curl -s -X POST "${BASE_URL}/v1/core/system/info" \
  -u cvedix:cvedix \
  -H "Content-Type: application/json" \
  -d "${TEST_DATA}")

echo "Response:"
echo "${RESPONSE}" | python3 -m json.tool 2>/dev/null || echo "${RESPONSE}"
echo ""
echo ""

echo "3. Checking if device_registered.json was created"
echo "--------------------------------------------------"
if [ -f "./device_registered.json" ]; then
    echo "✓ File ./device_registered.json exists"
    echo "Content:"
    cat ./device_registered.json | python3 -m json.tool 2>/dev/null || cat ./device_registered.json
elif [ -f "/etc/device_registered.json" ]; then
    echo "✓ File /etc/device_registered.json exists"
    echo "Content:"
    cat /etc/device_registered.json | python3 -m json.tool 2>/dev/null || cat /etc/device_registered.json
else
    echo "✗ device_registered.json not found!"
fi
echo ""
echo ""

echo "4. Testing GET after POST (should show POSTed values)"
echo "------------------------------------------------------"
echo "Device info from GET:"
curl -s "${BASE_URL}/v1/core/system/info" | python3 -m json.tool 2>/dev/null | grep -A 15 '"device"' | head -20
echo ""
echo "Endpoint port:"
curl -s "${BASE_URL}/v1/core/system/info" | python3 -m json.tool 2>/dev/null | grep '"endpoint_port"'
echo ""
echo "Instances:"
curl -s "${BASE_URL}/v1/core/system/info" | python3 -m json.tool 2>/dev/null | grep -A 5 '"instances"'
echo ""

