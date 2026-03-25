#!/bin/bash
# validate-wasm.sh - Validate WASM output from compiler
#
# Usage: ./validate-wasm.sh <input.c> [output.wat]
#
# This script compiles a C file to WASM and validates the output using wat2wasm

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMPILER="$SCRIPT_DIR/../compiler"

if [ $# -lt 1 ]; then
    echo "Usage: $0 <input.c> [output.wat]"
    exit 1
fi

INPUT="$1"
OUTPUT="${2:-/tmp/validate_output.wat}"

echo "========================================="
echo "WASM Validation"
echo "========================================="
echo "Input:  $INPUT"
echo "Output: $OUTPUT"
echo ""

# Check if compiler exists
if [ ! -x "$COMPILER" ]; then
    echo "Error: Compiler not found at $COMPILER"
    echo "Run 'make' first to build the compiler"
    exit 1
fi

# Check if wat2wasm is available
if ! command -v wat2wasm &> /dev/null; then
    echo "Error: wat2wasm not found"
    echo "Install WABT: brew install wabt (macOS) or sudo apt-get install wabt (Linux)"
    exit 1
fi

# Compile to WAT
echo "Step 1: Compiling to WAT..."
if "$COMPILER" --target=wasm "$INPUT" -o "$OUTPUT" 2>/dev/null; then
    echo "  [PASS] Compilation successful"
else
    echo "  [FAIL] Compilation failed"
    exit 1
fi

# Validate WAT syntax
echo "Step 2: Validating WAT syntax..."
if wat2wasm "$OUTPUT" -o /dev/null 2>/dev/null; then
    echo "  [PASS] WAT syntax is valid"
else
    echo "  [FAIL] WAT syntax error"
    wat2wasm "$OUTPUT" -o /dev/null 2>&1 | head -5
    exit 1
fi

# Convert to WASM binary
WASM_OUTPUT="${OUTPUT%.wat}.wasm"
echo "Step 3: Converting to WASM binary..."
if wat2wasm "$OUTPUT" -o "$WASM_OUTPUT" 2>/dev/null; then
    echo "  [PASS] WASM binary created: $WASM_OUTPUT"
else
    echo "  [FAIL] WASM conversion failed"
    exit 1
fi

# Show output statistics
echo ""
echo "========================================="
echo "Validation Summary"
echo "========================================="
echo "WAT file:   $OUTPUT ($(/usr/bin/wc -l < "$OUTPUT") lines)"
echo "WASM file:  $WASM_OUTPUT ($(stat -f%z "$WASM_OUTPUT" 2>/dev/null || stat -c%s "$WASM_OUTPUT" 2>/dev/null) bytes)"
echo ""
echo "All validation steps passed!"
