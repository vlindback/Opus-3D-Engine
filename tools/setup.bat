@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

echo =============================
echo Setting up Opus3D environment
echo =============================

REM -------------------------------------
REM Ensure VS Build Tools are installed
REM -------------------------------------
where cl >nul 2>&1
IF ERRORLEVEL 1 (
    echo [ERROR] Visual Studio Build Tools not found.
    echo Install from:
    echo https://aka.ms/vs/17/release/vs_BuildTools.exe
    echo Required components:
    echo - MSVC v143 toolset
    echo - Windows SDK
    exit /b 1
)
echo MSVC toolchain found.

REM -------------------------------------
REM Check Meson & Ninja
REM -------------------------------------
where meson >nul 2>&1
IF ERRORLEVEL 1 (
    echo Meson not found - installing...
    python -m pip install --user meson ninja
    if ERRORLEVEL 1 (
        echo [ERROR] Failed to install Meson/Ninja.
        exit /b 1
    )
) else (
    echo Meson is already installed.
)

REM -------------------------------------
REM Check Vulkan SDK
REM -------------------------------------
echo Checking Vulkan SDK...
if not defined VULKAN_SDK (
    echo [ERROR] Vulkan SDK not found!
    echo Download from: https://vulkan.lunarg.com/sdk/home
    exit /b 1
)
echo Vulkan SDK detected at "%VULKAN_SDK%"

REM -------------------------------------
REM Install WrapDB dependencies
REM -------------------------------------
echo Installing Meson wrap dependencies...
meson wrap install glm
meson wrap install xxhash

REM -------------------------------------
REM Configure VSCode Launch for Windows
REM -------------------------------------
echo Configuring VSCode launch settings...
copy /Y ".vscode\launch-windows.json" ".vscode\launch.json" >nul
echo Launch config ready.

echo ====================================
echo Opus3D is ready to build and debug!
echo Open in VS Code and press F5 :)
echo ====================================

ENDLOCAL
exit /b 0
