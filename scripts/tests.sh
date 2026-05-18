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

TESTS_OK=(
    "add.c" "sub.c" "mul.c" "div.c" "neg.c"
    "variables.c" "loops.c" "cond.c"
    "functions.c" "listes.c" "compteur.c" "ptr.c"
)

TESTS_FAIL=(
    "expr.c"
    "pointeur.c"
    "err_increment.c"
    "err_args.c"
    "err_undeclared.c"
)

FAILED=0

for test in "${TESTS_OK[@]}"; do
    TEST_FILE="$TEST_DIR/$test"
    BASE_NAME="${test%.c}"
    OUTPUT_FILE="$OUTPUT_DIR/${BASE_NAME}_3.c"

    if [ ! -f "$TEST_FILE" ]; then
        echo "SKIP $test (fichier manquant)"
        ((FAILED++))
        continue
    fi

    if $BIN "$TEST_FILE" "$OUTPUT_FILE" > /dev/null 2>&1; then
        [ $PRINT_DETAILS = true ] && echo "OK   $test"
    else
        echo "FAIL $test (devrait passer)"
        ((FAILED++))
    fi

    if [ $VALIDATE_BACKEND = true ] && [ -f "$OUTPUT_FILE" ]; then
        if ./bin/structit_backend < "$OUTPUT_FILE" > /dev/null 2>&1; then
            [ $PRINT_DETAILS = true ] && echo "OK   $test (backend)"
        else
            echo "FAIL $test (backend)"
            ((FAILED++))
        fi
    fi
done

for test in "${TESTS_FAIL[@]}"; do
    TEST_FILE="$TEST_DIR/$test"
    OUTPUT_FILE="/dev/null"

    if [ ! -f "$TEST_FILE" ]; then
        echo "SKIP $test (fichier manquant)"
        ((FAILED++))
        continue
    fi

    if $BIN "$TEST_FILE" "$OUTPUT_FILE" > /dev/null 2>&1; then
        echo "FAIL $test (devrait échouer)"
        ((FAILED++))
    else
        [ $PRINT_DETAILS = true ] && echo "OK   $test (erreur attendue)"
    fi
done

if [ $FAILED -eq 0 ]; then
    echo "Tous les tests passent."
    exit 0
else
    echo "$FAILED test(s) échoué(s)."
    exit 1
fi
