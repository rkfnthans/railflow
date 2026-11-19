# FSR calibration dataset acquisition

이번 단계는 **기준추 label과 9채널 raw ADC를 함께 기록**한다. ADC → gram 모델,
regression, polynomial fitting, lookup table, tare algorithm, 승객 수 추론을 구현하지 않는다.
SerialVehicleLoadSource / CompositeInputSource / freshness / LED 연결도 이번 범위가 아니다.
기존 `railflow`, `railflow_serial_monitor`, firmware와 raw CSV schema는 변경하지 않는다.

FSR 3개는 30cm 길이 방향으로 일렬 배치하며, 정확한 센서 간격은 아직 미확정이다.
실제 배치와 조립 형태에 따라 보정 값이 달라질 수 있다.
[FINAL_MODEL_LAYOUT.md](FINAL_MODEL_LAYOUT.md)를 참조한다.

## Build / CLI

`railflow_calibration`은 표준 C++20 + 기존 `railflow_serial`만 사용하는 별도 라이브러리다.
CLI는 Windows의 COM 포트와 macOS/Linux의 POSIX 시리얼 포트를 지원한다.
OpenCV 없이 빌드할 수 있다. [Windows 실행 안내](WINDOWS_SETUP.md)

```sh
cmake -S . -B build -DRAILFLOW_ENABLE_OPENCV=OFF -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure

./build/railflow_calibration_record /dev/cu.usbmodemXXXX \
  --vehicle CAR1 --weight-g 500 --position CENTER --trial 1 \
  --duration-sec 5 --settle-ms 1000 \
  --output data/calibration/car1.csv \
  --session-id 2026-10-08-car1 --note 'assembled platform, first run'
```

필수: device, `--vehicle`, `--weight-g`, `--position`, `--trial`, `--output`.
모든 metadata/CLI 옵션은 Serial open과 파일 생성 전에 검증한다.
Arduino Serial Monitor 등 다른 포트 사용 프로그램은 닫는다.
출력 상위 디렉터리는 자동 생성하므로 build output과 섞지 않고 `data/calibration/`에 보관할 수 있다.

| 옵션 | 정책 |
| --- | --- |
| `--vehicle` | 정확히 CAR1 / CAR2 / CAR3, 대문자만 허용 |
| `--weight-g` | 유한한 0 이상 실수, 기준추의 실제 질량 label. ADC로 계산하지 않음. 1kg/2kg에서 clamp하지 않음 |
| `--position` | 정확히 CENTER / LEFT / RIGHT / FRONT / REAR, 대문자만 허용 |
| `--trial` | 1..4294967295 정수. 동일 조건의 반복 번호, frame 번호가 아님 |
| `--duration-sec` | 기본 5, 1..3600 정수 초. 안정화가 끝난 뒤의 측정 시간 |
| `--settle-ms` | 기본 1000, 0..60000 정수 ms |
| `--samples` | 선택, 양의 정수. parser-valid 저장 행 수 상한 |
| `--baud` | 기본 115200. 기존 transport의 9600/19200/38400/57600/115200 지원 |
| `--session-id`, `--note` | 선택, 기본 빈 문자열. 각 최대 1024 bytes, 제어문자/CR/LF/NUL 금지. UTF-8·쉼표·따옴표 허용 |

중복/알 수 없는 옵션, 누락된 값, 잘못된 enum/숫자는 오류로 종료한다.
`--samples`와 duration은 함께 사용할 수 있다. **먼저 충족한 조건에서 종료**하며,
samples만 지정해도 기본 5초 duration이 유지된다. 입력이 끊기거나 무효 줄만 와도 무한 대기하지 않는다.
예를 들어 약 10Hz에서 50개 확보를 목표로 하되 최대 10초만 대기하려면:

```sh
./build/railflow_calibration_record /dev/cu.usbmodemXXXX \
  --vehicle CAR1 --weight-g 500 --position CENTER --trial 2 \
  --settle-ms 1000 --duration-sec 10 --samples 50 \
  --session-id 2026-10-08-car1 --output data/calibration/car1.csv
```

