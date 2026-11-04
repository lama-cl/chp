#!/bin/bash

set -e

make unit_tests

# We run ./unit_tests once per test because several of them test SIGILL and we
# haven't implemented a way to recover from it and continue running additional
# tests as opposed to just reporting that it happened as expected.
test_names=($(sed -nEe '/^#define/!s,.*REQUESTED_TEST_IS\(([^()]+)\).*,\1,p' unit_tests.c))

all_passed=true

for test_name in "${test_names[@]}"; do
    echo "=== $test_name ==="
    if ! ./unit_tests "$test_name"; then
        echo "TEST FAILED"
        all_passed=false
    fi
done

if $all_passed; then
  echo '=== All tests passed. ==='
else
  echo '=== At least one test failed: see above. ==='
fi

# Set exit status.
$all_passed
