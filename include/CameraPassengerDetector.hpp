#pragma once

#include <vector>
#include <opencv2/core/mat.hpp>
#include "CoordinateMapper.hpp"
#include "MarkerDetectorConfig.hpp"
#include "Passenger.hpp"

// 차량 하중이나 InputFrame을 생성하지 않는 승객 위치 검출기.
class CameraPassengerDetector
{
public:
    CameraPassengerDetector(MarkerDetectorConfig config, CoordinateMapper mapper);
    // CV_8UC3 BGR 이미지. 모든 중심점은 mapper를 통과한다.
    // 변환 불가능한 점은 제외하고, canonical 범위 밖 점은 clamp하지 않는다.
    // 결과 순서는 canonical x, y 순서이며 프레임 간 ID/tracking을 의미하지 않는다.
    std::vector<Passenger> detect(const cv::Mat& frame) const;

private:
    MarkerDetectorConfig config_;
    CoordinateMapper mapper_;
};
