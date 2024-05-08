#!/bin/bash

# Array of test files accessible to both build-all.sh
TEST_FILES=(
    registration_test_sock_handler.c
    initcomm_test_sock_handler.c
    start_unconditional_stream_test_sock_handler.c
    stop_unconditional_stream_test_sock_handler.c
    initcomm_start_stop_unconditional_stream_test_sock_handler.c
    nop_test_sock_handler.c
    interactive_test_sock_handler.c
)

# Compute array of test executables accessible to run-all.sh
TEST_EXECUTABLES=()
for test_file in ${TEST_FILES[@]}; do
    test_name=$(basename $test_file _sock_handler.c)
    TEST_EXECUTABLES+=($test_name)
done