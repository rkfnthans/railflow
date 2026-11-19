#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include "CameraFrameReader.hpp"
#include "CameraPassengerDetector.hpp"

namespace
{
    class Fixtures
    {
    public:
        Fixtures()
        {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            for (int attempt = 0; attempt < 100; ++attempt)
            {
                const auto candidate = std::filesystem::temp_directory_path()
                    / ("railflow-camera-tests-" + std::to_string(stamp) + "-" + std::to_string(attempt));
                if (std::filesystem::create_directory(candidate))
                {
                    directory = candidate;
                    return;
                }
            }
            throw std::runtime_error("cannot create camera test fixtures directory");
        }
        ~Fixtures()
        {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }
        std::filesystem::path directory;
    };
}

int main()
{
    try
    {
        Fixtures fixtures;
        int total = 0;
        int passed = 0;
        const auto check = [&](bool result, const char* name)
        {
            ++total;
            passed += result;
            std::cout << (result ? "[PASS] " : "[FAIL] ") << name << '\n';
        };
        const CameraPassengerDetector detector{MarkerDetectorConfig{},
                                               CoordinateMapper::scaledFrame(320, 240)};
        cv::Mat single = cv::Mat::zeros(240, 320, CV_8UC3);
        cv::circle(single, {80, 120}, 12, {0, 255, 0}, cv::FILLED);
        const auto imagePath = (fixtures.directory / "markers.png").string();
        if (!cv::imwrite(imagePath, single)) throw std::runtime_error("cannot write PNG fixture");
        auto image = CameraFrameReader::image(imagePath);
        const auto frame = image.read();
        const auto passengers = frame ? detector.detect(*frame) : std::vector<Passenger>{};
        check(passengers.size() == 1 && passengers[0].x == 300 && passengers[0].y == 300,
              "PNG file read, detect and canonical mapping");
        check(!image.read() && !image.read(), "image consumed once");
        check(image.backendName() == "IMAGE" && image.nominalFramesPerSecond() == 0,
              "image metadata does not require a capture backend");

        const auto videoPath = (fixtures.directory / "markers.avi").string();
        // OpenCV 내장 MJPEG backend로 실제 파일을 기록한 뒤 다시 읽는다.
        cv::VideoWriter writer(videoPath, cv::CAP_OPENCV_MJPEG,
            cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 5.0, {320, 240});
        if (!writer.isOpened()) throw std::runtime_error("MJPEG fixture writer unavailable");
        writer.write(cv::Mat::zeros(240, 320, CV_8UC3));
        writer.write(single);
        auto multiple = single.clone();
        cv::circle(multiple, {160, 120}, 12, {0, 255, 0}, cv::FILLED);
        writer.write(multiple);
        writer.release();

        auto video = CameraFrameReader::video(videoPath);
        check(!video.backendName().empty() && std::abs(video.nominalFramesPerSecond() - 5.0) < 0.1,
              "capture backend and nominal FPS reported");
        for (std::size_t expectedCount = 0; expectedCount < 3; ++expectedCount)
        {
            const auto videoFrame = video.read();
            const auto detections = videoFrame ? detector.detect(*videoFrame) : std::vector<Passenger>{};
            bool correct = videoFrame.has_value() && detections.size() == expectedCount;
            if (expectedCount >= 1 && !detections.empty())
                correct = correct && std::abs(detections[0].x - 300) <= 4
                    && std::abs(detections[0].y - 300) <= 4;
            if (expectedCount == 2 && detections.size() == 2)
                correct = correct && std::abs(detections[1].x - 600) <= 4
                    && std::abs(detections[1].y - 300) <= 4;
            check(correct, "prerecorded MJPEG video frame detection and mapping");
        }
        check(!video.read() && !video.read(), "video EOF stable, no stale frame reuse");
        bool missingRejected = false;
        try { auto missing = CameraFrameReader::image((fixtures.directory / "missing.png").string()); }
        catch (const std::runtime_error&) { missingRejected = true; }
        check(missingRejected, "missing image reported as error");
        bool invalidDeviceRejected = false;
        try { auto camera = CameraFrameReader::camera(-1); }
        catch (const std::invalid_argument&) { invalidDeviceRejected = true; }
        check(invalidDeviceRejected, "invalid camera index rejected without hardware access");
        bool invalidSettingsRejected = false;
        try { auto camera = CameraFrameReader::camera(0, {cv::CAP_ANY, -1, 480, 30}); }
        catch (const std::invalid_argument&) { invalidSettingsRejected = true; }
        check(invalidSettingsRejected, "invalid capture settings rejected before opening hardware");
        std::cout << "Checks passed: " << passed << '/' << total << '\n';
        return passed == total ? 0 : 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Camera frame fixture test failed: " << error.what() << '\n';
        return 1;
    }
}
