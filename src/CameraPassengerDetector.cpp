#include "CameraPassengerDetector.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <opencv2/imgproc.hpp>

CameraPassengerDetector::CameraPassengerDetector(
    MarkerDetectorConfig config, CoordinateMapper mapper)
    : config_(config), mapper_(std::move(mapper))
{
    for (std::size_t i = 0; i < config_.hsvLower.size(); ++i)
    {
        const int maximum = i == 0 ? 179 : 255;
        if (config_.hsvLower[i] < 0 || config_.hsvLower[i] > maximum
            || config_.hsvUpper[i] < 0 || config_.hsvUpper[i] > maximum
            || (i != 0 && config_.hsvLower[i] > config_.hsvUpper[i]))
        {
            throw std::invalid_argument("invalid marker HSV range");
        }
    }
    if (!std::isfinite(config_.minArea) || !std::isfinite(config_.maxArea)
        || config_.minArea < 0.0 || config_.maxArea < config_.minArea
        || config_.morphologyKernelSize <= 0 || config_.morphologyKernelSize % 2 == 0
        || config_.openingIterations < 0 || config_.closingIterations < 0)
    {
        throw std::invalid_argument("invalid marker area or morphology config");
    }
}

std::vector<Passenger> CameraPassengerDetector::detect(const cv::Mat& frame) const
{
    if (frame.empty() || frame.dims != 2 || frame.type() != CV_8UC3)
    {
        throw std::invalid_argument("marker detector requires a nonempty CV_8UC3 BGR frame");
    }
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    const auto& low = config_.hsvLower;
    const auto& high = config_.hsvUpper;
    cv::Mat mask;
    if (low[0] <= high[0])
    {
        cv::inRange(hsv, cv::Scalar(low[0], low[1], low[2]),
                    cv::Scalar(high[0], high[1], high[2]), mask);
    }
    else
    {
        cv::Mat second;
        cv::inRange(hsv, cv::Scalar(low[0], low[1], low[2]),
                    cv::Scalar(179, high[1], high[2]), mask);
        cv::inRange(hsv, cv::Scalar(0, low[1], low[2]),
                    cv::Scalar(high[0], high[1], high[2]), second);
        cv::bitwise_or(mask, second, mask);
    }
    const auto kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE,
        cv::Size(config_.morphologyKernelSize, config_.morphologyKernelSize));
    if (config_.openingIterations > 0)
    {
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel,
                         cv::Point(-1, -1), config_.openingIterations);
    }
    if (config_.closingIterations > 0)
    {
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel,
                         cv::Point(-1, -1), config_.closingIterations);
    }
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    std::vector<Passenger> passengers;
    for (const auto& contour : contours)
    {
        const double area = cv::contourArea(contour);
        if (area < config_.minArea || area > config_.maxArea) continue;
        const auto moments = cv::moments(contour);
        if (moments.m00 == 0.0) continue;
        const auto passenger = mapper_.map({moments.m10 / moments.m00,
                                            moments.m01 / moments.m00});
        if (passenger) passengers.push_back(*passenger);
    }
    std::sort(passengers.begin(), passengers.end(), [](const auto& a, const auto& b)
    {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    });
    return passengers;
}
