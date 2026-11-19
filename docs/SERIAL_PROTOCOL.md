# RailFlow RF1 Serial protocol

이 단계는 raw ADC 전달과 검증을 정의한다. 차량 3량, 차량당 FSR402 3개,
총 9개를 Uno 1개와 CD74HC4067 1개로 순차 읽는 설계다.
Arduino raw acquisition sketch와 Windows/POSIX 시리얼 transport, monitor, CSV 기록기가 구현됐다.
배선과 사용법은 [FSR_HARDWARE_BRINGUP.md](FSR_HARDWARE_BRINGUP.md)를 참고한다.
실제 하드웨어 검증과 calibration은 별도 단계다.

차량 크기와 FSR 일렬 배치는 [FINAL_MODEL_LAYOUT.md](FINAL_MODEL_LAYOUT.md)에 정리되어 있다.

## Wire format

```text
RF1,<sequence>,<deviceMillis>,<adc0>,<adc1>,<adc2>,<adc3>,<adc4>,<adc5>,<adc6>,<adc7>,<adc8>\n
RF1,42,15320,231,228,235,410,398,405,120,118,121\n
```

위 `\n`은 wire에서 실제 LF 한 바이트다. CRLF도 허용한다.
한 줄은 RF1을 포함해 정확히 12개 필드와 11개 comma로 구성된다.

| 필드 | 자료형 / 의미 |
| --- | --- |
| RF1 | ADC 범위와 필드 정의를 식별하는 버전, 대소문자 구분 |
| sequence | uint64, 9채널 scan frame마다 증가하는 순번 |
| deviceMillis | uint32, 해당 scan 취득 완료 시 장치의 ms counter |
| adc0..adc8 | 부호 없는 정수 표기, Uno 기준 0..1023 |

sequence는 0..18446744073709551615, deviceMillis는 0..4294967295다.
숫자는 ASCII 10진 정수로 표기한다. 공백·부호·실수·NaN·Inf·지수·hex 표기는 거부한다.
leading zero는 허용하지만 전체 줄 길이 제한은 유지한다.
최대 내용 길이는 80바이트, LF 포함 81바이트, CRLF 포함 82바이트다.

## 채널 배치

| ADC index / MUX channel | 차량 | 상판 지지점 |
| --- | --- | --- |
| 0 | CAR1 | FSR1 |
| 1 | CAR1 | FSR2 |
| 2 | CAR1 | FSR3 |
| 3 | CAR2 | FSR1 |
| 4 | CAR2 | FSR2 |
| 5 | CAR2 | FSR3 |
| 6 | CAR3 | FSR1 |
| 7 | CAR3 | FSR2 |
| 8 | CAR3 | FSR3 |

다른 ADC 범위나 채널 구조를 도입하면 RF1 정의를 바꾸지 말고 새 protocol version을 사용한다.

## 모듈 경계

```text
Win32 또는 POSIX transport → SerialLineTransport → ReceivedSerialLine
                                                ↓ parseFsrSerialFrame
                                          FsrRawFrame
                                                ↓ host 수신 시각 보존
                                       ReceivedFsrRawFrame
```

`railflow_serial`은 표준 C++만 사용하는 독립 라이브러리다.
core, OpenCV, termios, Arduino 라이브러리에 의존하지 않는다.
`SerialLineTransport::readLine()`은 완성된 줄과 해당 줄을 PC가 수신한
`steady_clock::time_point`를 반환하는 인터페이스다. 완성된 줄이 없으면 nullopt,
I/O 오류는 예외다. Windows의 `railflow_win32_serial`은 Win32 API로 COM 포트를 열고,
macOS/Linux의 `railflow_posix_serial`은 termios/poll을 사용한다.

두 transport 모두 byte chunk를 누적해 LF에서 줄을 확정한다.
partial line은 parser에 넘기지 않으며, 길이 초과 줄은 LF까지 폐기한 뒤 재동기화한다.
기본 timeout은 100 ms(nullopt)이며 disconnect/OS 오류는 예외다.
wire line 기본 한도 512 bytes와 parser의 RF1 내용 한도 80 bytes는 서로 다른 제한이다.
Windows 드라이버가 수신 손상을 보고하면 큐와 미완성 줄을 버리고 다음 LF까지 건너뛴다.
RF1에는 체크섬이 없으므로 유효한 숫자로 바뀐 값은 형식 검사만으로 감지할 수 없다.
연결 해제 오류가 발생하면 포트를 다시 연결하고 CLI를 재실행해 새 sequence 추적을 시작한다.

