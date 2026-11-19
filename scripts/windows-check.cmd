@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "packageDir=%~dp0"
if not exist "%packageDir%railflow.exe" (
    echo Place this file beside the three RailFlow .exe files, then run it again.
    pause
    exit /b 1
)

:newDirectory
set "checkDir=%packageDir%windows-check-%RANDOM%-%RANDOM%"
if exist "%checkDir%" goto newDirectory
mkdir "%checkDir%\data\logs"
if errorlevel 1 (
    echo Could not create the test directory. Extract the package to a writable folder.
    pause
    exit /b 1
)
set "report=%checkDir%\windows-test.txt"
set "failures=0"
set "checks=0"
> "%report%" echo RailFlow Windows check, package revision 2026-11-19-r2
>> "%report%" ver
>> "%report%" echo Package directory: %packageDir%
>> "%report%" echo Test directory: %checkDir%
for %%F in (railflow.exe railflow_serial_monitor.exe railflow_calibration_record.exe) do (
    >> "%report%" echo SHA256 %%F
    certutil.exe -hashfile "%packageDir%%%F" SHA256 >> "%report%" 2>&1
)

pushd "%checkDir%"
call :run "%packageDir%railflow.exe" --layout
call :run "%packageDir%railflow.exe" --demo --frames 3 --period-ms 100 --no-log
call :run "%packageDir%railflow.exe" --demo --frames 3 --period-ms 100
call :run "%packageDir%railflow_serial_monitor.exe" --help
call :run "%packageDir%railflow_calibration_record.exe" --help
call :checkCsv
popd

>> "%report%" echo Checks=%checks% Failures=%failures%
echo.
echo Checks=%checks% Failures=%failures%
echo Send the report and CSV from this directory:
echo "%checkDir%"
echo.
pause
exit /b %failures%

:run
echo Running %*
>> "%report%" echo COMMAND %*
%* >> "%report%" 2>&1
set "lastExit=%ERRORLEVEL%"
call :recordResult
exit /b 0

:checkCsv
>> "%report%" echo Checking Mock CSV row count and recommended_zone
set "lastExit=1"
if not exist "data\logs\railflow_log.csv" (
    >> "%report%" echo FAIL: CSV was not created
    call :recordResult
    exit /b 0
)
set "csvLines=0"
set "p3Rows=0"
for /f "usebackq delims=" %%L in ("data\logs\railflow_log.csv") do set /a csvLines+=1 >nul
for /f "usebackq skip=1 tokens=17 delims=," %%Z in ("data\logs\railflow_log.csv") do if "%%Z"=="P3" set /a p3Rows+=1 >nul
if "%csvLines%"=="4" if "%p3Rows%"=="3" set "lastExit=0"
>> "%report%" echo CSV lines=%csvLines% P3 rows=%p3Rows%
type "data\logs\railflow_log.csv" >> "%report%"
call :recordResult
exit /b 0

:recordResult
>> "%report%" echo EXIT=%lastExit%
echo EXIT=%lastExit%
set /a checks+=1 >nul
if not "%lastExit%"=="0" set /a failures+=1 >nul
exit /b 0
