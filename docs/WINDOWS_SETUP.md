# Windows 10/11 x64에서 RailFlow 실행

## 실행 패키지 자동 확인

Windows 실행용 ZIP의 `TEST_WINDOWS.cmd`를 실행하면 ASCII 명령으로 차량 배치, Mock 데모,
CSV 3행 저장, 모니터·보정 기록기의 도움말을 검사한다. 결과는 새 `windows-check-*` 폴더의
`windows-test.txt`와 `data/logs/railflow_log.csv`에 저장된다. 패키지의 기존 데이터와 별도 경로를 사용한다.
검사 파일 원본은 `scripts/windows-check.cmd`이며 실행 패키지에서는 세 `.exe` 옆에 배치한다.

옵션 입력 오류 시 프로그램은 받은 인수를 16진수 바이트로 출력한다. ASCII `--`는 `2d 2d`다.
복사 과정에서 대시 문자가 바뀌었는지 이 값으로 확인할 수 있다.

## 개발 환경

Visual Studio 2022 또는 Visual Studio 2022 Build Tools 설치 관리자에서 **C++를 사용한 데스크톱 개발** 워크로드를 선택합니다. MSVC v143 x64/x86 도구, Windows 10 또는 11 SDK, Windows용 C++ CMake 도구를 포함합니다. [Microsoft Build Tools 구성](https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-build-tools?view=vs-2022)

설치 후 **Developer PowerShell for VS 2022**를 열고 `CMakeLists.txt`가 있는 RailFlow 루트로 이동합니다. Visual Studio 2022 generator를 사용하려면 CMake 3.21 이상이 필요합니다. [CMake Visual Studio 2022 generator](https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2017%202022.html)

## 빌드, 테스트, Mock 데모

```powershell
cmake --version
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 -DRAILFLOW_ENABLE_OPENCV=OFF -DBUILD_TESTING=ON
cmake --build build-win --config Release --parallel
ctest --test-dir build-win -C Release --output-on-failure

.\build-win\Release\railflow.exe --layout
.\build-win\Release\railflow.exe --demo --frames 3 --period-ms 100 --no-log
```

위 구성은 OpenCV와 Arduino 없이 실행할 수 있습니다. `--layout`은 25 × 15 × 약 15 cm인 3량과 길이 방향 FSR 9개의 채널을 표시합니다. `--demo`는 Mock 차량 하중과 승객 위치로 차량별 혼잡도, Zone별 인원수, 추천 승차 위치를 계산합니다. Mock 결과를 `data/logs/railflow_log.csv`에 추가하려면 `--no-log`를 빼고 실행합니다. Windows 테스트의 `Win32SerialTransportTests`는 가상 COM 입력으로 두 시리얼 도구의 처리 및 CSV 기록을 검증합니다.

## Arduino COM 포트 및 raw ADC 모니터

Arduino Uno에 [RF1 펌웨어](../firmware/railflow_fsr_uno/railflow_fsr_uno.ino)를 올리고 USB 데이터 케이블로 연결합니다. **장치 관리자 → 포트(COM 및 LPT)**에서 보드를 연결했을 때 나타나는 번호를 확인합니다. PowerShell에서도 연결된 포트 이름을 확인할 수 있습니다.

```powershell
Get-PnpDevice -Class Ports -PresentOnly | Select-Object FriendlyName
```

Arduino IDE의 Serial Monitor와 같은 포트를 사용하는 다른 프로그램을 닫습니다. 아래 `COM10`을 실제 번호로 바꿉니다. `COM3`, `COM4`, `COM10` 이상을 모두 같은 형태로 지정합니다.

```powershell
.\build-win\Release\railflow_serial_monitor.exe COM10 --baud 115200 --timeout-ms 100 --frames 10 --record data\raw_adc.csv
```

RF1 기본 설정은 115200 baud, 8N1이며 흐름 제어를 끕니다. 모니터는 CAR1 C0–C2, CAR2 C3–C5, CAR3 C6–C8의 ADC 값(0–1023)을 표시하고 `--record` 경로에 추가합니다. `--frames`를 빼면 Ctrl+C를 누를 때까지 수신합니다. `--timeout-ms`는 한 줄 읽기의 대기 한도입니다.

## FSR 보정 데이터 기록

모니터를 종료해 COM 포트를 비운 뒤, 예를 들어 CAR1 중앙에 실측 500 g 기준추를 올려 기록합니다.

```powershell
.\build-win\Release\railflow_calibration_record.exe COM10 --vehicle CAR1 --weight-g 500 --position CENTER --trial 1 --settle-ms 1000 --duration-sec 5 --output data\calibration\car1.csv
```

기록기는 기준추 라벨과 `adc0`~`adc8`을 함께 저장하고 선택한 차량의 raw ADC 통계를 출력합니다. `--samples N`을 지정하면 샘플 수와 측정 시간 중 먼저 도달한 조건에서 종료합니다. Ctrl+C로 종료해도 이미 기록된 행은 보존됩니다. 보정 곡선은 실제 실험으로 산출해야 하며 현재 승차 안내는 Mock 입력을 사용합니다. [보정 데이터 수집 절차](FSR_CALIBRATION.md)

## 문제 해결

| 증상 | 조치 |
| --- | --- |
| Visual Studio generator 또는 컴파일러를 찾지 못함 | `cmake --version`과 `cmake --help`를 확인합니다. VS 2022 워크로드에서 MSVC x64/x86 및 Windows SDK를 설치합니다. 다른 generator로 전환할 때는 새 빌드 디렉터리를 사용합니다. |
| `.exe`를 찾지 못함 | Visual Studio Release 빌드는 `build-win\Release`에 생성됩니다. 프로젝트 루트에서 `--config Release`로 빌드했는지 확인합니다. |
| 한글 글리프가 표시되지 않음 | 한글을 표시할 수 있는 글꼴이 설정된 Windows Terminal에서 실행합니다. 파일이나 파이프로 보낸 출력은 UTF-8로 읽습니다. |
| COM 포트가 목록에 없음 | 데이터 전송이 가능한 USB 케이블과 보드 드라이버를 확인하고 장치 관리자에서 번호를 다시 확인합니다. |
| 포트를 열 때 액세스 거부 | Arduino IDE의 Serial Monitor, 다른 RailFlow 프로세스, 같은 포트를 점유한 프로그램을 닫습니다. |
| RF1 데이터가 없거나 `Invalid RF1 line`이 반복됨 | COM 번호, 펌웨어, 115200 baud를 확인합니다. 포트를 열면 Uno가 재시작될 수 있으므로 보정 기록의 `--settle-ms`를 늘려 확인합니다. |
| 실행 중 USB 분리 또는 I/O 오류 | 프로그램이 포트를 해제하고 오류로 종료합니다. 보드를 다시 연결하고 변경된 COM 번호를 확인한 뒤 명령을 다시 실행합니다. 보정 실험은 새 세션 또는 파일에서 재개합니다. |
| CSV 열기 또는 쓰기 실패 | 쓰기 가능한 위치를 사용하고 파일을 점유한 프로그램을 닫습니다. raw ADC와 보정 CSV에는 각각의 경로를 지정합니다. 모니터는 출력 상위 디렉터리가 미리 존재해야 합니다. |
