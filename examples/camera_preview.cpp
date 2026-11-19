#include <algorithm>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <opencv2/core/version.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "CameraFrameReader.hpp"
#include "CameraPreviewOptions.hpp"
#include "ConsoleSupport.hpp"

namespace
{
    using Clock = std::chrono::steady_clock;
    constexpr const char* windowName = "RailFlow Camera Preview";

    struct OpenedCamera
    {
        std::unique_ptr<CameraFrameReader> reader;
        cv::Mat firstFrame;
    };

    std::vector<int> backends(CameraBackend requested)
    {
#ifdef _WIN32
        if (requested == CameraBackend::DirectShow) return {cv::CAP_DSHOW};
        if (requested == CameraBackend::MediaFoundation) return {cv::CAP_MSMF};
        return {cv::CAP_DSHOW, cv::CAP_MSMF};
#else
        if (requested != CameraBackend::Automatic)
            throw std::invalid_argument("dshow and msmf backends require Windows");
        return {cv::CAP_ANY};
#endif
    }

    OpenedCamera openCamera(int index, const CameraPreviewOptions& options)
    {
        for (const int backend : backends(options.backend))
        {
            try
            {
                const CameraCaptureOptions capture{backend, options.width, options.height,
                    static_cast<double>(options.framesPerSecond)};
                auto reader = std::unique_ptr<CameraFrameReader>(
                    new CameraFrameReader(CameraFrameReader::camera(index, capture)));
                auto frame = reader->read();
                if (!frame) throw std::runtime_error("Camera returned no frame");
                std::cout << "camera=" << index << " backend=" << reader->backendName()
                          << " actual_size=" << frame->cols << 'x' << frame->rows
                          << " nominal_fps=" << reader->nominalFramesPerSecond() << '\n' << std::flush;
                return {std::move(reader), std::move(*frame)};
            }
            catch (const std::exception& error)
            {
                std::cerr << "camera=" << index << " backend_id=" << backend
                          << " failed: " << error.what() << '\n';
            }
        }
        throw std::runtime_error("Camera unavailable. Check index, USB connection, camera permissions and other camera apps.");
    }

    cv::Mat testPattern(cv::Size size, std::size_t index)
    {
        cv::Mat frame(size, CV_8UC3, cv::Scalar{30, 30, 30});
        const std::vector<cv::Scalar> colors{{220, 220, 220}, {0, 220, 220}, {220, 220, 0},
            {0, 220, 0}, {220, 0, 220}, {0, 0, 220}, {220, 0, 0}};
        for (std::size_t bar = 0; bar < colors.size(); ++bar)
        {
            const int left = static_cast<int>(bar) * size.width / static_cast<int>(colors.size());
            const int right = static_cast<int>(bar + 1) * size.width / static_cast<int>(colors.size());
            cv::rectangle(frame, {left, 0}, {right, size.height - 1}, colors[bar], cv::FILLED);
        }
        const int x = static_cast<int>((index * 5) % static_cast<std::size_t>(size.width));
        cv::line(frame, {x, 65}, {x, size.height - 1}, {255, 255, 255}, 3);
        return frame;
    }

    void overlay(cv::Mat& frame, const std::string& source, double fps)
    {
        cv::rectangle(frame, {0, 0}, {frame.cols - 1, 56}, {25, 25, 25}, cv::FILLED);
        std::ostringstream status;
        status << source << " | " << frame.cols << 'x' << frame.rows << " | "
               << std::fixed << std::setprecision(1) << fps << " fps";
        cv::putText(frame, status.str(), {10, 22}, cv::FONT_HERSHEY_SIMPLEX, 0.55, {255, 255, 255}, 1, cv::LINE_AA);
        cv::putText(frame, "Q / Esc: close", {10, 45}, cv::FONT_HERSHEY_SIMPLEX, 0.5, {220, 220, 220}, 1, cv::LINE_AA);
    }

