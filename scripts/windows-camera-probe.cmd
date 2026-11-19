@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
if not exist "camera-logs" mkdir "camera-logs"
set "report=camera-logs\probe-%RANDOM%-%RANDOM%.txt"
echo Checking camera indices 0 through 3. Please wait.
"%~dp0railflow_camera_preview.exe" --probe > "%report%" 2>&1
set "result=%ERRORLEVEL%"
type "%report%"
echo EXIT=%result%
>> "%report%" echo EXIT=%result%
echo Log saved to "%report%"
pause
exit /b %result%
