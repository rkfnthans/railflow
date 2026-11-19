# Windows USB 카메라 미리보기

이번 실행 도구는 USB 카메라 연결과 영상 표시를 확인한다. 현재 Windows PC에 연결된 카메라는
사용자가 실행 패키지에서 검사한다. 승객 검출은 기존 `CameraPassengerDetector`로 이어서 연결할 수 있다.

## 실행 패키지

ZIP 전체를 쓰기 가능한 폴더에 압축 해제한 뒤 다음 파일을 실행한다.

1. `TEST_DISPLAY.cmd`는 생성한 색상 막대와 움직이는 선을 5초간 표시한다.
   창에 `TEST PATTERN`이라고 표시되며 카메라가 연결되지 않은 상태에서도 검사할 수 있다.
2. `CHECK_CAMERAS.cmd`는 카메라 번호 0~3에서 실제 프레임 수신을 시도하고 사용 가능한 번호를 출력한다.
3. `START_CAMERA.cmd`에서 번호와 backend를 선택한다. 노트북 기본 카메라가 0일 수 있으므로
   USB 카메라가 보이지 않으면 1, 2, 3을 차례로 확인한다. 실제 번호는 연결 상태와 backend에 따라 달라진다.

backend의 `A`는 DirectShow를 먼저 시도하고 Media Foundation을 이어서 시도한다.
`D`와 `M`은 각각 한 backend만 사용한다. 화면에 표시되는 카메라 번호, backend, 해상도와
측정 FPS를 확인한다. 처음 1초간 FPS는 0.0으로 표시되며 수신 후 갱신된다.

미리보기 창에서 Q, Esc 또는 창 닫기로 종료한다. 콘솔에서는 Ctrl+C로 종료할 수 있다.
종료 후 창과 카메라 포트가 해제된다.
실행 로그는 매번 `camera-logs`에 새 파일로 저장된다. 확인이 필요하면 해당 로그와
실제 USB 카메라 화면이 보였는지를 전달한다.

## PowerShell 명령

```powershell
.\railflow_camera_preview.exe --probe
.\railflow_camera_preview.exe --camera 1
.\railflow_camera_preview.exe --camera 1 --backend dshow
.\railflow_camera_preview.exe --camera 1 --backend msmf
.\railflow_camera_preview.exe --camera 1 --width 1280 --height 720 --fps 30
.\railflow_camera_preview.exe --camera 1 --frames 60 --no-preview
```

기본 요청은 640×480, 30 FPS다. 카메라와 드라이버가 지원하는 모드로 협상되므로
실제 프레임 크기와 nominal FPS를 로그에 따로 출력한다. 화면의 FPS는 실제 수신 속도다.
`--frames 0`은 연속 수신이며 기본값이다.

## 연결 문제 확인

Windows 설정의 **개인 정보 및 보안 → 카메라**에서 카메라 액세스와 데스크톱 앱의 카메라 액세스를
허용한다. 다른 카메라 앱이 장치를 사용 중이면 종료한 뒤 다시 실행한다.
[Microsoft 카메라 권한 안내](https://support.microsoft.com/en-us/windows/privacy/manage-app-permissions-for-a-camera-in-windows)

| 증상 | 확인 방법 |
| --- | --- |
| 노트북 카메라가 표시됨 | 다른 카메라 번호를 선택하고 `CHECK_CAMERAS.cmd` 결과를 확인한다. |
| 열기 실패 또는 검은 화면 | Windows 기본 카메라 앱에서 USB 카메라 영상을 먼저 확인하고, 앱을 닫은 뒤 D/M backend를 각각 시도한다. |
| 낮은 FPS 또는 모드 협상 실패 | 기본 640×480으로 검사하고 USB 포트와 케이블, 조명, 카메라 지원 해상도를 확인한다. |
| 장치 분리 후 오류 | 오류로 종료된 로그를 보관하고 카메라를 다시 연결해 새 실행을 시작한다. |
| 초기 열기나 읽기가 오래 걸림 | 해당 USB 드라이버 호출이 지연될 수 있다. 다른 backend와 카메라 앱의 동작을 확인한다. |

카메라 backend는 OpenCV의 VideoCapture API를 사용한다. 장치/드라이버에 따라 속성 설정과
호출 지연이 달라진다. [OpenCV VideoCapture 문서](https://docs.opencv.org/4.x/d8/dfe/classcv_1_1VideoCapture.html)

## 소스 빌드

C++용 OpenCV 4 개발 패키지의 core, imgproc, imgcodecs, videoio, highgui 모듈을 준비한다.
`OpenCV_DIR`은 설치본의 `OpenCVConfig.cmake`가 있는 위치로 지정한다.

```powershell
cmake -S . -B build-camera -G "Visual Studio 17 2022" -A x64 -DRAILFLOW_ENABLE_OPENCV=ON -DRAILFLOW_ENABLE_CAMERA_PREVIEW=ON -DOpenCV_DIR=C:\opencv\build -DBUILD_TESTING=ON
cmake --build build-camera --config Release --parallel
ctest --test-dir build-camera -C Release --output-on-failure
.\build-camera\Release\railflow_camera_preview.exe --camera 1
```

직접 빌드한 OpenCV가 DLL 구성이라면 그 빌드의 DLL을 실행 파일 옆에 배치하거나 해당 bin 경로를
PATH에 추가한다. 같은 아키텍처와 호환되는 컴파일러 구성을 사용한다.
화면 표시가 필요 없는 기존 검출 도구만 빌드하려면 `RAILFLOW_ENABLE_CAMERA_PREVIEW=OFF`로 설정한다.
OpenCV 없이 기본 Mock/시리얼 프로그램을 빌드하는 `RAILFLOW_ENABLE_OPENCV=OFF` 구성도 사용할 수 있다.
