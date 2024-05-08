#!/bin/bash

# Get TEST_FILES array
. ./test-config.sh

# For each test file call build-single.sh
for test_file in ${TEST_FILES[@]}; do
    ./build-single.sh $test_file
done
