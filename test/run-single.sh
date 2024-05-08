#!/bin/bash

check_test_result() {
    if [ $? -ne 0 ]; then
        echo -e "\e[31m--- $1 FAILED! ---\e[0m"
        exit 1
    else
        echo -e "\e[32m--- $1 PASSED! ---\e[0m"
    fi
}

# Check number of arguments
if [ $# -ne 1 ]; then
    echo "Usage: $0 <TEST_EXECUTABLE>"
    exit 1
fi

# Get the file path
test_file=$1

echo -e "\e[34m--- Running $test_file ---\e[0m"
./$test_file
check_test_result
