@echo off
setlocal
echo ========================================================
echo  Building Deadlock Simulator (C++ Engine and Bridge)
echo ========================================================

set "PATH=C:\Program Files\CodeBlocks\MinGW\bin;%PATH%"

if not exist build (
    mkdir build
)

cmake -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/Program Files/CodeBlocks/MinGW/bin/g++.exe" -DCMAKE_MAKE_PROGRAM="C:/Program Files/CodeBlocks/MinGW/bin/mingw32-make.exe"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed.
    exit /b %ERRORLEVEL%
)

cmake --build build
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build compilation failed.
    exit /b %ERRORLEVEL%
)

echo.
echo [SUCCESS] Build completed successfully!
echo Executables created:
echo   - build\deadlock_sim.exe    (Interactive Simulator CLI)
echo   - build\deadlock_server.exe (HTTP REST API and Frontend Server)
echo   - build\deadlock_tests.exe  (Core Engine Unit Tests)
echo   - build\prevention_tests.exe (Prevention Policy Unit Tests)
echo   - build\detection_tests.exe  (Deadlock Detection Unit Tests)
echo   - build\recovery_tests.exe   (Deadlock Recovery Unit Tests)
echo   - build\api_tests.exe        (API and Bridge Integration Tests)
echo.
