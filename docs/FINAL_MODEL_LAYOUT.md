# RailFlow 캡스톤 최종 모형 사양 (2026-11-12)

## 사용자 확정 사양

| 항목 | 내용 |
| --- | --- |
| 편성 | 3량 (CAR1~CAR3) |
| 한 차량 길이 | 25 cm |
| 한 차량 폭 | 15 cm |
| 한 차량 높이 | 약 15 cm (제작 목표) |
| 차체 길이 합계 | 75 cm (연결기·차량 간 틈 제외) |
| FSR 배치 | 차량마다 **길이 방향(25 cm 축)으로 일렬 3개** |
| FSR 총수 | 3량 × 3개 = **9개** |
| FSR 간격 | **미확정**. 임의의 고정 간격/좌표를 사용하지 않음 |

### 상면 배치 (개념도, 축척 아님)

```text
           <------------- 차량 길이 25 cm ------------->
           +-------------------------------------------+
CAR1       | FSR1(C0)    FSR2(C1)    FSR3(C2)           | 폭 15 cm
           +-------------------------------------------+
           +-------------------------------------------+
CAR2       | FSR1(C3)    FSR2(C4)    FSR3(C5)           | 폭 15 cm
           +-------------------------------------------+
           +-------------------------------------------+
CAR3       | FSR1(C6)    FSR2(C7)    FSR3(C8)           | 폭 15 cm
           +-------------------------------------------+
```

각 열의 그림상 간격은 설명용이다. 실제 장착 좌표와 FSR 지지 하중 전달 기구는
실물 설계 시 확정한다. 본 문서의 높이 15cm는 정확한 제조 공차를 뜻하지 않는다.

## 코드/배선 채널 계약

`TrainLayout.hpp`는 PC 코드에서 편성 수, 차량 크기 및 채널 순서를 관리한다.
인덱스는 0부터 시작한다. `channelIndex(car, sensor) = car * 3 + sensor`.

| 차량 | 센서(앞에서 뒤로 또는 한쪽 끝에서부터 순서대로 고정) | MUX | RF1 ADC |
| --- | --- | --- | --- |
| CAR1 | FSR1, FSR2, FSR3 | C0, C1, C2 | adc0, adc1, adc2 |
| CAR2 | FSR1, FSR2, FSR3 | C3, C4, C5 | adc3, adc4, adc5 |
| CAR3 | FSR1, FSR2, FSR3 | C6, C7, C8 | adc6, adc7, adc8 |

물리적인 FSR1 방향을 차량마다 일치시키고, 배선 라벨과 실제 압력 테스트로 C0..C8
순서를 검증해야 한다. Arduino sketch에는 동일 계약을 독립된 상수(9)로 유지한다.
Uno + CD74HC4067 1개와 FSR 9개로 구성되어 있으며, 9개 독립 전압분배회로가 필요하다.

## 실행 방법

```sh
cmake -S . -B build -DRAILFLOW_ENABLE_OPENCV=OFF -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/railflow --layout
./build/railflow --demo --frames 3 --period-ms 100 --no-log
# 장비를 연결했을 때, 포트 이름은 예시
./build/railflow_serial_monitor /dev/ttyACM0 --baud 115200
```

`--layout`은 센서/차량 사양 및 채널 대응을 출력하고 종료한다.
`--demo`는 **가상의 차량 하중과 승객 위치**를 사용하므로 9개의 실제 FSR값을
반영한 결과가 아니다. `railflow_serial_monitor`는 **raw ADC**를 차량별로 표시한다.

## 현재 구현과 남은 범위

- 구현: Arduino 9채널 스캔, RF1 시리얼, 채널 분류, raw 모니터/CSV, 보정 실험 데이터 수집, 기존 mock 기반 혼잡도 및 추천 계산.
- 미구현: 실물 장착 상태의 센서별 보정 곡선/상판 tare/차량별 하중 합산,
  실제 RF1 데이터를 차량 하중(g)으로 환산해 `InputFrame`에 넣는 연동,
  Camera/FSR 실시간 데이터 합성 및 LED 실제 제어.
- 이유: FSR ADC값과 하중(g)은 동등하지 않으며 보정 없이 `ADC=그램`으로
  대체하면 혼잡도와 추천 위치가 허위 수치가 된다.

기준 하중과 위치별 측정은 [FSR_CALIBRATION.md](FSR_CALIBRATION.md)를 참고한다.
