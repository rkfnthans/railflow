# 공통 패키지 검증 기록 — 2026-11-19

## 빌드 환경과 결과

검사 호스트는 macOS의 Apple Silicon Mac이다. Windows용 프로그램은 LLVM-MinGW로
교차 컴파일하고 Wine에서 실행했다. Visual Studio/MSVC와 실제 Windows PC에서 새 시작 메뉴를
실행한 결과는 이번 검사에 포함하지 않는다. Intel Mac용은 x86_64로 빌드하고 Rosetta에서 실행했다.

| 구성 | 자동 테스트 |
| --- | --- |
| Mac arm64 / OpenCV ON / 최소 OS 11.0 | 29/29 통과 |
| Mac x86_64 / OpenCV ON / 최소 OS 11.0 / Rosetta | 29/29 통과 |
| Windows x64 / OpenCV ON / Wine | 27/27 통과 |

macOS 11.0은 실행 파일에 기록한 배포 대상이다. 실제 구형 macOS와 실물 Intel Mac의 카메라 장치는
검사하지 않았다. Mac Universal 실행 파일은 arm64·x86_64 두 아키텍처를 포함하며 ad-hoc 서명했다.
Apple 개발자 인증서와 공증은 적용하지 않았다. OpenCV는 4.12.0을 정적으로 링크했다.

## 실행한 빌드와 자동 테스트

아래 `cmake`와 `ctest`는 작업 폴더의 CMake 도구를 사용했다. Windows 빌드 디렉터리에는
기존 LLVM-MinGW 도구 모음과 Wine CMake 에뮬레이터 설정을 사용했다.

```sh
cmake -S . -B build/cross-macos-arm64 \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DCMAKE_BUILD_TYPE=Release -DRAILFLOW_ENABLE_OPENCV=ON \
  -DRAILFLOW_ENABLE_CAMERA_PREVIEW=ON -DBUILD_TESTING=ON \
  -DOpenCV_DIR="$PWD/build/camera-tools/opencv-macos-arm64-install/lib/cmake/opencv4"
cmake --build build/cross-macos-arm64 --parallel 8
ctest --test-dir build/cross-macos-arm64 --output-on-failure

cmake -S . -B build/cross-macos-intel \
  -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DCMAKE_BUILD_TYPE=Release -DRAILFLOW_ENABLE_OPENCV=ON \
  -DRAILFLOW_ENABLE_CAMERA_PREVIEW=ON -DBUILD_TESTING=ON \
  -DOpenCV_DIR="$PWD/build/camera-tools/opencv-macos-intel-install/lib/cmake/opencv4"
cmake --build build/cross-macos-intel --parallel 8
ctest --test-dir build/cross-macos-intel --output-on-failure

cmake --build build/camera-windows-x64 --parallel 8
ctest --test-dir build/camera-windows-x64 --output-on-failure
```

## 배포 ZIP 검사

압축을 한글과 공백이 있는 새 경로에 풀고 파일 해시를 모두 확인했다.

- Mac: `START_RAILFLOW.command --layout`, `TEST_SYSTEM.command` 성공. 검사 7/7 통과.
- Windows/Wine: `START_RAILFLOW.cmd --layout`, `--demo --frames 3 --period-ms 100 --no-log`,
  `TEST_SYSTEM.cmd` 성공. 검사 7/7 통과.
- 두 시스템 모두 30×15cm 차량 3량, FSR 9개, 총 길이 90cm 출력과 Mock CSV 3행 및 P3 추천 확인.
- Mac Universal의 Intel 코드로 layout과 카메라 테스트 프레임 수신 성공.
- Mac 실제 창에서 테스트 패턴 30프레임 표시 후 정상 종료 확인.
- 소스 ZIP을 별도 한글·공백 경로에 풀고 `START_RAILFLOW.command --layout` 실행.
  OpenCV 없이 현재 Mac용 CMake 빌드 및 실행 성공.
- 이전 r3 ZIP은 교체 전에 `build/exports/backups`에 보관.

## GitHub 저장소에서 직접 실행

후속 변경으로 같은 실행 파일을 저장소의 `bin/windows-x64`와 `bin/macos`에 넣었다.
별도 배포 폴더 없이 `railflow` 폴더 자체에서 시작한다. `bin/manifest.json`의 SHA256으로
검증한 실행 파일과 같은 파일임을 확인했다.

- 저장소 루트의 `START_RAILFLOW.command --layout`, `TEST_SYSTEM.command`: 종료 코드 0, 검사 7/7 통과.
- 같은 루트에서 Wine으로 `START_RAILFLOW.cmd --layout`, `TEST_SYSTEM.cmd`: 종료 코드 0, 검사 7/7 통과.
- Windows 검사도 공통 실행기를 통해 `bin/windows-x64`를 선택하도록 변경.
- 소스 ZIP에도 이제 두 운영체제의 `bin`이 포함된다. 앞 절의 소스 자동 빌드는 준비된 실행 파일이
  없을 때의 대체 경로를 검사한 기록이다.
- `build`, 새 로그와 시스템 검사 결과는 Git 업로드에서 제외하며 `bin`은 업로드 대상이다.

카메라 테스트 패턴은 `TEST_PATTERN`으로 표시된다. 사용자에게 받은 기존 Windows 로그에서는
카메라 1 / DirectShow / 640×480 영상이 정상 동작했지만, 이번 Mac 패키징 검사에서는 실제 USB 카메라나
Arduino를 새로 연결하지 않았다. FSR 실측 보정, ADC→그램 변환과 실제 센서 기반 혼잡도 연결은
미완료이며 혼잡도와 추천은 기존 Mock 입력을 사용한다.
