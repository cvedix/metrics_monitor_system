#!/bin/bash

# Test script to verify POST endpoint saves instances correctly

PORT=${1:-8080}
BASE_URL="http://192.168.10.50:${PORT}"

echo "=========================================="
echo "Testing POST /v1/core/system/info with instances"
echo "=========================================="
echo ""

# Test data with new instance
TEST_DATA='{
    "device": {
        "version": "2025.0.1.2",
        "serial_number": "202508110010001",
        "model_type": "BETA_FR_SVR.1.0.0.1.1",
        "device_type": "edge",
        "hardware_revision": "REV_B",
        "production_date": "2025-11-8",
        "warranty_period": "12",
        "build_date": "Oct 16 2025 09:08:23",
        "mode": "production"
    },
    "endpoint_port": "8080",
    "instances": [
        "41c708a0-4156-e5da-270a-1a57e52ca082"
    ]
}'

echo "1. Current device_registered.json before POST:"
echo "------------------------------------------------------"
if [ -f "./build/device_registered.json" ]; then
    cat ./build/device_registered.json | python3 -m json.tool 2>/dev/null || cat ./build/device_registered.json
elif [ -f "./device_registered.json" ]; then
    cat ./device_registered.json | python3 -m json.tool 2>/dev/null || cat ./device_registered.json
else
    echo "File not found"
fi
echo ""
echo ""

echo "2. Sending POST request..."
echo "-----------------------------------------------------------"
RESPONSE=$(curl -s -X POST "${BASE_URL}/v1/core/system/info" \
  -u cvedix:cvedix \
  -H "Content-Type: application/json" \
  -d "${TEST_DATA}")

echo "Response:"
echo "${RESPONSE}" | python3 -m json.tool 2>/dev/null || echo "${RESPONSE}"
echo ""
echo ""

echo "3. Checking device_registered.json after POST:"
echo "--------------------------------------------------"
sleep 1  # Wait a moment for file to be written
if [ -f "./build/device_registered.json" ]; then
    echo "✓ File ./build/device_registered.json exists"
    echo "Content:"
    cat ./build/device_registered.json | python3 -m json.tool 2>/dev/null || cat ./build/device_registered.json
    echo ""
    echo "Instances in file:"
    cat ./build/device_registered.json | grep -A 5 '"instances"' | python3 -m json.tool 2>/dev/null || cat ./build/device_registered.json | grep -A 5 '"instances"'
elif [ -f "./device_registered.json" ]; then
    echo "✓ File ./device_registered.json exists"
    echo "Content:"
    cat ./device_registered.json | python3 -m json.tool 2>/dev/null || cat ./device_registered.json
    echo ""
    echo "Instances in file:"
    cat ./device_registered.json | grep -A 5 '"instances"' | python3 -m json.tool 2>/dev/null || cat ./device_registered.json | grep -A 5 '"instances"'
elif [ -f "/etc/device_registered.json" ]; then
    echo "✓ File /etc/device_registered.json exists"
    echo "Content:"
    cat /etc/device_registered.json | python3 -m json.tool 2>/dev/null || cat /etc/device_registered.json
else
    echo "✗ device_registered.json not found!"
fi
echo ""
echo ""

echo "4. Testing GET to verify instances:"
echo "--------------------------------------------------"
GET_RESPONSE=$(curl -s "${BASE_URL}/v1/core/system/info")
echo "Instances in GET response:"
echo "${GET_RESPONSE}" | python3 -c "import sys, json; data=json.load(sys.stdin); print(json.dumps(data.get('instances', []), indent=2))" 2>/dev/null || echo "Failed to parse response"


