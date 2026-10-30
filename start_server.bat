@echo off
REM ============================================================================
REM Deadlock Prevention and Recovery System - C++ HTTP Server & Frontend
REM ============================================================================
echo Starting Deadlock C++ Native HTTP Server & Simulation Engine...
echo Listening on port 8080
echo Serving Web Assets from: frontend/
echo REST API Base: http://localhost:8080/api
echo.

set "PATH=C:\Program Files\CodeBlocks\MinGW\bin;%PATH%"

if not exist "build\deadlock_server.exe" (
    echo [ERROR] deadlock_server.exe not found. Building now...
    call build.bat
)

start "" "http://localhost:8080"
build\deadlock_server.exe 8080 frontend
pause
