@echo off
echo Setting up MSVC environment...

rem --- Find VSROOT using vswhere ---
set "VSWHERE="

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
) else if exist "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" (
    set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
)

if "%VSWHERE%"=="" (
    echo ERROR: Could not locate vswhere.exe
    goto :EOF
)

for /f "tokens=*" %%i in ('"%VSWHERE%" -latest -property installationPath') do set "VSROOT=%%i"

if "%VSROOT%"=="" (
    echo ERROR: Visual Studio installation not found.
    goto :EOF
)

rem --- Crucial Step: Call vcvars64.bat ---
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"

echo MSVC x64 environment set up successfully.