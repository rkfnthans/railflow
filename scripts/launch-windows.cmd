@echo off
setlocal EnableExtensions DisableDelayedExpansion
if not "%OS%"=="Windows_NT" (
    echo This entry point requires Windows. On Mac, open START_RAILFLOW.command.
    exit /b 1
)
for %%I in ("%~dp0..") do set "root=%%~fI"
if not defined RAILFLOW_PROGRAM set "RAILFLOW_PROGRAM=railflow"
set "validProgram="
for %%P in (railflow railflow_serial_monitor railflow_calibration_record railflow_camera_preview railflow_detect) do if "%RAILFLOW_PROGRAM%"=="%%P" set "validProgram=1"
if not defined validProgram (
    echo Unknown RailFlow program.
    exit /b 2
)
echo RailFlow host: Windows / %PROCESSOR_ARCHITECTURE%
set "binary=%root%\bin\windows-x64\%RAILFLOW_PROGRAM%.exe"
if exist "%binary%" goto ready
set "binary=%root%\%RAILFLOW_PROGRAM%.exe"
if exist "%binary%" goto ready
if not exist "%root%\CMakeLists.txt" (
    echo Missing executable. Extract the entire runtime ZIP.
    exit /b 1
)
where cmake >nul 2>&1
if errorlevel 1 (
    echo Source build requires CMake and Visual Studio C++ Build Tools.
    echo The runtime ZIP includes ready-built programs.
    exit /b 1
)
set "build=%root%\build\native\windows-x64"
set "opencv=OFF"
if "%RAILFLOW_PROGRAM%"=="railflow_camera_preview" set "opencv=ON"
if "%RAILFLOW_PROGRAM%"=="railflow_detect" set "opencv=ON"
cmake -S "%root%" -B "%build%" -DCMAKE_BUILD_TYPE=Release -DRAILFLOW_ENABLE_OPENCV=%opencv% -DRAILFLOW_ENABLE_CAMERA_PREVIEW=ON
if errorlevel 1 exit /b 1
cmake --build "%build%" --config Release --target %RAILFLOW_PROGRAM% --parallel
if errorlevel 1 exit /b 1
set "binary=%build%\Release\%RAILFLOW_PROGRAM%.exe"
if not exist "%binary%" set "binary=%build%\%RAILFLOW_PROGRAM%.exe"

:ready
if not defined RAILFLOW_WORK_DIR set "RAILFLOW_WORK_DIR=%root%"
if not exist "%RAILFLOW_WORK_DIR%\data\logs" mkdir "%RAILFLOW_WORK_DIR%\data\logs"
cd /d "%RAILFLOW_WORK_DIR%"
if errorlevel 1 exit /b 1
"%binary%" %*
exit /b %ERRORLEVEL%
