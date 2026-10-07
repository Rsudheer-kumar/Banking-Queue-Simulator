@echo off
title Banking Queue Simulator - DSA Unit Tests
cd /d "%~dp0\backend"
echo ===================================================
echo  Compiling and Running C++ DSA Unit Tests
echo ===================================================
g++ -std=c++14 -O2 -I include tests/queue_manager_tests.cpp src/Desk.cpp src/QueueManager.cpp src/Bank.cpp -o tests/queue_tests.exe
if errorlevel 1 (
    echo Compilation failed!
    pause
    exit /b 1
)
tests\queue_tests.exe
pause
