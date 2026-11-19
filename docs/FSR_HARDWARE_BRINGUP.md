# FSR raw acquisition bring-up

이번 단계의 범위는 `9 FSR → Uno/MUX → RF1 → POSIX transport → parser/tracker → monitor/CSV`이다.
**ADC는 무게가 아니다.** Calibration, tare, force equation, 차량 중량 fusion,
freshness rejection, CompositeInputSource, Guidance/LED 연결은 아직 구현하지 않는다.
기존 `railflow`, `MockInput`, `InputFrame`, core 계산과 OpenCV optional 구조는 그대로 유지한다.

차량별 FSR은 30cm 길이 방향에 3개씩 일렬 배치한다. 센서 간격은 아직 미확정이다.
모형 치수·센서 순서의 기준은 [FINAL_MODEL_LAYOUT.md](FINAL_MODEL_LAYOUT.md)다.

## Wiring

전원을 끈 상태에서 결선한다. 모듈의 실제 핀 라벨과 전원/신호 허용 범위를 확인한다.
CD74HC4067 보드의 SIG/COM 표기는 보드에 따라 다를 수 있다.

| CD74HC4067 신호 | Arduino Uno |
| --- | --- |
| S0 (선택 비트 0, LSB) | D2 |
| S1 | D3 |
| S2 | D4 |
| S3 (선택 비트 3, MSB) | D5 |
| SIG / COM | A0 |
| EN / E (active LOW) | GND: 항상 enable |
| VCC | 5V (사용 모듈 사양 확인) |
| GND | GND (전체 회로 공통) |

