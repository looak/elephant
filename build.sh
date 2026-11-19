#!/usr/bin/env bash
# Builds Elephant Gambit into dist/, on any machine with bash: Linux, macOS, Git Bash or MSYS2 on Windows. Checks the
# toolchain first and says what's missing. It's a convenience, plain cmake and ob_build/makefile keep working without it.
#
#   ./build.sh                  phant-uci, the UCI engine, with CMake and this machine's compiler
#   ./build.sh --reference      phant-uci with g++ & the makefile: what OpenBench & releases build, where bench comes from
#   ./build.sh --arch avx2      a level other machines can run: x86-64, sse42, avx2, or native (the default)
#   ./build.sh --cli --tuner    other products: --uci --cli --tuner --indexer --tests, or --all
#   ./build.sh --dev            assertions & exceptions on. Implied by --indexer, --tests & --all. The name says -dev
#   ./build.sh --debug          a Debug build in dist/debug
#   ./build.sh --clean          this build's objects first. Add --deps to fetch the dependencies again as well
#   ./build.sh --check          only check the toolchain
#
# Objects go in .build/<os>-<flavour>-<arch>, fetched sources in .build/deps, what you run in dist/.
set -u

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

die() { printf '%s\n' "$*" >&2; exit 1; }
row() { printf '%-11s%s\n' "$1" "$2"; }

usage() { sed -n '2,/^set -u/p' "${BASH_SOURCE[0]}" | sed -e '/^set -u/d' -e 's/^# \{0,1\}//'; exit "${1:-0}"; }

want_uci=0 want_cli=0 want_tuner=0 want_indexer=0 want_tests=0
arch=native reference=0 dev=0 debug=0 clean=0 clean_deps=0 check_only=0
while [ $# -gt 0 ]; do
    case "$1" in
        --uci) want_uci=1 ;;
        --cli) want_cli=1 ;;
        --tuner) want_tuner=1 ;;
        --indexer) want_indexer=1 ;;
        --tests) want_tests=1 ;;
        --all) want_uci=1 want_cli=1 want_tuner=1 want_indexer=1 want_tests=1 ;;
        --reference) reference=1 ;;
        --arch) [ $# -ge 2 ] || die "--arch needs a level: x86-64, sse42, avx2 or native"; arch="$2"; shift ;;
        --dev) dev=1 ;;
        --debug) debug=1 ;;
        --clean) clean=1 ;;
        --deps) clean_deps=1 ;;
        --check) check_only=1 ;;
        -h|--help) usage ;;
        *) printf 'unknown option: %s\n\n' "$1" >&2; usage 2 ;;
    esac
    shift
done
[ "$want_cli$want_tuner$want_indexer$want_tests" = 0000 ] && want_uci=1

case "$arch" in
    native) march=native;     arch_name=x86-64-native ;;
    x86-64) march=x86-64;     arch_name=x86-64 ;;
    sse42)  march=x86-64-v2;  arch_name=x86-64-sse42-popcnt ;;
    avx2)   march=x86-64-v3;  arch_name=x86-64-avx2-bmi2 ;;
    *) die "unknown --arch $arch: x86-64, sse42, avx2 or native" ;;
esac

# --- the platform. On Windows "bash" can be Git's or WSL's: the same command gives a different binary, so it's said first.
kernel="$(uname -s)"
ext=""
case "$kernel" in
    MINGW*|MSYS*|CYGWIN*) os=win; ext=.exe; shown="windows ($kernel)" ;;
    Darwin) os=mac; shown=macos ;;
    Linux)
        os=linux; shown=linux
        case "$(uname -r)" in *[Mm]icrosoft*) shown="linux (WSL, this builds a Linux binary)" ;; esac ;;
    *) os="$(printf '%s' "$kernel" | tr '[:upper:]' '[:lower:]')"; shown="$os" ;;
esac
row platform "$shown"

# --- the toolchain ---
have() { command -v "$1" >/dev/null 2>&1; }
first_line_version() { "$@" 2>&1 | head -n 1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n 1; }

check_cmake() {
    have cmake || die "cmake not found. Install CMake 3.14 or newer: https://cmake.org/download/"
    row cmake "$(first_line_version cmake --version)"
    if have ninja; then row ninja "$(first_line_version ninja --version)"; else row ninja "not found, using CMake's default generator"; fi
    # on Windows a compiler usually isn't on the path outside a developer prompt, CMake finds Visual Studio itself.
    if [ "$os" != win ] && ! have c++ && ! have g++ && ! have clang++; then
        die "no C++ compiler found. Install g++ 13 or newer, or clang++ 17 or newer."
    fi
}

