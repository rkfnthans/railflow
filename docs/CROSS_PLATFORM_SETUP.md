# RailFlow Windows·Mac 공통 실행

## railflow 폴더에서 시작

GitHub 저장소를 복제하거나 Download ZIP으로 받아 전체 압축을 푼다.
받은 `railflow` 폴더에서 바로 실행한다. 별도 배포용 폴더로 옮기지 않는다.

| 컴퓨터 | 시작 파일 |
| --- | --- |
| Windows 10/11 64비트 | `START_RAILFLOW.cmd` |
| MacBook / Mac | `START_RAILFLOW.command` |

시작 파일을 더블클릭하면 공통 메뉴가 열린다. 실행기는 운영체제와 CPU를 확인하고 Windows용 `.exe`
또는 Mac용 Universal 실행 파일을 선택한다. Mac 실행 파일은 Apple Silicon과 Intel 코드를 포함하며
운영체제가 현재 CPU에 맞는 코드를 로드한다.

```text
railflow/
  START_RAILFLOW.cmd       Windows 시작
  START_RAILFLOW.command   Mac 시작
  TEST_SYSTEM.cmd         Windows 검사
  TEST_SYSTEM.command     Mac 검사
  bin/
    windows-x64/          Windows 실행 파일 5개
    macos/                Apple Silicon·Intel Universal 실행 파일 5개
    manifest.json         실행 파일 검증용 SHA256
  scripts/
  src/  include/  tests/  docs/  firmware/
```

소스와 두 운영체제용 `bin` 파일을 같은 저장소에 올린다. `build`와 새 실행 로그는 `.gitignore`로
제외한다. GitHub에서 폴더 전체를 받으면 준비된 실행 파일을 사용하므로 개발 도구와 OpenCV의
별도 설치 없이 시작할 수 있다.

메뉴에서 Mock 데모, 차량 배치, 카메라 미리보기, 카메라 검색, 표시 테스트,
raw ADC 모니터와 보정 기록기를 실행한다. 차량 규격은 1량 30×15×약15cm, 총 3량·FSR 9개다.
카메라에서는 현재 연결된 번호를 선택한다. 확인된 사용자 Windows 설정은 카메라 1번 / DirectShow다.
Mac에서는 자동 backend로 AVFoundation을 사용한다. 시리얼 포트는 Windows의 `COM10` 형식,
Mac의 `/dev/cu.usbserial-…` 형식으로 입력하며, 해당 운영체제의 통신 구현을 사용한다.

카메라 창은 Q·Esc·창 닫기로 종료한다. 터미널에서는 Ctrl+C로 종료할 수 있다.
기준추의 실측 질량과 차량·위치·trial을 입력하면 보정용 raw ADC를 기록한다.
Mock 결과 CSV는 `data/logs`, 보정 데이터는 `data/calibration`, 실행 로그는 `logs`에 저장한다.

## 공통 검사

Windows에서는 `TEST_SYSTEM.cmd`, Mac에서는 `TEST_SYSTEM.command`를 실행한다.
장비 없이 차량 배치, Mock 실행, CSV, 도구 도움말과 카메라 테스트 화면 생성 경로를 검사한다.
정상 결과는 `Checks=7 Failures=0`이다. 실제 USB 카메라 화면은 메뉴 3번에서 별도로 확인한다.

Windows 검사 결과는 `windows-check-*`, Mac 검사 결과는 `system-checks/macos-*`에 저장된다.
문제가 생기면 해당 검사 폴더나 `logs`의 실행 로그를 전달한다.

## Mac 실행 권한과 카메라 권한

Mac 패키지는 최소 지원 버전 macOS 11.0으로 빌드한 Universal 프로그램이다.
현재 Mac에서 Apple Silicon 실행과 Rosetta의 Intel 실행을 검증했다. 다른 macOS 버전과 실물 Intel Mac의
카메라 장치 확인은 각 컴퓨터에서 수행한다.

인터넷에서 받은 패키지에 macOS 확인 창이 표시되면 Finder에서 시작 파일을 우클릭해 **열기**를 선택한다.
카메라 사용 요청이 뜨면 허용한다. macOS의 개인정보 보호 및 보안 설정에서 터미널의 카메라 접근을 확인할 수 있다.
압축 해제 도구가 실행 권한을 보존하지 않은 경우 터미널에서 다음 명령으로 복구한다.

```sh
chmod +x START_RAILFLOW.command bin/macos/*
./START_RAILFLOW.command
```

## 소스 수정 후 빌드

저장소와 소스 ZIP 모두 두 운영체제용 `bin`과 같은 시작 파일이 포함된다.
준비된 실행 파일이 없으면 현재 컴퓨터용 CMake 빌드를 수행한다.
빌드 도구는 Windows에서 Visual Studio C++ Build Tools와 CMake, Mac에서 Xcode Command Line Tools와 CMake를 사용한다.
카메라 프로그램의 소스 빌드에는 C++용 OpenCV 4의 core, imgproc, imgcodecs, videoio, highgui 모듈이 필요하다.

운영체제별 빌드 디렉터리는 `build/native` 아래에 생성한다. `OpenCV_DIR` 설정은 각 컴퓨터에 설치한
OpenCVConfig.cmake 위치를 사용한다. 실행 패키지에는 OpenCV를 정적으로 링크한 프로그램이 포함된다.
준비된 실행 패키지로 카메라를 볼 때는 OpenCV를 별도로 설치하지 않는다.
소스를 수정한 뒤에는 각 컴퓨터에서 CMake로 다시 빌드하고 해당 빌드 폴더의 실행 파일을 사용한다.
`bin`의 프로그램은 미리 빌드한 버전이며, 저장소용 실행 파일을 갱신하는 절차는 아래와 같다.

명령으로 차량 배치를 바로 확인할 수도 있다.

```powershell
.\START_RAILFLOW.cmd --layout
```

```sh
./START_RAILFLOW.command --layout
```

기존 자세한 설정은 `docs/WINDOWS_SETUP.md`와 `docs/USB_CAMERA_WINDOWS.md`에 있다.
이번 패키지의 빌드·검사 결과와 검증 범위는 `docs/CROSS_PLATFORM_VERIFICATION.md`에 기록했다.

## 패키지 갱신

`scripts/package_release.py`에 최신 Windows x64, Mac arm64, Mac x86_64 빌드 디렉터리를 전달하면
두 Mac 실행 파일을 Universal로 합치고 코드 서명을 다시 적용한다. 먼저 저장소의 `bin/windows-x64`와
`bin/macos`, `bin/manifest.json`을 갱신하고, 같은 파일로 공통 실행 패키지와 지정된 r3 ZIP을 만든다.
소스 ZIP에도 `bin`을 포함한다. 교체 전 ZIP은 `build/exports/backups`에 보관한다.

```sh
python3 scripts/package_release.py \
  --windows-bin build/camera-windows-x64 \
  --macos-arm64-bin build/cross-macos-arm64 \
  --macos-intel-bin build/cross-macos-intel \
  --output-dir build/exports
```
