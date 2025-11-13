#!/bin/bash

# Simple test script for API endpoints
# Make sure the server is running before executing this script

PORT=${1:-8080}
BASE_URL="http://localhost:${PORT}"

echo "Testing Metrics Monitor System API"
echo "=================================="
echo ""

echo "1. Testing GET /v1/core/system/info"
echo "-----------------------------------"
curl -s "${BASE_URL}/v1/core/system/info" | python3 -m json.tool 2>/dev/null || curl -s "${BASE_URL}/v1/core/system/info"
echo ""
echo ""

echo "2. Testing GET /v1/core/system/status"
echo "--------------------------------------"
curl -s "${BASE_URL}/v1/core/system/status" | python3 -m json.tool 2>/dev/null || curl -s "${BASE_URL}/v1/core/system/status"
echo ""
echo ""

echo "3. Testing POST /v1/core/system/reboot"
echo "--------------------------------------"
curl -s -X POST "${BASE_URL}/v1/core/system/reboot" | python3 -m json.tool 2>/dev/null || curl -s -X POST "${BASE_URL}/v1/core/system/reboot"
echo ""
echo ""

echo "4. Testing GET /health"
echo "----------------------"
curl -s "${BASE_URL}/health" | python3 -m json.tool 2>/dev/null || curl -s "${BASE_URL}/health"
echo ""
echo ""

echo "5. Testing GET / (root)"
echo "----------------------"
curl -s "${BASE_URL}/" | python3 -m json.tool 2>/dev/null || curl -s "${BASE_URL}/"
echo ""

