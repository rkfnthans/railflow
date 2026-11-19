@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
if errorlevel 1 exit /b 1
echo RailFlow USB camera preview
echo Try index 1 if index 0 shows the laptop's built-in camera.
choice /C 0123 /N /M "Camera index [0,1,2,3]: "
set /a cameraIndex=%ERRORLEVEL%-1 >nul
if %cameraIndex% LSS 0 exit /b 1
if %cameraIndex% GTR 3 exit /b 1
choice /C ADM /N /M "Backend [A=auto, D=DirectShow, M=Media Foundation]: "
set "backendChoice=%ERRORLEVEL%"
if %backendChoice% LSS 1 exit /b 1
if %backendChoice% GTR 3 exit /b 1
set "backend=auto"
if "%backendChoice%"=="3" set "backend=msmf"
if "%backendChoice%"=="2" set "backend=dshow"
if not exist "camera-logs" mkdir "camera-logs"
set "report=camera-logs\preview-%RANDOM%-%RANDOM%.txt"
echo Starting camera %cameraIndex% using %backend%.
echo Close the preview with Q, Esc or the window close button.
"%~dp0railflow_camera_preview.exe" --camera %cameraIndex% --backend %backend% > "%report%" 2>&1
set "result=%ERRORLEVEL%"
type "%report%"
echo EXIT=%result%
>> "%report%" echo EXIT=%result%
echo Log saved to "%report%"
pause
exit /b %result%
