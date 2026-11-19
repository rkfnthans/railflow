# 색상 마커 기반 승객 검출

## 빌드

OpenCV 4 C++ 개발 패키지의 core, imgproc, imgcodecs, videoio 모듈이 필요하다.
카메라 미리보기를 함께 빌드하는 기본 ON 구성에는 highgui도 필요하다.
`RAILFLOW_ENABLE_CAMERA_PREVIEW=OFF`로 설정하면 기존 파일/콘솔 검출 도구를 highgui 없이 빌드한다.
기본 빌드는 OpenCV를 찾거나 다운로드하지 않는다.

```sh
cmake -S . -B build -DRAILFLOW_ENABLE_OPENCV=OFF
cmake --build build
ctest --test-dir build --output-on-failure

cmake -S . -B build/opencv -DRAILFLOW_ENABLE_OPENCV=ON -DOpenCV_DIR=/path/to/lib/cmake/opencv4
cmake --build build/opencv
ctest --test-dir build/opencv --output-on-failure
```

`railflow_core`와 기본 `railflow`는 OpenCV를 링크하지 않는다.
ON인 경우 별도 `railflow_camera`, `railflow_detect`, 카메라 테스트를 생성한다.
미리보기 옵션을 켜면 `railflow_camera_preview`도 생성한다.
Windows 실행 패키지는 [USB_CAMERA_WINDOWS.md](USB_CAMERA_WINDOWS.md)를 참고한다.
ON인데 개발 패키지가 없으면 configure에서 실패하며 Mock으로 조용히 대체하지 않는다.
core와 InputFrame, 기본 Mock 실행 경로는 그대로 유지한다.

## 책임

`CameraFrameReader → BGR frame → CameraPassengerDetector → canonical Passenger 목록`

CameraFrameReader는 PNG 등 정지 이미지, prerecorded video, 카메라 장치를 읽는다.
이미지는 한 번만 반환한다. 영상 소진 이후에는 이전 프레임을 재사용하지 않는다.
파일 읽기 실패/카메라 읽기 실패는 오류다. OpenCV VideoCapture 파일 backend의
read 실패는 EOF와 decode 실패를 구분하지 못하므로 둘 다 nullopt로 반환한다.
카메라 읽기 timeout과 Ctrl+C 처리는 이 작은 파일 검출 예제에 아직 없다.

CameraPassengerDetector는 BGR→HSV→색상 threshold→opening→closing→외부 contour
→면적 필터→contour moments 중심→CoordinateMapper 순서로 처리한다.
차량 하중과 InputFrame을 만들지 않으며 InputSource를 상속하지 않는다.
향후 SerialVehicleLoadSource와 합성할 CompositeInputSource는 별도 단계다.

## Config

`MarkerDetectorConfig`의 기본값은 **녹색 마커 예시**이며 현장 조명·마커로 보정해야 한다.

| 항목 | 기본값 | 의미 |
| --- | --- | --- |
| hsvLower | 40, 100, 80 | H/S/V 최소값 |
| hsvUpper | 85, 255, 255 | H/S/V 최대값 |
| minArea / maxArea | 50 / 5000 | morphology 후 contour 면적, raw pixel², 양 끝 포함 |
| morphologyKernelSize | 3 | elliptical 커널 한 변 길이, 양의 홀수 |
| openingIterations | 1 | 작은 노이즈 제거, 0이면 생략 |
| closingIterations | 1 | 작은 구멍 메우기, 0이면 생략 |

H는 0..179, S/V는 0..255다. H 최소값이 최대값보다 크면 179/0 경계를
넘는 범위로 처리한다(예: 빨강 H=170..10). S/V 역전, 잘못된 면적·커널은 거부한다.
서로 닿는 같은 색 마커는 하나의 contour가 될 수 있으므로 실제 배치 검증이 필요하다.

```cpp
MarkerDetectorConfig config;
config.hsvLower = {170, 100, 80};
config.hsvUpper = {10, 255, 255};
config.minArea = 80.0;
config.maxArea = 3000.0;
config.morphologyKernelSize = 5;
CameraPassengerDetector detector{config, CoordinateMapper{calibratedMapping}};
auto passengers = detector.detect(bgrFrame);
```

검출 중심점은 항상 mapper를 통과한다. 투영 불가능한 점은 제외한다.
canonical 범위 밖 점은 clamp하지 않으며 ZoneManager에서 UNASSIGNED가 된다.
결과는 canonical x/y 순서로 정렬한다. 추적 ID나 사람 재식별 기능은 없다.
모형 위 마커의 중심이 승객 위치다. YOLO, 신체 바닥 접점 추정은 사용하지 않는다.

## 카메라 없이 실행

```sh
./build/opencv/railflow_detect image /path/to/markers.png
./build/opencv/railflow_detect video /path/to/recording.avi
./build/opencv/railflow_detect camera 0
./build/opencv/railflow_detect image /path/to/markers.png --homography 1 0 0 0 1 0 0 0 1
```

예제 도구의 검출 config는 위 기본 녹색 값이다. 다른 threshold는 API 설정으로 전달한다.
행렬을 생략하면 **전체 raw frame을 1200×600으로 단순 크기 변환**한다.
이는 정면 테스트 영상용이며 실제 광각 카메라의 원근/렌즈 보정을 의미하지 않는다.
`--homography`는 이미 얻은 raw→canonical 3×3 행렬을 row-major 순서로 받는다.
행렬 추정 UI와 렌즈 보정은 구현하지 않았다.

이미지/영상 테스트는 테스트마다 임시 PNG와 실제 MJPEG AVI 파일을 생성하고 삭제한다.
0/1/여러 마커, 작은 noise, 큰 blob, 좌표 변환·범위 밖 판정, image/video EOF 등을
검증하며 USB 카메라와 외부 fixture 다운로드는 필요하지 않다.

## 참고

OpenCV 공식 문서의 [HSV inRange](https://docs.opencv.org/4.x/da/d97/tutorial_threshold_inRange.html),
[morphology](https://docs.opencv.org/4.x/d9/d61/tutorial_py_morphological_ops.html),
[contour moments](https://docs.opencv.org/4.x/dd/d49/tutorial_py_contour_features.html)를 사용한다.
