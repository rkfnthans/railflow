@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "packageDir=%~dp0"
if not exist "%packageDir%scripts\launch-windows.cmd" for %%I in ("%~dp0..") do set "packageDir=%%~fI\"
cd /d "%packageDir%"
if not exist "camera-logs" mkdir "camera-logs"
set "report=camera-logs\display-test-%RANDOM%-%RANDOM%.txt"
echo Showing a generated test pattern for five seconds.
set "RAILFLOW_PROGRAM=railflow_camera_preview"
call "%packageDir%scripts\launch-windows.cmd" --test-pattern --frames 150 > "%report%" 2>&1
set "result=%ERRORLEVEL%"
type "%report%"
echo EXIT=%result%
>> "%report%" echo EXIT=%result%
echo Log saved to "%report%"
pause
exit /b %result%
