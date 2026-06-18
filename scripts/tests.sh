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
    "edge_precedence.c" "edge_boolexpr.c" "edge_dangling_else.c"
    "edge_recursion.c" "edge_structs_chain.c" "edge_funcptr.c"
    "edge_sizeof.c" "edge_globals.c" "edge_nested_loops.c"
    "edge_many_temps.c" "edge_void_return.c" "edge_type_ops_valides.c"
    "gap_break_keyword.c" "gap_undeclared_var.c"
)

TESTS_FAIL=(
    "expr.c"
    "pointeur.c"
    "err_increment.c"
    "err_args.c"
    "err_undeclared.c"
    "err_ternary.c"
    "err_array.c"
    "err_double_pointer.c"
    "err_sizeof_struct.c"
    "err_sizeof_type.c"
    "err_struct_par_valeur.c"
    "err_struct_champ_par_valeur.c"
    "err_struct_param_par_valeur.c"
    "err_struct_retour_par_valeur.c"
    "err_type_ops.c"
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
