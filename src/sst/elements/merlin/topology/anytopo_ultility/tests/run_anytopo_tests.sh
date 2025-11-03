#!/bin/bash
# Script to run all test_anytopo.py topology tests

# Change to script directory
cd "$(dirname "${BASH_SOURCE[0]}")"

echo "========================================="
echo "Running SST AnyTopo Tests"
echo "========================================="
echo ""

# List of topologies to test
TOPOLOGIES=("complete_4" "cubical" "dallydragonfly" "slimfly" "polarfly" "jellyfish")

# Track results
passed=0
failed=0

# Run each topology test
for topo in "${TOPOLOGIES[@]}"; do
    echo "Running test: $topo topology..."
    
    if sst test_anytopo.py -- --topology "$topo"; then
        echo "✓ $topo passed"
        ((passed++))
    else
        echo "✗ $topo failed"
        ((failed++))
    fi
    echo ""
done

# Print summary
echo "========================================="
echo "Test Summary"
echo "========================================="
echo "Passed: $passed"
echo "Failed: $failed"
echo ""

# List generated output files
echo "Generated files:"
ls -1 stat_*.csv 2>/dev/null || echo "  (none)"

# Exit with error if any tests failed
[ $failed -eq 0 ]
