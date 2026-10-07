@echo off
title Banking Queue Simulator - Frontend Server
cd /d "%~dp0\frontend"
echo ===================================================
echo  Starting Banking Queue Simulator Frontend Server
echo  Port: 5500 (http://localhost:5500)
echo ===================================================
python -m http.server 5500
pause
