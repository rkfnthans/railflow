@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
if not exist "camera-logs" mkdir "camera-logs"
set "report=camera-logs\display-test-%RANDOM%-%RANDOM%.txt"
echo Showing a generated test pattern for five seconds.
"%~dp0railflow_camera_preview.exe" --test-pattern --frames 150 > "%report%" 2>&1
set "result=%ERRORLEVEL%"
type "%report%"
echo EXIT=%result%
>> "%report%" echo EXIT=%result%
echo Log saved to "%report%"
pause
exit /b %result%
