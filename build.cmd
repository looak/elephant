@echo off
rem build.sh from PowerShell or cmd, same options: build --all. There, build.sh itself only opens in a Git Bash window of
rem its own, which closes when it's done, and "bash" can be WSL's, which builds a Linux binary.
setlocal
for /f "delims=" %%g in ('where git.exe 2^>nul') do if exist "%%~dpg..\bin\bash.exe" set "gitbash=%%~dpg..\bin\bash.exe"
if not defined gitbash (
    echo Git Bash not found. Install Git for Windows: https://git-scm.com/download/win 1>&2
    exit /b 1
)
"%gitbash%" "%~dp0build.sh" %*
exit /b %errorlevel%