## Parser validation

`parseFsrSerialFrame(std::string_view)`는 완성된 한 줄의 내용만 parse한다.
`getline` 사용을 위해 terminator가 제거된 내용도 받으며, 실제 LF/CRLF 한 개가
붙어 있어도 받는다. parser가 bare line을 허용한다고 wire의 newline이 선택 사항인 것은 아니다.
parser 자체는 partial byte stream을 조립하지 않는다.

빈 줄, 잘못된 버전, 부족/초과/빈 필드, 잘못된 delimiter, 숫자 범위 초과,
숫자 뒤 garbage, 여러 줄/embedded newline/NUL, 길이 초과를 nullopt로 거부한다.
유효한 모든 필드를 읽기 전에는 부분 frame을 반환하지 않는다.
invalid line은 frame과 sequence tracker를 갱신하지 않고 폐기한다.

## Sequence tracking

`SerialFrameTracker`는 parser와 별개다. 유효한 frame의 sequence만 `observe()`에 전달한다.

| 입력 관계 | 결과 | 기준값 갱신 |
| --- | --- | --- |
| 처음 수신 | First | 갱신 |
| last + 1 | Sequential | 갱신 |
| last와 동일 | Duplicate | 유지 |
| last보다 2 이상 큼 | Skipped, skippedFrames = current - last - 1 | 갱신 |
| last보다 작음 | Backward | 유지 |

예: 10→11 정상, 11→11 중복, 11→14 누락 2개, 14→13 역순.
Duplicate/Backward 뒤에도 마지막 수락 순번을 기준으로 검사한다.
tracker는 분류만 제공한다. 소비 여부와 오류 기록은 호출자가 결정한다.
장치 재시작/재연결 시 호출자가 `reset()`하면 다음 frame은 First다.
uint64 wrap-around도 reset 전에는 Backward로 분류하며 자동 세션 재설정하지 않는다.
차분으로 검사하므로 최댓값 부근에서도 last + 1 정수 overflow가 없다.

## Host timestamp와 다음 단계

`ReceivedFsrRawFrame`은 raw frame과 transport의 `receivedAt`을 함께 보존한다.
장치 deviceMillis와 PC steady_clock은 다른 clock domain이므로 직접 비교하지 않는다.
deviceMillis의 uint32 rollover는 값 그대로 보존하며 sequence 분류에 영향을 주지 않는다.
아직 freshness/timeout 정책을 RailFlowEngine에 적용하지 않는다.

```cpp
if (const auto line = transport.readLine()) {
    if (const auto parsed = parseFsrSerialFrame(line->text)) {
        const ReceivedFsrRawFrame received{*parsed, line->receivedAt};
        const auto sequenceResult = tracker.observe(received.frame.sequence);
        // 다음 단계의 소비자가 상태와 freshness 정책을 적용한다.
    }
}
```

Arduino는 ADC만 보내고 최종 g을 계산하지 않는다. 실제 FSR/상판 조립 후 PC에서
각 센서 calibration, 3개 센서 결합, estimatedWeightG 계산을 구현한다.
지금은 ADC→g 변환, tare, 무게 합산, SerialVehicleLoadSource,
CompositeInputSource, InputFrame 반영을 구현하지 않는다.
기존 Mock 차량 하중, 1kg=100% 기준과 100% 초과 혼잡도 표현은 그대로 유지한다.

## 테스트

SerialFrameParserTests, SerialFrameTrackerTests, SerialTransportTests는
OpenCV OFF/ON 모두 등록된다. transport 테스트는 테스트 전용 replay로
valid/invalid line, host timestamp 보존, invalid line의 tracker 비갱신과
deviceMillis rollover를 검증한다. 실제 serial port나 하드웨어는 열지 않는다.
추가된 PosixSerialTransportTests는 실제 USB 대신 POSIX pseudo-terminal을 열어
framing, timeout, close, disconnect, EINTR, overlong discard/resync를 검증한다.
SerialMonitorTests는 raw CSV와 invalid 처리 및 tracker 상태 출력도 검증한다.
Win32SerialTransportTests는 가상 COM 입력으로 조각 수신, timeout, 손상 복구, 재연결,
Ctrl+C용 종료 요청과 모니터·보정 CSV의 9채널 순서를 검증한다.
