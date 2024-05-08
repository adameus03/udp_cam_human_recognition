#!/bin/bash

# Get TEST_EXECUTABLES array
. ./test-config.sh

# For each test executable call run-single.sh
for test_file in ${TEST_EXECUTABLES[@]}; do
    ./run-single.sh $test_file
done