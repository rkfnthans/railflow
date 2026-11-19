#include <charconv>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include "CameraFrameReader.hpp"
#include "CameraPassengerDetector.hpp"

namespace
{
    int deviceIndex(std::string_view text)
    {
        int index{};
        const auto result = std::from_chars(text.data(), text.data() + text.size(), index);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || index < 0)
            throw std::invalid_argument("invalid camera index");
        return index;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3 && argc != 13)
        {
            std::cerr << "Usage: railflow_detect <image|video|camera> <path|index>"
                      << " [--homography h00 h01 h02 h10 h11 h12 h20 h21 h22]\n";
            return 1;
        }
        const std::string_view mode = argv[1];
        if (mode != "image" && mode != "video" && mode != "camera")
            throw std::invalid_argument("unknown media type");
        std::optional<CoordinateMappingConfig> mapping;
        if (argc == 13)
        {
            if (std::string_view{argv[3]} != "--homography")
                throw std::invalid_argument("unknown mapping option");
            mapping.emplace();
            for (std::size_t i = 0; i < mapping->homography.size(); ++i)
            {
                std::size_t consumed{};
                const std::string text = argv[i + 4];
                mapping->homography[i] = std::stod(text, &consumed);
                if (consumed != text.size()) throw std::invalid_argument("invalid matrix value");
            }
            // 매체를 열기 전에 잘못된 행렬을 거부한다.
            const CoordinateMapper validated{*mapping};
        }
        auto reader = mode == "image" ? CameraFrameReader::image(argv[2])
            : mode == "video" ? CameraFrameReader::video(argv[2])
            : CameraFrameReader::camera(deviceIndex(argv[2]));
        const MarkerDetectorConfig config; // API에서 현장 마커에 맞춰 설정 가능
        std::size_t frameIndex = 0;
        while (const auto frame = reader.read())
        {
            const auto mapper = mapping ? CoordinateMapper{*mapping}
                : CoordinateMapper::scaledFrame(frame->cols, frame->rows);
            const CameraPassengerDetector detector{config, mapper};
            const auto passengers = detector.detect(*frame);
            std::cout << "frame " << frameIndex++ << ": " << passengers.size() << " markers\n";
            for (const auto& passenger : passengers)
                std::cout << passenger.x << ',' << passenger.y << '\n';
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Marker detection: " << error.what() << '\n';
        return 1;
    }
}