## Acquisition / settling / sequence 정책

1. 포트와 CSV를 연 뒤 capture 시작 시각을 host `steady_clock`으로 잡는다.
2. 안정화 구간에도 계속 Serial을 읽고 parser/tracker를 동작시켜 backlog를 줄인다.
3. 저장 구간은 `[시작 + settle, 시작 + settle + duration)`이다.
4. 각 줄의 **host receive timestamp**로 구간을 판단한다. 안정화 때 받은 줄이 늦게 dequeue되어도 저장하지 않는다.
5. 구간 내 parser-invalid 줄은 CSV/ADC 통계/sequence 기준값을 갱신하지 않고 invalid count만 증가시킨다.
6. 유효 줄은 기존 tracker 상태와 함께 저장한다. Duplicate/Backward도 관측 증거로 보존하고 별도 계수한다.

안정화 구간의 유효 sequence는 tracker 기준값에 반영한다. 따라서 첫 저장 행도 Sequential/Skipped 등이
될 수 있다. 자동 reset이나 freshness rejection은 추가하지 않는다. 새 CLI 실행마다 tracker는 새로 시작한다.
summary의 skipped count는 저장된 행의 `skipped_frames` 합이다. 첫 sequence 이전 누락은 알 수 없다.
Duplicate/Backward가 있으면 raw 수집 상태를 점검하고 향후 분석 시 사용 정책을 결정한다.
안정화 줄 수는 별도로 출력하며 invalid count와 ADC 통계는 **저장 구간만** 집계한다.
transport가 폐기한 overlong 줄 수는 안정화 포함 전체 실행 기준으로 별도 출력한다.

read timeout은 기본 100ms다. duration/Ctrl+C 종료 확인이 최장 약 한 read timeout만큼 늦어질 수 있지만,
측정 종료 이후 timestamp의 행은 저장하지 않는다. Windows에서는 Ctrl+C, macOS/Linux에서는
Ctrl+C 또는 SIGTERM으로 정상 종료 코드 0과 summary를 제공한다. 파일과 포트는 RAII로 정리한다.
OS 오류·disconnect·CSV 오류는 종료 코드 1이다.
정상 종료했어도 0행이면 경고와 n/a 통계를 출력한다. 빈 dataset을 성공적인 센서 실험으로 해석하지 않는다.

Uno는 USB 포트를 열 때 재시작될 수 있다. settle은 첫 RF1이 아니라 capture 시작 기준이다.
실제 보드 기동 시간과 하중 안정화 특성을 보고 `--settle-ms`를 조정한다.
기본 1초는 센서 안정성 보증이 아니다. 약 10Hz/5초에서 대략 50행이지만 실제 수는 반드시 summary로 확인한다.

## 고정 CSV schema

```text
host_time_steady_ns,sequence,deviceMillis,vehicle_id,known_weight_g,load_position,trial,adc0,adc1,adc2,adc3,adc4,adc5,adc6,adc7,adc8,tracker_status,skipped_frames,session_id,note
```

- `known_weight_g`는 사람이 올려놓은 기준추 질량이다. predicted weight 열은 없다.
- CAR1 실험도 adc0..adc8 **전부** 저장한다. cross-talk/전원/다른 차량 영향 검수에 활용한다.
- 기본 관심 채널은 CAR1=adc0..2, CAR2=adc3..5, CAR3=adc6..8이다.
- host timestamp는 transport의 LF 수신 시각을 그대로 보존한 steady nanoseconds다.
  UTC가 아니고 deviceMillis와 다른 clock domain이다. 한 OS read로 받은 여러 줄의 시각은 같을 수 있다.
- session/note는 항상 따옴표로 감싸고 내부 `"`는 `""`로 저장한다. 숫자는 locale에 상관없이 소수점 `.`을 사용한다.
- 새 파일/빈 파일은 header 한 번, 동일 schema 기존 파일은 append한다. raw CSV 등 호환되지 않는 header나
  LF 없이 끊긴 마지막 행은 거부하고 기존 내용을 덮어쓰지 않는다. 매 행 flush하며 쓰기 오류를 보고한다.
