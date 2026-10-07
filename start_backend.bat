@echo off
title Banking Queue Simulator - C++ Backend
cd /d "%~dp0\backend"
echo ===================================================
echo  Starting Banking Queue Simulator C++ Backend
echo  Port: 8080 (http://localhost:8080)
echo ===================================================
if not exist "bank_server.exe" (
    echo Compiling bank_server.exe...
    g++ -std=c++14 -O2 -I include -I third_party src/server.cpp src/Desk.cpp src/Bank.cpp src/QueueManager.cpp -lws2_32 -o bank_server.exe
    if errorlevel 1 (
        echo Compilation failed! Please check your C++ compiler.
        pause
        exit /b 1
    )
)
bank_server.exe
pause
