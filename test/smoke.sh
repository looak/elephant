#!/bin/sh
# The local sanity check before a merge, in order: unit tests, bench, ElephantSuite, the EPD smoke tier. Prints a summary and
# exits non-zero when a step fails. CI runs the unit tests & perft only, see .github/workflows/ci.yml.
#
#   test/smoke.sh [build dir]     default: build, the CMake build with ElephantTest, ElephantSuite & ElephantGambit
#
# Bench is built with ob_build/makefile and g++ like the commit hook and OpenBench do, through WSL on Windows. It only
# warns when it differs from the last commit's bench line, a search change is supposed to change it.
#
# The EPD smoke tier is test/epd_runner.py playing the positions with ElephantGambit over UCI, one engine per core
# (test/epd_tiers.json). A suite below its baseline warns, below its minimum it fails. Every position is recorded with
# test/results.py and compared with the nearest ancestor commit's smoke run, see test/RESULTS.md. On a dirty tree the
# run is wip until a commit with exactly that tree is made. Runs are uploaded to nyayo when it's configured
# (git config nyayo.url).

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
engine="$(find_binary ElephantGambit)"
if [ -z "$unit" ] || [ -z "$suite" ] || [ -z "$engine" ]; then
    echo "smoke: ElephantTest, ElephantSuite or ElephantGambit not found under $build, build them first" >&2
    exit 2
fi

python=""
for candidate in python3 python py; do
    command -v "$candidate" >/dev/null 2>&1 && "$candidate" -c "import sys; sys.exit(sys.version_info < (3, 9))" 2>/dev/null \
        && { python="$candidate"; break; }
done

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

# --- 3. ElephantSuite: deep perft & deep searches ---
echo "== suite: $suite"
start=$(date +%s)
suite_log="$log_dir/smoke_suite_$stamp.log"
if (cd "$root" && "$suite") > "$suite_log" 2>&1 \
        && grep -aq "^\[  PASSED  \]" "$suite_log"; then
    add_summary PASS suite "$(( $(date +%s) - start )) s"
else
    failed=1
    add_summary FAIL suite "see $suite_log"
    grep -a "FAILED" "$suite_log" | head -n 20
fi

# --- 4. the EPD smoke tier ---
echo "== EPD smoke tier: $engine"
start=$(date +%s)
started_at="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
epd_log="$log_dir/smoke_epd_$stamp.log"
checks="$log_dir/smoke_checks_$stamp.jsonl"
if [ -z "$python" ]; then
    failed=1
    add_summary FAIL epd "no python 3.9+ for test/epd_runner.py"
else
    "$python" "$root/test/epd_runner.py" --engine "$engine" --tier smoke --out "$checks" 2>&1 | tee "$epd_log"
    if grep -aqE "^(FAIL|ERROR) |^epd_runner:" "$epd_log"; then
        failed=1
    else
        add_summary PASS epd "EPD smoke tier, $(( $(date +%s) - start )) s"
    fi
    while IFS= read -r line; do
        case "$line" in
            WARNING*) add_summary WARN epd "${line#WARNING }" ;;
            NOTE*)    add_summary NOTE epd "${line#NOTE }" ;;
            FAIL*)    add_summary FAIL epd "${line#FAIL }" ;;
            ERROR*)   add_summary FAIL epd "${line#ERROR }" ;;
            epd_runner:*) add_summary FAIL epd "${line#epd_runner: }" ;;
        esac
    done <<EOF
$(grep -aE "^(WARNING|NOTE|FAIL|ERROR) |^epd_runner:" "$epd_log")
EOF
fi

# --- 5. results: record the positions, compare with history ---
if [ -z "$python" ]; then
    :
elif [ ! -s "$checks" ]; then
    add_summary WARN results "the runner wrote no results, nothing recorded"
elif record="$("$python" "$root/test/results.py" record --checks "$checks" --kind smoke --started "$started_at")"; then
    echo "$record"
    comparison="$("$python" "$root/test/results.py" compare)"
    echo "$comparison"
    "$python" "$root/test/results.py" sync --quiet
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
