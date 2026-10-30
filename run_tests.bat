@echo off
setlocal
set "PATH=C:\Program Files\CodeBlocks\MinGW\bin;%PATH%"

if not exist build\deadlock_tests.exe (
    echo [INFO] Binaries not found. Running build.bat first...
    call build.bat
)

echo.
echo ========================================================
echo  Executing Deadlock Simulator Unit Tests: Core
echo ========================================================
build\deadlock_tests.exe
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Core unit tests failed!
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================================
echo  Executing Deadlock Simulator Unit Tests: Prevention
echo ========================================================
build\prevention_tests.exe
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Prevention unit tests failed!
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================================
echo  Executing Deadlock Simulator Unit Tests: Detection
echo ========================================================
build\detection_tests.exe
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Detection unit tests failed!
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================================
echo  Executing Deadlock Simulator Unit Tests: Recovery
echo ========================================================
build\recovery_tests.exe
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Recovery unit tests failed!
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================================
echo  Executing Deadlock Simulator Integration Tests: API
echo ========================================================
build\api_tests.exe
if %ERRORLEVEL% neq 0 (
    echo [ERROR] API integration tests failed!
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================================
echo  [ALL TESTS PASSED] 47/47 Unit and Integration Tests Succeeded!
echo ========================================================
