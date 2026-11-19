@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
if errorlevel 1 exit /b 1
if not "%~1"=="" (
    set "RAILFLOW_PROGRAM=railflow"
    "%~dp0scripts\launch-windows.cmd" %*
    exit /b
)

:menu
echo.
echo RailFlow
echo 1. Mock demo (3 frames and CSV)
echo 2. Train layout
echo 3. Camera preview
echo 4. Find cameras
echo 5. Test display
echo 6. Serial raw ADC monitor
echo 7. Calibration recorder
echo 0. Exit
choice /C 12345670 /N /M "Select: "
set "selected=%ERRORLEVEL%"
if "%selected%"=="1" goto demo
if "%selected%"=="2" goto layout
if "%selected%"=="3" goto camera
if "%selected%"=="4" goto probe
if "%selected%"=="5" goto display
if "%selected%"=="6" goto serial
if "%selected%"=="7" goto calibration
exit /b 0

:demo
set "RAILFLOW_PROGRAM=railflow"
call :run --demo --frames 3 --period-ms 100
goto menu

:layout
set "RAILFLOW_PROGRAM=railflow"
call :run --layout
goto menu

:camera
set "cameraIndex=0"
set /p "cameraIndex=Camera index [0]: "
set "RAILFLOW_PROGRAM=railflow_camera_preview"
call :run --camera "%cameraIndex%"
goto menu

:probe
set "RAILFLOW_PROGRAM=railflow_camera_preview"
call :run --probe
goto menu

:display
set "RAILFLOW_PROGRAM=railflow_camera_preview"
call :run --test-pattern --frames 150
goto menu

:serial
call :port
if not defined port goto menu
set "RAILFLOW_PROGRAM=railflow_serial_monitor"
call :run "%port%" --baud 115200 --record data\raw_adc.csv
goto menu

:calibration
call :port
if not defined port goto menu
set "vehicle="
set "weight="
set "position="
set "trial="
set /p "vehicle=Vehicle (CAR1/CAR2/CAR3): "
set /p "weight=Measured reference weight in grams: "
set /p "position=Position (CENTER/LEFT/RIGHT/FRONT/REAR): "
set /p "trial=Trial number: "
if not defined vehicle goto menu
if not defined weight goto menu
if not defined position goto menu
if not defined trial goto menu
if not exist data\calibration mkdir data\calibration
set "dataset=data\calibration\session-%RANDOM%-%RANDOM%.csv"
set "RAILFLOW_PROGRAM=railflow_calibration_record"
call :run "%port%" --vehicle "%vehicle%" --weight-g "%weight%" --position "%position%" --trial "%trial%" --output "%dataset%"
goto menu

:port
echo Available serial ports:
powershell.exe -NoProfile -Command "[System.IO.Ports.SerialPort]::GetPortNames()"
set "port="
set /p "port=Serial port (COM3, COM10; empty cancels): "
exit /b 0

:run
if not exist logs mkdir logs
set "report=logs\run-%RANDOM%-%RANDOM%.txt"
call "%~dp0scripts\launch-windows.cmd" %* > "%report%" 2>&1
set "result=%ERRORLEVEL%"
type "%report%"
>> "%report%" echo EXIT=%result%
echo EXIT=%result%
echo Log: %report%
exit /b 0
