#include <iostream>
#include <limits>
#include <stdexcept>
#include <opencv2/imgproc.hpp>
#include "CameraPassengerDetector.hpp"
#include "ZoneConfig.hpp"
#include "ZoneManager.hpp"

int main()
{
    int passed = 0;
    int total = 0;
    const auto check = [&](bool result, const char* name)
    {
        ++total;
        passed += result;
        std::cout << (result ? "[PASS] " : "[FAIL] ") << name << '\n';
    };
    const auto blank = [] { return cv::Mat(300, 600, CV_8UC3, cv::Scalar::all(0)); };
    const cv::Scalar green{0, 255, 0};
    MarkerDetectorConfig config;
    const CameraPassengerDetector detector{config, CoordinateMapper::scaledFrame(600, 300)};
    check(detector.detect(blank()).empty(), "zero markers");

    auto frame = blank();
    cv::circle(frame, {100, 260}, 12, green, cv::FILLED);
    const auto before = frame.clone();
    auto passengers = detector.detect(frame);
    check(passengers.size() == 1 && passengers[0].x == 200 && passengers[0].y == 520,
          "one marker centroid passes through mapper");
    check(cv::norm(frame, before, cv::NORM_INF) == 0.0, "input frame remains unchanged");
    cv::circle(frame, {300, 260}, 12, green, cv::FILLED);
    cv::circle(frame, {500, 260}, 12, green, cv::FILLED);
    passengers = detector.detect(frame);
    check(passengers.size() == 3 && passengers[0].x == 200
          && passengers[1].x == 600 && passengers[2].x == 1000, "multiple markers sorted");
    auto zones = ZoneConfig::createDefaultZones();
    const auto unassigned = countPassengersInZones(passengers, zones);
    check(unassigned == 0 && zones[0].detectedModelPassengers == 1
          && zones[1].detectedModelPassengers == 1 && zones[2].detectedModelPassengers == 1,
          "canonical detection to ZoneManager");

    auto noise = blank();
    cv::circle(noise, {100, 100}, 2, green, cv::FILLED);
    check(detector.detect(noise).empty(), "tiny noise filtered");
    auto large = blank();
    cv::circle(large, {200, 150}, 50, green, cv::FILLED);
    check(detector.detect(large).empty(), "oversized blob filtered");
    auto wrongColor = blank();
    cv::circle(wrongColor, {100, 100}, 12, {255, 0, 0}, cv::FILLED);
    check(detector.detect(wrongColor).empty(), "non-marker color rejected");

    auto morphologyConfig = config;
    morphologyConfig.minArea = 0.1;
    morphologyConfig.openingIterations = 0;
    morphologyConfig.closingIterations = 0;
    const CameraPassengerDetector noMorphology{morphologyConfig, CoordinateMapper{}};
    morphologyConfig.morphologyKernelSize = 7;
    morphologyConfig.openingIterations = 1;
    const CameraPassengerDetector opening{morphologyConfig, CoordinateMapper{}};
    check(noMorphology.detect(noise).size() == 1 && opening.detect(noise).empty(),
          "configurable opening removes noise independently of area filter");

    auto redConfig = config;
    redConfig.hsvLower = {170, 100, 80};
    redConfig.hsvUpper = {10, 255, 255};
    const CameraPassengerDetector red{redConfig, CoordinateMapper{}};
    auto redFrame = blank();
    cv::circle(redFrame, {100, 100}, 12, {0, 0, 255}, cv::FILLED);
    check(red.detect(redFrame).size() == 1 && detector.detect(redFrame).empty(),
          "HSV hue wrap-around configuration");

    const CoordinateMapper translated{CoordinateMappingConfig{{1, 0, 1200, 0, 1, 0, 0, 0, 1}}};
    const CameraPassengerDetector outside{config, translated};
    passengers = outside.detect(frame);
    bool outsidePreserved = passengers.size() == 3;
    for (const auto& passenger : passengers)
        outsidePreserved = outsidePreserved && passenger.x >= ZoneConfig::FRAME_WIDTH
            && classifyPassenger(passenger, zones) == -1;
    check(outsidePreserved, "outside canonical frame preserved and UNASSIGNED");

    const CoordinateMapper horizon{CoordinateMappingConfig{{1, 0, 0, 0, 1, 0, 1, 0, -100}}};
    const CameraPassengerDetector unmappable{config, horizon};
    auto single = blank();
    cv::circle(single, {100, 100}, 12, green, cv::FILLED);
    check(unmappable.detect(single).empty(), "unmappable centroid excluded");

    const auto invalidConfig = [&](MarkerDetectorConfig invalid)
    {
        try { CameraPassengerDetector rejected{invalid, CoordinateMapper{}}; }
        catch (const std::invalid_argument&) { return true; }
        return false;
    };
    auto invalid = config;
    invalid.morphologyKernelSize = 2;
    check(invalidConfig(invalid), "even morphology kernel rejected");
    invalid = config;
    invalid.hsvUpper[0] = 180;
    check(invalidConfig(invalid), "out-of-range HSV rejected");
    invalid = config;
    invalid.minArea = config.maxArea + 1;
    check(invalidConfig(invalid), "reversed area range rejected");
    invalid = config;
    invalid.maxArea = std::numeric_limits<double>::quiet_NaN();
    check(invalidConfig(invalid), "non-finite area rejected");
    invalid = config;
    invalid.closingIterations = -1;
    check(invalidConfig(invalid), "negative morphology iterations rejected");

    bool emptyRejected = false;
    try { detector.detect(cv::Mat{}); }
    catch (const std::invalid_argument&) { emptyRejected = true; }
    check(emptyRejected, "empty frame is an input error, not zero passengers");
    bool formatRejected = false;
    try { detector.detect(cv::Mat::zeros(20, 20, CV_8UC1)); }
    catch (const std::invalid_argument&) { formatRejected = true; }
    check(formatRejected, "unsupported image format rejected");

    std::cout << "Checks passed: " << passed << '/' << total << '\n';
    return passed == total ? 0 : 1;
}
