@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
echo =============================
echo Installing Opus3D Dependencies
echo =============================

REM -------------------------------
REM Check Vulkan SDK
REM -------------------------------
echo Checking Vulkan SDK...
if not defined VULKAN_SDK goto :ERROR_VULKAN_MISSING

echo Vulkan SDK found at "%VULKAN_SDK%"
goto :CONTINUE_VULKAN

:ERROR_VULKAN_MISSING
echo [ERROR] Vulkan SDK not found.
echo -----------------------------------------------------------
echo Please install it from the official LunarG website.
echo Opening the download page now...
echo -----------------------------------------------------------
start "" "https://vulkan.lunarg.com/sdk/home"
echo Please install the SDK, restart your terminal, and run this script again.
exit /b 1

:CONTINUE_VULKAN

echo -------------------------------------
echo Core Dependencies installed successfully!
echo -------------------------------------

REM -------------------------------
REM Meson WrapDB Dependencies
REM -------------------------------
echo =============================
echo Installing Meson WrapDB Dependencies
echo =============================

REM These wrap dependencies will be available in the source tree

REM Fetch glm
echo Fetching glm...
meson wrap install glm

REM Fetch xxhash
echo Fetching xxhash...
meson wrap install xxhash

echo Done installing Meson wrap dependencies.

REM ---------------------------------------
REM Meson Configuration Step
REM ---------------------------------------

REM First argument = target (Editor or Game)
SET TARGET=%1
IF "%TARGET%"=="" SET TARGET=Editor

REM Second argument = config (Debug or Release)
SET CONFIG=%2
IF "%CONFIG%"=="" SET CONFIG=Debug

REM Normalize to lowercase for Meson --buildtype
SET MESON_BUILD_TYPE=%CONFIG%
IF "%CONFIG%"=="Debug" SET MESON_BUILD_TYPE=debug
IF "%CONFIG%"=="Release" SET MESON_BUILD_TYPE=release

SET BUILD_DIR=build/windows-msvc-%TARGET%-%CONFIG%

echo ===========================================
echo Configuring build for %TARGET% - %CONFIG%
echo Build directory: %BUILD_DIR%
echo Meson build type: %MESON_BUILD_TYPE%
echo ===========================================

REM Ensure MSVC env is active FIRST
call ".vscode\msvc-setup.bat"

meson setup "%BUILD_DIR%" ^
    --buildtype=%MESON_BUILD_TYPE% ^
    -Dapp_target=%TARGET% ^
    --vsenv ^
    --wipe

ENDLOCAL
exit /b 0