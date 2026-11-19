#include "CameraFrameReader.hpp"

#include <filesystem>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <opencv2/imgcodecs.hpp>

CameraFrameReader CameraFrameReader::image(const std::string& path)
{
    return CameraFrameReader{Kind::Image, path, 0};
}

CameraFrameReader CameraFrameReader::video(const std::string& path)
{
    return CameraFrameReader{Kind::Video, path, 0};
}

CameraFrameReader CameraFrameReader::camera(int deviceIndex, CameraCaptureOptions options)
{
    return CameraFrameReader{Kind::Camera, {}, deviceIndex, options};
}

CameraFrameReader::CameraFrameReader(Kind kind, const std::string& path, int deviceIndex,
                                     CameraCaptureOptions options)
    : kind_(kind)
{
    if (kind != Kind::Camera && !std::filesystem::is_regular_file(path))
    {
        throw std::runtime_error("camera media file does not exist: " + path);
    }
    if (kind == Kind::Image)
    {
        image_ = cv::imread(path, cv::IMREAD_COLOR);
        if (image_.empty()) throw std::runtime_error("cannot decode image: " + path);
    }
    else if (kind == Kind::Video)
    {
        if (!capture_.open(path)) throw std::runtime_error("cannot open video: " + path);
    }
    else
    {
        if (deviceIndex < 0) throw std::invalid_argument("camera index must be nonnegative");
        if (options.width < 0 || options.height < 0 || !std::isfinite(options.framesPerSecond)
            || options.framesPerSecond < 0)
            throw std::invalid_argument("invalid camera capture settings");
        if (!capture_.open(deviceIndex, options.apiPreference))
            throw std::runtime_error("cannot open camera index " + std::to_string(deviceIndex));
        // Drivers can negotiate a different mode. The preview reports actual frame dimensions.
        if (options.width > 0) (void)capture_.set(cv::CAP_PROP_FRAME_WIDTH, options.width);
        if (options.height > 0) (void)capture_.set(cv::CAP_PROP_FRAME_HEIGHT, options.height);
        if (options.framesPerSecond > 0) (void)capture_.set(cv::CAP_PROP_FPS, options.framesPerSecond);
    }
}

std::optional<cv::Mat> CameraFrameReader::read()
{
    if (finished_) return std::nullopt;
    if (kind_ == Kind::Image)
    {
        finished_ = true;
        return std::move(image_);
    }
    cv::Mat frame;
    if (!capture_.read(frame) || frame.empty())
    {
        if (kind_ == Kind::Camera) throw std::runtime_error("camera frame read failed");
        finished_ = true;
        return std::nullopt;
    }
    return frame;
}

std::string CameraFrameReader::backendName() const
{
    return kind_ == Kind::Image ? "IMAGE" : capture_.getBackendName();
}

double CameraFrameReader::nominalFramesPerSecond() const
{
    return kind_ == Kind::Image ? 0.0 : capture_.get(cv::CAP_PROP_FPS);
}
