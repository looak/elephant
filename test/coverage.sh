#!/bin/sh
# Test coverage of the unit tests with gcc & gcovr, test/coverage.ps1's twin for Linux: configures a build with
# ELEPHANT_COVERAGE (Debug, phant-indexer on, ElephantSuite off), builds ElephantTest, runs ctest -L unit and writes an
# HTML report to <build dir>/coverage/index.html, then prints gcovr's summary. Sources under test/, third_party/ &
# _deps/ are left out. Needs gcovr (pipx install gcovr).
#
#   test/coverage.sh [build dir]     default: build-cov in the repository
set -e

root="$(cd "$(dirname "$0")/.." && pwd)"
build="${1:-$root/build-cov}"
report="$build/coverage"
command -v gcovr >/dev/null 2>&1 || { echo "coverage: gcovr not found, pipx install gcovr" >&2; exit 2; }

cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc \
    -DELEPHANT_COVERAGE=ON -DELEPHANT_BUILD_TEST_SUITE=OFF -DELEPHANT_BUILD_INDEXER=ON -DELEPHANT_BUILD_TUNER=OFF
cmake --build "$build" --target ElephantTest --parallel

# the counters add up over runs, start from zero.
find "$build" -name '*.gcda' -delete
rm -rf "$report" && mkdir -p "$report"
tests=0
ctest --test-dir "$build" -L unit -j "$(nproc)" --output-on-failure || tests=$?

# throw branches are the exception paths every call that can throw grows, gcc counts them as branches. The hot board
# accessors pass 2^32 hits in the unit tests, gcovr's default threshold for a gcov bug, so it's off.
cd "$root"
gcovr --root "$root" --object-directory "$build" --exclude '(.*/)?(test|third_party|_deps)/' --exclude-throw-branches \
    --gcov-suspicious-hits-threshold 0 --html-details "$report/index.html" --txt --print-summary

echo "coverage: report in $report/index.html"
[ "$tests" -eq 0 ] || echo "coverage: unit tests failed, the report covers the tests that ran" >&2
exit "$tests"