CD74HC4067은 E가 HIGH이면 모든 채널이 꺼지고, LOW일 때 S0..S3로 채널을 선택한다.
핀의 의미와 선택 진리표는 [TI CD74HC4067 datasheet](https://www.ti.com/lit/ds/symlink/cd74hc4067.pdf)를 따른다.
Uno 보드 정보는 [Arduino Uno Rev3 공식 문서](https://docs.arduino.cc/hardware/uno-rev3/)를 참고한다.

| 차량 | MUX 채널 | RF1 필드 |
| --- | --- | --- |
| CAR1 | C0, C1, C2 | adc0, adc1, adc2 |
| CAR2 | C3, C4, C5 | adc3, adc4, adc5 |
| CAR3 | C6, C7, C8 | adc6, adc7, adc8 |

**센서별 독립 voltage divider 9개**가 필요하다. 예를 들어 각 채널마다
`5V → FSR → 측정 노드 → 고정 저항 → GND`로 구성하고, 측정 노드를 해당 MUX Cn에 연결한다.
저항값은 이 단계에서 확정하거나 힘 계산에 사용하지 않는다. 실제 회로/ADC 포화/settling을
측정해서 선정한다. 모든 측정 노드는 Uno와 MUX의 허용 전압 범위를 지켜야 한다.
C9..C15는 사용하지 않고 읽지도 않는다. 센서 연결 대신 9개 ADC를 단순 병렬 연결하지 않는다.

## Firmware / acquisition contract

Sketch: `firmware/railflow_fsr_uno/railflow_fsr_uno.ino`.
Arduino IDE에서 Arduino Uno 보드와 USB 포트를 선택하여 업로드한다.
Arduino CLI가 설치된 경우:

```sh
arduino-cli compile --fqbn arduino:avr:uno firmware/railflow_fsr_uno
arduino-cli upload --fqbn arduino:avr:uno -p /dev/cu.usbmodemXXXX firmware/railflow_fsr_uno
```

`AcquisitionConfig`의 기본값:

- baud 115200, 8N1, flow control 없음
- frame period 100 ms (약 10 Hz), `millis()` unsigned subtraction scheduling
- C0..C8 순차 측정, 채널 선택 후 settling 100 µs
- 채널당 첫 `analogRead(A0)` 1회 폐기, 다음 4회 평균
- 평균은 `(sum + samples / 2) / samples`, 정수 반올림(half up), 0..1023 전송
- ADC 기준 `analogReference(DEFAULT)`; 전압/힘/gram 변환 없음
- 9채널 수집이 끝난 후 `deviceMillis = millis()` 기록
- 부팅마다 sequence 0부터 frame마다 증가. uint64를 직접 십진수 출력하여 Uno의 32-bit Print 제한 회피
- 지연된 주기는 몰아서 전송하지 않는다. `delay(100)` 또는 실시간 catch-up loop 없음

Settling/discard/평균/기준전압/저항/센서 배치가 바뀌면 calibration 데이터의 acquisition 조건도
달라진다. 데이터 수집 시 firmware 버전과 이 설정을 별도 실험 기록에 남긴다.
100 µs는 bring-up 초기값이지 실제 회로 settling 보증이 아니다.

RF1은 기존 host parser와 동일한 12개 필드이며 LF로 끝난다. 진단 문구를 이 스트림에 섞지 않는다.

```text
RF1,<sequence>,<deviceMillis>,<adc0>,<adc1>,<adc2>,<adc3>,<adc4>,<adc5>,<adc6>,<adc7>,<adc8>\n
RF1,42,15320,231,228,235,410,398,405,120,118,121
```

sequence는 uint64, deviceMillis는 uint32이며 장시간 실행 시 millis wrap은 허용된다.
Tracker는 millis가 아니라 sequence를 관찰한다. Uno 재부팅 후 sequence가 뒤로 가면 Backward로
기록할 수 있으며, 이번 단계에서는 자동 reset/freshness 정책을 추가하지 않는다.
새 모니터 실행은 새 tracker로 시작한다.

## Host build and monitor

외부 serial library는 없다. macOS/Linux의 `UNIX` CMake 분기에서만 POSIX transport와
`railflow_serial_monitor`를 빌드하며, OpenCV가 없어도 동작한다.

```sh
cmake -S . -B build -DRAILFLOW_ENABLE_OPENCV=OFF -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/railflow_serial_monitor /dev/cu.usbmodemXXXX
./build/railflow_serial_monitor /dev/cu.usbmodemXXXX --baud 115200 --record data/logs/fsr_raw.csv
```

Linux에서는 예를 들어 `/dev/ttyACM0`을 사용하며 장치 접근 권한이 필요하다.
Arduino IDE Serial Monitor 등 다른 포트 사용 프로그램은 닫는다. 실제 포트 이름은 환경마다 다르다.

```text
seq=42 ms=15320
CAR1 raw: 231 228 235
CAR2 raw: 410 398 405
CAR3 raw: 120 118 121
status=First skipped=0
```

상태는 First / Sequential / Duplicate / Skipped / Backward이다.
잘못된 RF1 줄은 invalid count를 증가시키고 계속 읽는다. Duplicate/Backward도 raw 관측값으로
출력/기록한다. Skipped에는 빠진 sequence 개수를 기록한다. 종료 시 valid/invalid/overlong 개수를 출력한다.
연결 해제나 OS I/O 오류는 메시지와 종료 코드 1, 정상 Ctrl+C는 종료 코드 0이다.
재연결은 이번 범위에 없으므로 포트를 확인한 뒤 다시 실행한다.

선택 옵션: `--timeout-ms 100` (1..60000 ms), `--frames N` (정상 parse된 frame N개 후 종료,
0은 무제한). Ctrl+C 응답 대기 상한은 read timeout이므로 대화형 사용에는 기본 100 ms를 권장한다.

Transport는 raw 8N1 nonblocking fd + `poll` deadline을 사용한다. `readLine()` 기본 timeout은
100 ms이고, overload로 0..60000 ms를 지정할 수 있다. 정상 timeout은 `nullopt`, close된 fd/OS 오류/
disconnect는 예외다. EINTR는 남은 deadline으로 재시도한다. 부분 줄은 다음 호출까지 유지하고,
LF 기준 완성된 줄을 하나씩 반환하며 CRLF의 마지막 CR은 제거한다.
기본 wire content 한도는 512 bytes(선택 CR 포함, LF 제외), 초과 시 그 줄 전체를 다음 LF까지
폐기한 뒤 재동기화한다. `discardedOverlongLines()`는 invalid parser count와 별도 계수다.
완성 줄 큐는 한 번의 최대 512-byte read에서만 생성되며 큐를 비운 후에만 새 read를 수행한다.
fd와 원래 termios는 close/destructor에서 정리한다. 인스턴스의 동시 read/close는 지원하지 않는다.

## Raw CSV

```text
host_time_steady_ns,sequence,deviceMillis,adc0,adc1,adc2,adc3,adc4,adc5,adc6,adc7,adc8,tracker_status,skipped_frames
```

`host_time_steady_ns`는 LF가 들어 있는 OS read가 완료된 host `steady_clock` 시각의
나노초 값이다. UTC/벽시계가 아니고 deviceMillis와 다른 clock domain이며, 다른 호스트/세션의
절대시각으로 사용하지 않는다. 한 read에 여러 줄이 완성되면 같은 timestamp일 수 있다.
CSV는 새 파일/빈 파일에 header 한 번, 같은 header인 기존 파일에는 append한다.
호환되지 않는 header나 LF 없이 끊긴 마지막 행이 있는 파일은 덮어쓰지 않고 오류로 중단한다.
상위 디렉터리는 미리 생성해야 한다. 매 행 flush하고 쓰기 오류를 보고한다.
weight_g 열이나 ADC 합계→gram 계산은 없다. 기존 `railflow_log.csv`와 별도 파일을 사용한다.

## 다음 단계 calibration 데이터 수집 원칙

Label이 포함된 dataset 수집 도구와 실험 절차는 [FSR_CALIBRATION.md](FSR_CALIBRATION.md)를 참고한다.
기존 raw monitor CSV와 별도 schema로 저장하며 아직 ADC→gram 모델은 없다.

**FSR raw ADC 3개를 단순히 더해서 gram으로 간주하지 않는다.** 실제 조립 후 CAR별로 알려진 하중
0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1200, 1500, 2000 g을 사용하고,
각 동일 하중을 중앙/왼쪽/오른쪽/앞/뒤에 배치해 3개 FSR load distribution을 기록한다.
반복 측정과 안정화 시간, 실험별 실제 하중/위치/차량/회로/firmware 버전을 별도 라벨로 남긴다.
이후 PC에서 이 데이터로 calibration model을 결정한다. 1 kg = 정원 100%는 향후 혼잡도 기준이지
센서 최대값이 아니다. ADC 포화와 기구 허용 하중은 실제 하드웨어에서 확인한다.

## 검증 범위

Host CTest는 pseudo-terminal로 LF/CRLF, split/multiple lines, timeout, close, disconnect,
EINTR, overlong discard/resync, parser 위임을 검증한다. Replay 테스트는 5가지 tracker 상태,
CAR 채널 순서, invalid 처리, CSV append/header/schema/timestamp를 검증한다.
Arduino sketch는 host CTest에서 빌드하지 않는다.

실제 Uno 업로드/USB reset, 보드별 EN/전원 배선, 9채널 독립성, 실제 ADC 범위·노이즈·포화,
settling의 충분성, 약 10 Hz 실측, 장시간 USB 안정성은 실제 장비로 별도 확인해야 한다.
