@echo off
REM ============================================================================
REM Deadlock Prevention & Recovery System - Frontend Dev Server
REM ============================================================================
echo Starting Deadlock Simulator Frontend Server...
echo Directory: frontend\
echo Address:   http://localhost:8000
echo.

start "" "http://localhost:8000"
python -m http.server 8000 --directory frontend
pause
