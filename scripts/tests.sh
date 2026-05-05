#!/bin/bash

VALIDATE_BACKEND=false
PRINT_DETAILS=false

if [ "$1" == "--validate" ] || [ "$2" == "--validate" ]; then
    VALIDATE_BACKEND=true
fi
if [ "$1" == "--verbose" ] || [ "$2" == "--verbose" ]; then
    PRINT_DETAILS=true
fi

TEST_DIR="tests"
BIN="./bin/structit"
OUTPUT_DIR="output"
mkdir -p "$OUTPUT_DIR"

TESTS=(
    "add.c" "sub.c" "mul.c" "div.c" "neg.c"
    "variables.c" "expr.c" "loops.c" "cond.c"
    "functions.c" "pointeur.c" "listes.c" "compteur.c"
)

FAILED=0

for test in "${TESTS[@]}"; do
    TEST_FILE="$TEST_DIR/$test"
    BASE_NAME="${test%.c}"
    OUTPUT_FILE="$OUTPUT_DIR/${BASE_NAME}_3.c"

    if [ ! -f "$TEST_FILE" ]; then
        if [ $PRINT_DETAILS = true ]; then
            echo "SKIP $test"
        fi
        ((FAILED++))
        continue
    fi
    
    if $BIN "$TEST_FILE" > "$OUTPUT_FILE" 2>&1; then
        if [ $PRINT_DETAILS = true ]; then
            echo "FRONT SUCC $test"
        fi
    else
        if [ $PRINT_DETAILS = true ]; then
            echo "FRONT FAIL $test"
        fi
        ((FAILED++))
    fi

    if [ $VALIDATE_BACKEND = true ]; then
        if ./bin/structit_backend "$OUTPUT_FILE" > /dev/null 2>&1; then
            if [ $PRINT_DETAILS = true ]; then
                echo "BACK SUCC $test"
            fi
        else
            if [ $PRINT_DETAILS = true ]; then
                echo "BACK FAIL $test"
            fi
            ((FAILED++))
        fi
    fi

done

if [ $FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi