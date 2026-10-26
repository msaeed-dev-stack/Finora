@echo off
rem =================================================================
rem  Finora build script for Windows
rem
rem  Builds Finora with the Qt 6.12 MinGW tools, then runs the tests.
rem    build.bat           Debug build   (build\debug\Finora.exe)
rem    build.bat release   Release build (build\release\Finora.exe)
rem =================================================================
setlocal

set "PRESET=debug"
if /I "%~1"=="release" set "PRESET=release"

rem Put Qt's CMake, Ninja and compiler first, ahead of any older compiler.
set "PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;%PATH%"

echo === 1/3 Configuring (%PRESET%) ===
cmake --preset %PRESET%
if errorlevel 1 goto failed

echo === 2/3 Building (%PRESET%) ===
cmake --build --preset %PRESET%
if errorlevel 1 goto failed

echo === 3/3 Running tests (%PRESET%) ===
ctest --preset %PRESET%
if errorlevel 1 goto failed

echo.
echo Build and tests succeeded.
echo Start Finora with:  build\%PRESET%\Finora.exe
exit /b 0

:failed
echo.
echo Build FAILED. Read the messages above to find the problem.
exit /b 1
