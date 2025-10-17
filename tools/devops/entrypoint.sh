#!/bin/bash
set -e

echo "🔥 ForeFire Quick Start Demo"
echo "=============================="
echo ""

# Navigate to the test directory
cd /forefire/tests/runff

echo "📁 Working directory: $(pwd)"
echo "🌐 Starting ForeFire with HTTP server..."
echo ""
echo "Once started, open your browser to: http://localhost:8000"
echo "To run the demo simulation, type: include[real_case.ff]"
echo "To stop, press Ctrl+C"
echo ""

# Function to handle graceful shutdown
cleanup() {
    echo ""
    echo "🛑 Shutting down ForeFire..."
    exit 0
}

# Set up signal handlers for graceful shutdown
trap cleanup SIGINT SIGTERM

# Start forefire and automatically launch HTTP server
forefire -l &
FOREFIRE_PID=$!

# Wait for the forefire process
wait $FOREFIRE_PID
