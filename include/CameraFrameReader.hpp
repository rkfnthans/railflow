#pragma once

#include <optional>
#include <string>
#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>

struct CameraCaptureOptions
{
    int apiPreference{cv::CAP_ANY};
    int width{};
    int height{};
    double framesPerSecond{};
};

// 영상 취득만 담당한다. detector와 차량 입력을 소유하지 않는다.
class CameraFrameReader
{
public:
    static CameraFrameReader image(const std::string& path);
    static CameraFrameReader video(const std::string& path);
    static CameraFrameReader camera(int deviceIndex, CameraCaptureOptions options = {});
    CameraFrameReader(const CameraFrameReader&) = delete;
    CameraFrameReader& operator=(const CameraFrameReader&) = delete;

    // 이미지 소진/영상 EOF는 nullopt. 카메라 읽기 실패는 예외.
    // VideoCapture는 파일 EOF와 decode 실패를 구분하지 않으므로 둘 다 nullopt다.
    std::optional<cv::Mat> read();
    std::string backendName() const;
    double nominalFramesPerSecond() const;

private:
    enum class Kind { Image, Video, Camera };
    CameraFrameReader(Kind kind, const std::string& path, int deviceIndex,
                      CameraCaptureOptions options = {});
    Kind kind_;
    bool finished_{false};
    cv::Mat image_;
    cv::VideoCapture capture_;
};
