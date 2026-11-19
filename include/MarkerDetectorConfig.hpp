#pragma once

#include <array>

struct MarkerDetectorConfig
{
    // OpenCV 8-bit HSV: H=0..179, S/V=0..255. 기본값은 녹색 마커 예시.
    // lower H > upper H이면 179/0 경계를 가로지르는 색상 범위다.
    std::array<int, 3> hsvLower{40, 100, 80};
    std::array<int, 3> hsvUpper{85, 255, 255};

    // morphology 이후 contour 면적(raw camera pixel^2), 양 끝 포함.
    double minArea{50.0};
    double maxArea{5000.0};
    int morphologyKernelSize{3}; // 양의 홀수, elliptical kernel
    int openingIterations{1};
    int closingIterations{1}; // 0이면 해당 morphology 단계 생략
};
