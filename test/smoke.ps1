# Runs test/smoke.sh with Git's bash, the local sanity check before a merge: unit tests, bench, ElephantSuite, phant-indexer, the EPD smoke
# tier. Arguments are passed on, e.g. .\test\smoke.ps1 out\build\x64-release
$bash = Join-Path $env:ProgramFiles 'Git\bin\bash.exe'
if (-not (Test-Path $bash)) {
    Write-Error "Git bash not found at $bash, run test/smoke.sh from a bash shell instead"
    exit 2
}
& $bash (Join-Path $PSScriptRoot 'smoke.sh') @args
exit $LASTEXITCODE