- 같은 CSV에 여러 프로세스가 동시에 쓰지 않는다. 세션 간 clock 절대값을 비교하지 말고 session_id를 사용한다.

## Summary

차량·known weight·위치·trial·session, 저장 valid 수, 구간 내 invalid 수, skipped 수,
Duplicate/Backward 수, 안정화 폐기 줄 수, 중단 여부, 파일 경로를 출력한다.
선택 차량의 3채널에 대해 raw mean/min/max와 **population standard deviation**을 출력한다.
통계에는 저장된 모든 parser-valid 행(표시된 Duplicate/Backward 포함)이 반영된다.
0행이면 숫자를 만들어내지 않고 n/a를 출력한다. 이 통계는 raw 품질 검수용이며 무게 추정이 아니다.

## 실제 센서 도착 후 실험 절차

배선/전원/MUX/전송 확인은 [FSR_HARDWARE_BRINGUP.md](FSR_HARDWARE_BRINGUP.md)를 따른다.
각 센서 divider가 독립이고 채널 순서가 맞는지 먼저 점검한다.

1. 차량/상판/FSR 지지 구조를 최종 상태로 조립한다. 회로 저항값, 전원, 센서 배치, firmware 버전과
   settling/discard/sample 평균 설정을 별도 실험 기록에 남긴다. 수집 도중 이 조건을 바꾸지 않는다.
2. **0g는 상판을 제거한 상태가 아니다.** 최종 차량 구조를 모두 조립한 상태에서
   **추가 승객 하중 0g**인 baseline이다. 향후 tare는 이 baseline을 기준으로 결정하며 지금은 차감하지 않는다.
3. 각 CAR별로 아래 알려진 추가 하중을 준비한다:

   `0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1200, 1500, 2000 g`

4. 각 하중마다 CENTER / LEFT / RIGHT / FRONT / REAR에서 측정한다.
   각 조건은 가능하면 2~3 trial 반복한다. 예: CAR1 / 500g / CENTER / trial1, trial2,
   다음 CAR1 / 500g / LEFT / trial1, trial2 등. 0g 위치 label은 실험 순서 분류용으로 기록할 수 있다.
5. 기준추와 위치를 실제로 바꾼 뒤 해당 label로 새 CLI를 실행한다. session_id로 실험 묶음을 식별하고,
   note에 기구/환경/하중 올림·내림 순서 등 관찰을 남긴다. 프로그램은 실제 기준추 배치를 확인할 수 없다.
6. 매 trial의 행 수, invalid/skipped/duplicate/backward, ADC min/max/흔들림/포화 여부를 검수한다.
   1000g은 정원 기준이지 센서 최대값이 아니다. 1200/1500/2000g도 실제 기구 안전 범위 내에서 측정한다.
7. 동일 조건과 하중을 올리거나 내리는 조건에서 반복 데이터가 달라질 수 있음을 기록한다.
   개별 FSR response 차이, 위치별 load sharing, creep/hysteresis 가능성을 확인한다.

**개별 FSR ADC는 weight가 아니며 `adc0 + adc1 + adc2`를 gram으로 취급하지 않는다.**
최종 calibration model은 이렇게 확보한 실측 dataset을 보고 이후 단계에서 결정한다.

## 자동 검증과 미검증 범위

CalibrationTests는 fake transport/clock으로 경계 시각, duration/sample 종료, metadata validation,
9채널/escaping/schema/append, raw 통계와 중단 처리를 검사한다.
UNIX의 CalibrationCliTests는 실제 CLI를 PTY로 실행하고 정상/invalid 입력, 실제 settle/duration,
SIGINT 중단 및 파일 재사용, 실행 전 validation을 검사한다. Arduino는 필요 없다.
실제 센서 안정화·creep/hysteresis·기구 하중 분배·전원 영향과 calibration 정확도는 이 테스트로 검증하지 않는다.
