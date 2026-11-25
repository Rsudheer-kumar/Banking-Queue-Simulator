@echo off
title SmartDesk Bank - Frontend Server
cd /d "%~dp0\frontend"
echo ===================================================
echo  Starting SmartDesk Bank Frontend Server
echo  Port: 5500 (http://localhost:5500)
echo ===================================================
python -m http.server 5500
pause