# the code needs C++20's <format>, which is what's missing from an old compiler.
check_reference() {
    local missing=""
    have g++ || missing="g++"
    have make || missing="$missing make"
    if [ -n "$missing" ]; then
        printf '%s not found.\n' "$missing" >&2
        case "$os" in
            win) printf '  --reference needs g++ and make. Install MSYS2 (pacman -S make mingw-w64-ucrt-x86_64-gcc)\n  and build from its UCRT64 shell, or run this from WSL for a Linux binary.\n' >&2 ;;
            *) printf '  --reference needs g++ 13 or newer and make.\n' >&2 ;;
        esac
        exit 1
    fi
    row g++ "$(first_line_version g++ --version)"
    row make "$(first_line_version make --version)"
    printf '#include <format>\nint main() { return std::format("{}", 1).empty(); }\n' | g++ -std=c++2a -x c++ -fsyntax-only - 2>/dev/null \
        || die "g++ can't compile C++20's <format>, it needs to be version 13 or newer."
}

if [ "$reference" = 1 ]; then
    [ "$want_cli$want_tuner$want_indexer$want_tests" = 0000 ] || die "--reference only builds phant-uci, the makefile has nothing else."
    [ "$dev$debug" = 00 ] || die "--reference is the release build, --dev and --debug don't go with it."
    check_reference
else
    check_cmake
fi
[ "$check_only" = 1 ] && { row toolchain ok; exit 0; }

mkdir -p "$root/dist"
jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

[ "$clean_deps" = 1 ] && rm -rf "$root/.build/deps"

# --- the makefile ---
if [ "$reference" = 1 ]; then
    version="$(head -n 1 "$root/version.txt" | tr -d '\r[:space:]')"
    name="phant-uci-$version-$os-$arch_name$ext"
    # objects by level: the makefile doesn't rebuild when only ARCH changes.
    objects="../.build/make-$os-$arch"
    [ "$clean" = 1 ] && rm -rf "$root/.build/make-$os-$arch"
    row building "$name, g++ reference build"
    make -s -C "$root/ob_build" -j"$jobs" ARCH="$march" BUILD_DIR="$objects" EXE="../dist/$name" || die "the build failed"
    row built "dist/$name"
    exit 0
fi

# --- CMake ---
flavour=release assertions=OFF
if [ "$debug" = 1 ]; then
    flavour=debug assertions=ON
elif [ "$dev$want_indexer$want_tests" != 000 ]; then
    flavour=dev assertions=ON
fi
config=Release
[ "$debug" = 1 ] && config=Debug

on_off() { [ "$1" = 1 ] && echo ON || echo OFF; }
tree="$root/.build/$os-$flavour-$arch"
[ "$clean" = 1 ] && rm -rf "$tree"

generator=""
# Ninja needs a compiler on the path, in a Windows shell outside a developer prompt that's Visual Studio's generator.
if [ ! -f "$tree/CMakeCache.txt" ] && have ninja; then
    if [ "$os" != win ] || have cl || have clang-cl || have g++ || have clang++; then
        generator="-GNinja"
    fi
fi

exceptions=ON
[ "$flavour" = release ] && exceptions=OFF
options=(-S "$root" -B "$tree" -DCMAKE_BUILD_TYPE="$config" -DELEPHANT_ARCH="$arch")
options+=(-DASSERTIONS_ENABLED="$assertions" -DEXCEPTIONS_ENABLED="$exceptions")
options+=(-DELEPHANT_BUILD_TESTS="$(on_off "$want_tests")" -DELEPHANT_BUILD_TEST_SUITE="$(on_off "$want_tests")")
options+=(-DELEPHANT_BUILD_TUNER="$(on_off "$want_tuner")" -DELEPHANT_BUILD_INDEXER="$(on_off "$want_indexer")")

row configure ".build/$os-$flavour-$arch"
log="$tree/configure.log"
mkdir -p "$tree"
if ! cmake "${options[@]}" ${generator:+"$generator"} > "$log" 2>&1; then
    tail -n 25 "$log" >&2
    die "configuring failed, the whole log is in $log"
fi
compiler="$(grep -m1 '^CMAKE_CXX_COMPILER:' "$tree/CMakeCache.txt" | cut -d= -f2-)"
row compiler "${compiler:-chosen by the generator}"

targets=()
[ "$want_uci" = 1 ] && targets+=(phant-uci)
[ "$want_cli" = 1 ] && targets+=(phant-cli)
[ "$want_tuner" = 1 ] && targets+=(phant-tuner)
[ "$want_indexer" = 1 ] && targets+=(phant-indexer)
[ "$want_tests" = 1 ] && targets+=(phant-test phant-suite)

row building "${targets[*]}"
cmake --build "$tree" --config "$config" --parallel "$jobs" --target "${targets[@]}" || die "the build failed"

out="$root/dist"
[ "$debug" = 1 ] && out="$root/dist/debug"
for product in "${targets[@]}"; do
    case "$product" in
        phant-uci) file="$(ls -t "$out"/phant-uci-* 2>/dev/null | grep -v '\.pdb$' | head -n 1)" ;;
        *) file="$out/$product$ext" ;;
    esac
    row built "${file#"$root"/}"
done
