# RailFlow

캡스톤 디자인용 3량 열차 모형의 FSR 모니터링 및 승강장 승차 위치 추천 프로젝트입니다.

- 확정 모형: **3량**, 각 차량 **30 cm(길이) × 15 cm(폭) × 약 15 cm(높이)**
- 센서: 각 차량 길이 방향으로 일렬 **FSR 3개**, 총 **9개**
- 채널: CAR1 C0–C2 / CAR2 C3–C5 / CAR3 C6–C8
- 현재 연동: Arduino Uno + CD74HC4067 + 9채널 RF1 raw ADC, PC 시리얼 모니터 및 보정 데이터 수집
- 혼잡도/승차 위치 데모는 현재 **Mock 데이터 기반**. 실제 하중(g) 추정은 실측 보정 후 연결해야 합니다.

GitHub에서 저장소를 복제하거나 **Code → Download ZIP**으로 받은 뒤, `railflow` 폴더 안에서 실행합니다.

| 컴퓨터 | 시작 | 장비 없이 검사 |
| --- | --- | --- |
| Windows 10/11 x64 | `START_RAILFLOW.cmd` 더블클릭 | `TEST_SYSTEM.cmd` |
| Mac (Apple Silicon·Intel) | `START_RAILFLOW.command` 더블클릭 | `TEST_SYSTEM.command` |

저장소의 `bin/windows-x64`와 `bin/macos`에 두 운영체제용 실행 파일이 함께 들어 있습니다.
시작 파일이 현재 컴퓨터에 맞는 프로그램을 실행합니다. 카메라 확인을 포함한 기본 실행에는
CMake, 컴파일러, OpenCV를 별도로 설치하지 않아도 됩니다.
GitHub에 올릴 때는 `bin`도 함께 포함하고 `build`는 제외합니다.
자세한 내용은 [공통 실행 안내](docs/CROSS_PLATFORM_SETUP.md)를 참고하세요.

Windows 10/11의 빌드와 COM 포트 사용법은 [Windows 실행 안내](docs/WINDOWS_SETUP.md)를 참고하세요.
USB 카메라 연결과 화면 표시는 [Windows 카메라 미리보기](docs/USB_CAMERA_WINDOWS.md)를 참고하세요.

macOS/Linux에서 설계 사양과 채널 배치를 확인하려면 다음을 실행하세요.

```sh
cmake -S . -B build -DRAILFLOW_ENABLE_OPENCV=OFF -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/railflow --layout
```

자세한 사항: [최종 모형 배치](docs/FINAL_MODEL_LAYOUT.md),
[개발 상태](docs/DEVELOPMENT.md),
[하드웨어 배선](docs/FSR_HARDWARE_BRINGUP.md),
[센서 보정](docs/FSR_CALIBRATION.md).
