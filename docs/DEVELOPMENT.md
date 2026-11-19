# RailFlow 개발 상태

확정된 3량(차량당 30×15×약15cm)과 길이방향 FSR 일렬 3개 배치는 [FINAL_MODEL_LAYOUT.md](FINAL_MODEL_LAYOUT.md)를 참고한다.

## 빌드와 실행

CMake 3.20 이상, C++20 컴파일러가 필요하다. 하드웨어 없이 실행 가능하다.
Windows 환경은 [Windows 실행 안내](WINDOWS_SETUP.md)를 참고한다.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/railflow
./build/railflow --layout  # 확정 차량·센서 구성 확인
./build/railflow --demo --frames 3 --period-ms 100
./build/railflow --demo --no-log
```

기본 실행은 기존처럼 한 프레임을 출력하고 CSV 한 행을 추가한다.
`--demo`는 Mock 입력으로 반복 실행하며 Ctrl+C/SIGTERM으로 종료한다.
`--frames N`은 실행 프레임 수(0이면 계속), `--period-ms N`은 주기다.
주기는 1~60000ms, 프레임 수는 0~1000000이다. 두 옵션은 `--demo`와 사용한다.
demo에서도 기본적으로 매 프레임 CSV를 append하므로 장시간 실행에는
`--no-log`를 사용할 수 있다. CSV 상대 경로 때문에 프로젝트 루트에서 실행한다.

## 현재 연결

`MockInputSource → InputFrame → RailFlowEngine → ConsoleReporter / Logger`

- `InputSource::read()`는 차량 3량 하중(g)과 canonical 승객 좌표를 반환한다.
- 기존 `createMockVehicles()`와 `createMockPassengers()`도 유지한다.
- 읽기 실패는 예외로 전달한다. 빈 승객 목록은 유효한 무인 상태다.
- `RailFlowEngine`은 각 프레임에 기존 ZoneManager, VehicleLoadEstimator,
  PlatformCrowdEstimator, GuidanceEngine을 순서대로 호출한다.
- Zone 설정과 계산 상태는 분리된다. 다음 프레임에 Count가 누적되지 않는다.
- `main.cpp`는 입력 생성, 옵션, 종료 신호, 업데이트·출력·저장 연결을 담당한다.

루프는 `steady_clock`의 고정 주기 스케줄을 사용하며 첫 프레임은 즉시 실행한다.
처리가 주기를 넘으면 밀린 주기를 건너뛴다. 센서 데이터를 과거 시점용으로 재실행하지
않는다. 현재 core 계산은 상태 없는 프레임 계산으로, 물리 solver의 fixed timestep
적분이나 hard realtime 보장은 제공하지 않는다. worker thread는 생성하지 않는다.
루프 대기 중 종료 신호를 최대 20ms 간격으로 확인한다. 추후 입력 읽기는 timeout이
필요하다. 입력 자체가 블로킹되면 현재 프레임이 반환할 때까지 종료가 지연될 수 있다.

## 좌표 변환

`CoordinateMapper`는 하드웨어와 OpenCV에 의존하지 않는다.
기본은 identity이고 `scaledFrame(rawWidth, rawHeight)`는 단순한 크기 변환이다.
실제 광각 카메라의 원근 보정은 별도 calibration으로 얻은 raw→canonical
3×3 homography를 `CoordinateMappingConfig`에 전달해야 한다. 행렬 추정이나
렌즈 왜곡 보정은 아직 구현하지 않았다.

출력은 최근접 정수 canonical 좌표다. 화면 밖 점은 clamp하지 않아 ZoneManager에서
UNASSIGNED로 처리할 수 있다. NaN/Inf, 투영 불가능한 점, 정수 범위를 넘는 점은
`nullopt`다. 기본 Mock 좌표는 이미 canonical이므로 다시 매핑하지 않는다.

## 다음 작업 순서

1. 선택적 OpenCV 색상 마커 검출과 이미지/영상 읽기가 구현됐다.
   사용법은 [CAMERA_MARKERS.md](CAMERA_MARKERS.md)를 참고한다.
   다음은 실제 마커·조명에 맞춘 threshold 설정과 이미 얻은 calibration 행렬 적용 검증이다.
2. RF1 raw ADC protocol, parser, sequence tracker와 host 수신 시각 metadata가 구현됐다.
   [SERIAL_PROTOCOL.md](SERIAL_PROTOCOL.md)에 채널 배치와 검증 규칙을 정리했다.
   Windows COM 및 POSIX 포트에서 부분 수신, timeout, 끊김 처리와 raw 모니터/CSV를 지원한다.
   Uno/MUX sketch는 g이 아닌 9개 raw ADC를 보낸다.
   배선/샘플링/실행은 [FSR_HARDWARE_BRINGUP.md](FSR_HARDWARE_BRINGUP.md)를 참고한다.
3. 하드웨어 입력: Uno 1개와 CD74HC4067 1개에 FSR402 9개(차량당 3개)를 연결한다.
   기준 하중/위치/trial을 붙여 9채널 raw dataset을 수집하는 CLI가 구현됐다.
   [FSR_CALIBRATION.md](FSR_CALIBRATION.md)의 절차로 실측한 뒤 calibration model을 결정한다.
   각 지지점의 보정된 힘을 합산하고 상판 tare를 제거해 차량 하중을 추정한다.
   raw ADC를 단순 평균해 g으로 취급하지 않는다. 보정 곡선·settling 시간은 실측한다.
4. Camera/Serial을 합성하는 InputSource에 취득 시각과 freshness 정책을 추가한다.
   오래된 입력을 정상 실시간 입력처럼 재사용하지 않는다.
5. Guidance 결과의 논리적 LED 출력 모델과 WS2812 전송 모듈을 분리하고,
   기록된 입력 replay로 전체 통합 테스트를 추가한다.

CameraPassengerDetector와 CameraFrameReader는 차량 하중을 생성하지 않는다.
향후 CameraPassengerDetector + SerialVehicleLoadSource를 CompositeInputSource로
합성한다. 실제 Uno/FSR 검증, CompositeInputSource, FSR calibration, LED 하드웨어 제어는 아직 남아 있다.
확정 기준은 차량 3량, 차량당 FSR 3개, 약 100g당 모형 승객 1명/실제 16명,
1kg=정원 100%다. 1kg은 센서 최대값이 아니며 2kg 이상에서도 혼잡도를 clamp하지 않는다.
