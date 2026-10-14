#!/bin/sh
# The local sanity check before a merge, in order: unit tests, perft, bench, the EPD smoke tier. Prints a summary and
# exits non-zero when a step fails. CI runs the extended EPD tier instead, see .github/workflows/suite.yml.
#
#   test/smoke.sh [build dir]     default: build, the CMake build with ElephantTest & ElephantSuite in it
#
# Bench is built with ob_build/makefile and g++ like the commit hook and OpenBench do, through WSL on Windows. It only
# warns when it differs from the last commit's bench line, a search change is supposed to change it. An EPD suite
# below its baseline warns as well, below its minimum it fails (test/suite/epd_suite_test.cpp).
#
# Every position the EPD suites search is recorded with test/results.py and compared with the nearest ancestor commit's
# smoke run, see test/RESULTS.md. On a dirty tree the run is in flight until a commit with exactly that tree is made.

root="$(git rev-parse --show-toplevel)"
build="${1:-$root/build}"
log_dir="$root/logs"
mkdir -p "$log_dir"
stamp="$(date +%Y%m%d_%H%M%S)"

failed=0
summary=""
add_summary() { summary="$summary$(printf '  %-6s %-10s %s' "$1" "$2" "$3")
"; }

find_binary() {
    # the Release build first, a multi config generator puts it in Release/bin, a single config one in bin.
    for candidate in "$build/Release/bin/$1.exe" "$build/Release/bin/$1" "$build/bin/$1.exe" "$build/bin/$1"; do
        [ -x "$candidate" ] && { echo "$candidate"; return; }
    done
    find "$build" -type f \( -name "$1" -o -name "$1.exe" \) -not -path "*CMakeFiles*" 2>/dev/null | head -n 1
}

unit="$(find_binary ElephantTest)"
suite="$(find_binary ElephantSuite)"
if [ -z "$unit" ] || [ -z "$suite" ]; then
    echo "smoke: ElephantTest or ElephantSuite not found under $build, build them first" >&2
    exit 2
fi

# --- 1. unit tests ---
echo "== unit tests: $unit"
start=$(date +%s)
if "$unit" --gtest_brief=1 > "$log_dir/smoke_unit_$stamp.log" 2>&1; then
    add_summary PASS unit "$(grep -a "PASSED" "$log_dir/smoke_unit_$stamp.log" | tail -n 1 | tr -s ' ') $(( $(date +%s) - start )) s"
else
    failed=1
    add_summary FAIL unit "see $log_dir/smoke_unit_$stamp.log"
    grep -a "FAILED" "$log_dir/smoke_unit_$stamp.log" | head -n 20
fi

# --- 2. bench ---
echo "== bench"
build_bench='cd ob_build && make -j"$(nproc)" EXE=build/elephant_ob >/dev/null 2>&1 && ./build/elephant_ob bench | tail -n 1'
if command -v g++ >/dev/null 2>&1 && command -v make >/dev/null 2>&1; then
    bench_line="$(cd "$root" && sh -c "$build_bench")"
elif command -v wsl.exe >/dev/null 2>&1; then
    wsl_root="$(wsl.exe -e wslpath -a "$root" | tr -d '\r')"
    bench_line="$(wsl.exe -e sh -c "cd '$wsl_root' && $build_bench" | tr -d '\r')"
fi
bench="$(echo "$bench_line" | grep -o '^[0-9]* nodes' | cut -d' ' -f1)"
committed="$(git -C "$root" log -1 --format=%B | grep -o 'bench [0-9]*' | tail -n 1 | cut -d' ' -f2)"
if [ -z "$bench" ]; then
    failed=1
    add_summary FAIL bench "no bench, g++/make or WSL missing or the build failed"
elif [ -n "$committed" ] && [ "$bench" != "$committed" ]; then
    add_summary WARN bench "$bench nodes, the last commit says $committed. Fine for a search change, not for a refactor"
else
    add_summary PASS bench "$bench_line"
fi

# --- 3. perft & the EPD smoke tier ---
echo "== perft & EPD smoke tier: $suite"
start=$(date +%s)
started_at="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
suite_log="$log_dir/smoke_suite_$stamp.log"
checks="$log_dir/smoke_checks_$stamp.jsonl"
# from the root, the EPD suites write their logs to logs/.
if (cd "$root" && EG_RESULTS_CHECKS="$checks" "$suite" --gtest_filter='PerftFixture.*:EpdSuite.Smoke*:EpdParser.*') 2>&1 | tee "$suite_log" \
        | grep -aE "solved \(|points \(|^WARNING|^NOTE|^FAIL|FAILED  \]"; then :; fi
if grep -aq "^\[  FAILED  \]" "$suite_log" || ! grep -aq "^\[  PASSED  \]" "$suite_log"; then
    failed=1
    add_summary FAIL suite "see $suite_log"
else
    add_summary PASS suite "perft & EPD smoke tier, $(( $(date +%s) - start )) s"
fi
while IFS= read -r line; do
    case "$line" in
        WARNING*) add_summary WARN epd "${line#WARNING }" ;;
        NOTE*)    add_summary NOTE epd "${line#NOTE }" ;;
    esac
done <<EOF
$(grep -aE "^(WARNING|NOTE) " "$suite_log")
EOF

# --- 4. results: record the positions, compare with history ---
python=""
for candidate in python3 python py; do
    command -v "$candidate" >/dev/null 2>&1 && "$candidate" -c "import sys; sys.exit(sys.version_info < (3, 9))" 2>/dev/null \
        && { python="$candidate"; break; }
done
if [ -z "$python" ]; then
    add_summary WARN results "no python 3.9+, the positions weren't recorded"
elif [ ! -s "$checks" ]; then
    add_summary WARN results "the suites wrote no results, nothing recorded"
elif record="$("$python" "$root/test/results.py" record --checks "$checks" --kind smoke --started "$started_at")"; then
    echo "$record"
    comparison="$("$python" "$root/test/results.py" compare)"
    echo "$comparison"
    while IFS= read -r line; do
        case "$line" in
            "  "[A-Za-z]*:*) add_summary INFO results "${line#  }" ;;
            results:*) add_summary INFO results "${line#results: }" ;;
        esac
    done <<EOF
$(printf '%s\n' "$comparison" | grep -aE "^(  [A-Za-z]|results:)")
EOF
    rm -f "$checks"
else
    add_summary WARN results "recording failed, the checks are in $checks"
fi

echo
echo "== smoke summary"
printf '%s' "$summary"
[ "$failed" -eq 0 ] && echo "smoke: passed" || echo "smoke: FAILED"
exit "$failed"