    void usage()
    {
        std::cout << "Usage: railflow_camera_preview [--camera 0] [--backend auto|dshow|msmf]\n"
                     "       [--width 640 --height 480 --fps 30] [--frames N] [--no-preview]\n"
                     "       railflow_camera_preview --probe [--backend auto|dshow|msmf]\n"
                     "       railflow_camera_preview --test-pattern [--frames N] [--no-preview]\n"
                     "Default: camera 0, 640x480 at requested 30 fps, continuous preview.\n"
                     "Windows auto tries DirectShow, then Media Foundation. Probe checks 0..3.\n"
                     "Q, Esc or window close stops the preview. Ctrl+C works in the console.\n"
                     "0 frames means continuous.\n";
    }
}

int main(int argc, char* argv[])
{
    try
    {
        const std::vector<std::string_view> arguments(argv + 1, argv + argc);
        const auto options = parseCameraPreviewOptions(arguments);
        if (options.help) { usage(); return 0; }
        installConsoleStopHandler();
        std::cout << "RailFlow camera preview | OpenCV " << CV_VERSION << '\n'
                  << "requested_size=" << options.width << 'x' << options.height
                  << " requested_fps=" << options.framesPerSecond << '\n' << std::flush;
        if (options.probe)
        {
            int available{};
            for (int index = 0; index < 4 && !consoleStopRequested(); ++index)
            {
                try { const auto camera = openCamera(index, options); ++available; }
                catch (const std::exception& error) { std::cerr << "camera=" << index << ' ' << error.what() << '\n'; }
            }
            std::cout << "available_cameras=" << available << '\n';
            return available > 0 || consoleStopRequested() ? 0 : 1;
        }

        OpenedCamera camera;
        std::string source = "TEST PATTERN";
        if (!options.testPattern)
        {
            camera = openCamera(options.cameraIndex, options);
            source = "Camera " + std::to_string(options.cameraIndex) + " / " + camera.reader->backendName();
        }
        std::cout << "source=" << (options.testPattern ? "TEST_PATTERN" : "LIVE_CAMERA") << '\n' << std::flush;
        if (options.preview)
        {
            cv::namedWindow(windowName, cv::WINDOW_NORMAL);
            cv::resizeWindow(windowName, std::min(options.width, 1280), std::min(options.height, 720));
        }

        std::size_t frames{};
        const auto start = Clock::now();
        auto fpsStart = start;
        std::size_t fpsFrames{};
        double measuredFps{};
        while (!consoleStopRequested() && (options.maxFrames == 0 || frames < options.maxFrames))
        {
            cv::Mat frame;
            if (options.testPattern) frame = testPattern({options.width, options.height}, frames);
            else if (!camera.firstFrame.empty()) frame = std::move(camera.firstFrame);
            else
            {
                auto next = camera.reader->read();
                if (!next) throw std::runtime_error("Camera stream ended");
                frame = std::move(*next);
            }
            ++frames;
            ++fpsFrames;
            const double interval = std::chrono::duration<double>(Clock::now() - fpsStart).count();
            if (interval >= 1.0)
            {
                measuredFps = static_cast<double>(fpsFrames) / interval;
                fpsStart = Clock::now();
                fpsFrames = 0;
            }
            if (options.preview)
            {
                overlay(frame, source, measuredFps);
                cv::imshow(windowName, frame);
                const int key = cv::waitKey(1) & 0xff;
                if (key == 'q' || key == 'Q' || key == 27
                    || cv::getWindowProperty(windowName, cv::WND_PROP_VISIBLE) < 1.0) break;
            }
            if (options.testPattern)
                std::this_thread::sleep_until(start + std::chrono::microseconds{
                    static_cast<long long>(frames) * 1000000 / options.framesPerSecond});
        }
        if (options.preview) cv::destroyAllWindows();
        camera.reader.reset();
        const double seconds = std::chrono::duration<double>(Clock::now() - start).count();
        std::cout << "frames=" << frames << " elapsed_sec=" << seconds
                  << " average_fps=" << (seconds > 0 ? static_cast<double>(frames) / seconds : 0.0)
                  << " stopped=" << (consoleStopRequested() ? "yes" : "no") << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Camera preview: " << error.what() << '\n';
        printCommandLineDiagnostics(argc, argv);
        return 1;
    }
}
