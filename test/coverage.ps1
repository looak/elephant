# Test coverage of the unit tests with clang-cl: configures build-cov with ELEPHANT_COVERAGE (Debug, phant-indexer on,
# ElephantSuite off), builds ElephantTest, runs ctest -L unit, merges the profiles and writes an HTML report to
# build-cov/coverage/index.html, then prints llvm-cov's summary. Sources under test/, third_party/ & _deps/ are left
# out. Run it from any shell, it loads Visual Studio's developer environment when clang-cl isn't on the path.
#
#   .\test\coverage.ps1 [build dir]     default: build-cov in the repository
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$build = if ($args.Count -gt 0) { $args[0] } else { Join-Path $root 'build-cov' }
$report = Join-Path $build 'coverage'

if (-not (Get-Command clang-cl -ErrorAction SilentlyContinue) -or -not $env:INCLUDE) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Llvm.Clang -property installationPath
    if (-not $vs) {
        Write-Error "Visual Studio with its clang tools not found, install 'C++ Clang tools for Windows'"
        exit 2
    }
    $env:PATH = (Split-Path $vswhere) + ";$env:PATH" # the developer shell asks vswhere too
    & (Join-Path $vs 'Common7\Tools\Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
    $env:PATH = (Join-Path $vs 'VC\Tools\Llvm\x64\bin') + ";$env:PATH"
}

# llvm-profdata & llvm-cov from clang-cl's own folder, the profile format follows the compiler.
$llvm = Split-Path (Get-Command clang-cl).Source -Parent
$profdata = Join-Path $llvm 'llvm-profdata.exe'
$cov = Join-Path $llvm 'llvm-cov.exe'

cmake -S $root -B $build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl `
    -DELEPHANT_COVERAGE=ON -DELEPHANT_BUILD_TEST_SUITE=OFF -DELEPHANT_BUILD_INDEXER=ON -DELEPHANT_BUILD_TUNER=OFF
if ($LASTEXITCODE) { exit $LASTEXITCODE }
cmake --build $build --target ElephantTest
if ($LASTEXITCODE) { exit $LASTEXITCODE }

# ctest runs each test in a process of its own, %8m has them merge into a pool of 8 profiles as they exit.
$profiles = Join-Path $build 'profiles'
Remove-Item -Recurse -Force $profiles, $report -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $profiles | Out-Null
$env:LLVM_PROFILE_FILE = Join-Path $profiles 'elephant-%8m.profraw'
ctest --test-dir $build -L unit -j $env:NUMBER_OF_PROCESSORS --output-on-failure
$tests = $LASTEXITCODE
Remove-Item Env:LLVM_PROFILE_FILE

$merged = Join-Path $build 'elephant.profdata'
& $profdata merge -sparse -o $merged (Get-ChildItem $profiles -Filter *.profraw).FullName
if ($LASTEXITCODE) { exit $LASTEXITCODE }

$binary = Get-ChildItem $build -Recurse -Filter ElephantTest.exe | Where-Object FullName -notmatch 'CMakeFiles' | Select-Object -First 1
$ignore = '[\\/](test|third_party|_deps)[\\/]'
& $cov show $binary.FullName "-instr-profile=$merged" "-ignore-filename-regex=$ignore" -format=html `
    "-output-dir=$report" -show-line-counts-or-regions -show-branches=count
if ($LASTEXITCODE) { exit $LASTEXITCODE }
& $cov report $binary.FullName "-instr-profile=$merged" "-ignore-filename-regex=$ignore"

Write-Host "coverage: report in $(Join-Path $report 'index.html')"
if ($tests) { Write-Host "coverage: unit tests failed, the report covers the tests that ran" -ForegroundColor Red }
exit $tests
