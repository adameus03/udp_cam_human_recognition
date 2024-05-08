#!/bin/bash

# Check number of arguments
if [ $# -ne 1 ]; then
    echo "Usage: $0 <TEST_SOCK_HANDLER_C_FILE>"
    exit 1
fi

# Get the file path
test_sock_handler_c_file=$1

# Get test name
test_name=$(basename $test_sock_handler_c_file _sock_handler.c)

# Call gcc
gcc test.c test_handlers.c $test_sock_handler_c_file -o $test_name

