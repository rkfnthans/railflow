cmake_minimum_required(VERSION 3.20)

project(RailFlow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


function(railflow_enable_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /utf-8)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
        target_compile_options(
            ${target_name}
            PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()
endfunction()


add_library(
    railflow_core
    STATIC
    src/ZoneManager.cpp
    src/ZoneConfig.cpp
    src/VehicleLoadEstimator.cpp
    src/PlatformCrowdEstimator.cpp
    src/GuidanceEngine.cpp
    src/Logger.cpp
    src/MockInput.cpp
    src/RailFlowEngine.cpp
    src/ConsoleReporter.cpp
    src/UpdateLoop.cpp
    src/RuntimeOptions.cpp
    src/TrainLayoutReporter.cpp
    src/CoordinateMapper.cpp
)

target_include_directories(
    railflow_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

railflow_enable_warnings(railflow_core)

find_package(Threads REQUIRED)
target_link_libraries(railflow_core PUBLIC Threads::Threads)


add_executable(
    railflow
    src/main.cpp
)

add_library(railflow_console STATIC src/ConsoleSupport.cpp)
target_include_directories(railflow_console PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
railflow_enable_warnings(railflow_console)

target_link_libraries(
    railflow
    PRIVATE
    railflow_core
    railflow_console
)

railflow_enable_warnings(railflow)


include(CTest)

add_library(railflow_serial STATIC
    src/SerialLineBuffer.cpp
    src/SerialFrameParser.cpp
    src/SerialFrameTracker.cpp
    src/RawAdcCsvRecorder.cpp
    src/SerialMonitor.cpp
)
target_include_directories(railflow_serial PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
railflow_enable_warnings(railflow_serial)

add_library(railflow_calibration STATIC
    src/CalibrationDataset.cpp
    src/CalibrationCapture.cpp
    src/CalibrationOptions.cpp
)
target_link_libraries(railflow_calibration PUBLIC railflow_serial)
railflow_enable_warnings(railflow_calibration)

# Native serial APIs are isolated from the core, RF1 parser and OpenCV modules.
if(WIN32)
    add_library(railflow_win32_serial STATIC src/Win32SerialLineTransport.cpp src/Win32SerialPort.cpp)
    target_link_libraries(railflow_win32_serial PUBLIC railflow_serial)
    railflow_enable_warnings(railflow_win32_serial)
    set(railflow_native_serial railflow_win32_serial)
elseif(UNIX)
    add_library(railflow_posix_serial STATIC src/PosixSerialLineTransport.cpp)
    target_link_libraries(railflow_posix_serial PUBLIC railflow_serial)
    railflow_enable_warnings(railflow_posix_serial)
    set(railflow_native_serial railflow_posix_serial)
endif()

if(railflow_native_serial)
    add_executable(railflow_serial_monitor examples/serial_monitor.cpp)
    target_link_libraries(railflow_serial_monitor PRIVATE ${railflow_native_serial} railflow_console)
    railflow_enable_warnings(railflow_serial_monitor)
    add_executable(railflow_calibration_record examples/calibration_record.cpp)
    target_link_libraries(railflow_calibration_record PRIVATE railflow_calibration ${railflow_native_serial} railflow_console)
    railflow_enable_warnings(railflow_calibration_record)
    if(BUILD_TESTING)
        add_test(NAME SerialMonitorHelpTest COMMAND railflow_serial_monitor --help)
        add_test(NAME CalibrationRecorderHelpTest COMMAND railflow_calibration_record --help)
        add_test(NAME CliSmokeTests COMMAND ${CMAKE_COMMAND}
            "-DRAILFLOW_EXECUTABLE=$<TARGET_FILE:railflow>"
            "-DRAILFLOW_MONITOR=$<TARGET_FILE:railflow_serial_monitor>"
            "-DRAILFLOW_CALIBRATION=$<TARGET_FILE:railflow_calibration_record>"
            "-DRAILFLOW_RUNNER=${CMAKE_CROSSCOMPILING_EMULATOR}"
            "-DRAILFLOW_TEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/cli-check"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/CliSmokeTests.cmake")
        set_tests_properties(CliSmokeTests PROPERTIES TIMEOUT 40)
    endif()
endif()

if(UNIX AND BUILD_TESTING)
    add_executable(calibration_cli_tests tests/CalibrationCliTests.cpp)
    target_link_libraries(calibration_cli_tests PRIVATE railflow_calibration Threads::Threads)
    railflow_enable_warnings(calibration_cli_tests)
    add_dependencies(calibration_cli_tests railflow_calibration_record)
    add_test(NAME CalibrationCliTests COMMAND calibration_cli_tests $<TARGET_FILE:railflow_calibration_record>)
    set_tests_properties(CalibrationCliTests PROPERTIES TIMEOUT 15)

    add_executable(posix_serial_transport_tests tests/PosixSerialTransportTests.cpp)
    target_link_libraries(posix_serial_transport_tests PRIVATE railflow_posix_serial Threads::Threads)
    railflow_enable_warnings(posix_serial_transport_tests)
    add_test(NAME PosixSerialTransportTests COMMAND posix_serial_transport_tests)
    set_tests_properties(PosixSerialTransportTests PROPERTIES TIMEOUT 10)
endif()

option(RAILFLOW_ENABLE_OPENCV "Build optional color-marker passenger detection" OFF)
option(RAILFLOW_ENABLE_CAMERA_PREVIEW "Build camera preview with OpenCV highgui" ON)
if(RAILFLOW_ENABLE_OPENCV)
    set(railflow_opencv_components core imgproc imgcodecs videoio)
    if(RAILFLOW_ENABLE_CAMERA_PREVIEW)
        list(APPEND railflow_opencv_components highgui)
    endif()
    find_package(OpenCV 4 REQUIRED COMPONENTS ${railflow_opencv_components})
    add_library(railflow_camera STATIC
        src/CameraPassengerDetector.cpp
        src/CameraFrameReader.cpp
        src/CameraPreviewOptions.cpp
    )
    target_include_directories(railflow_camera SYSTEM PUBLIC ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(railflow_camera PUBLIC railflow_core ${OpenCV_LIBS})
    railflow_enable_warnings(railflow_camera)

    add_executable(railflow_detect examples/marker_detection.cpp)
    target_link_libraries(railflow_detect PRIVATE railflow_camera)
    railflow_enable_warnings(railflow_detect)

    if(RAILFLOW_ENABLE_CAMERA_PREVIEW)
        add_executable(railflow_camera_preview examples/camera_preview.cpp)
        target_link_libraries(railflow_camera_preview PRIVATE railflow_camera railflow_console)
        railflow_enable_warnings(railflow_camera_preview)
        if(BUILD_TESTING)
            add_test(NAME CameraPreviewCliTests COMMAND ${CMAKE_COMMAND}
                "-DRAILFLOW_PREVIEW=$<TARGET_FILE:railflow_camera_preview>"
                "-DRAILFLOW_RUNNER=${CMAKE_CROSSCOMPILING_EMULATOR}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/CameraPreviewCliTests.cmake")
            set_tests_properties(CameraPreviewCliTests PROPERTIES TIMEOUT 30)
        endif()
    endif()

    if(BUILD_TESTING)
        add_executable(camera_preview_options_tests tests/CameraPreviewOptionsTests.cpp)
        target_link_libraries(camera_preview_options_tests PRIVATE railflow_camera)
        railflow_enable_warnings(camera_preview_options_tests)
        add_test(NAME CameraPreviewOptionsTests COMMAND camera_preview_options_tests)

        add_executable(camera_passenger_detector_tests tests/CameraPassengerDetectorTests.cpp)
        target_link_libraries(camera_passenger_detector_tests PRIVATE railflow_camera)
        railflow_enable_warnings(camera_passenger_detector_tests)
        add_test(NAME CameraPassengerDetectorTests COMMAND camera_passenger_detector_tests)

        add_executable(camera_frame_reader_tests tests/CameraFrameReaderTests.cpp)
        target_link_libraries(camera_frame_reader_tests PRIVATE railflow_camera)
        railflow_enable_warnings(camera_frame_reader_tests)
        add_test(NAME CameraFrameReaderTests COMMAND camera_frame_reader_tests)
    endif()
endif()

if(BUILD_TESTING)
    # Compile the same Windows framing/read loop with a fake COM byte source on every host.
    add_executable(win32_serial_transport_tests tests/Win32SerialTransportTests.cpp src/Win32SerialLineTransport.cpp)
    target_include_directories(win32_serial_transport_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(win32_serial_transport_tests PRIVATE railflow_calibration Threads::Threads)
    railflow_enable_warnings(win32_serial_transport_tests)
    add_test(NAME Win32SerialTransportTests COMMAND win32_serial_transport_tests)
    set_tests_properties(Win32SerialTransportTests PROPERTIES TIMEOUT 10)

    add_executable(calibration_tests tests/CalibrationTests.cpp)
    target_link_libraries(calibration_tests PRIVATE railflow_calibration)
    railflow_enable_warnings(calibration_tests)
    add_test(NAME CalibrationTests COMMAND calibration_tests)
    set_tests_properties(CalibrationTests PROPERTIES TIMEOUT 10)

    add_executable(serial_monitor_tests tests/SerialMonitorTests.cpp)
    target_link_libraries(serial_monitor_tests PRIVATE railflow_serial)
    railflow_enable_warnings(serial_monitor_tests)
    add_test(NAME SerialMonitorTests COMMAND serial_monitor_tests)

    add_executable(
        zone_manager_tests
        tests/ZoneManagerTests.cpp
    )

    target_link_libraries(
        zone_manager_tests
        PRIVATE
        railflow_core
    )

    railflow_enable_warnings(zone_manager_tests)

    add_test(
        NAME ZoneManagerTests
        COMMAND zone_manager_tests
    )


    add_executable(
        platform_crowd_estimator_tests
        tests/PlatformCrowdEstimatorTests.cpp
    )

    target_link_libraries(
        platform_crowd_estimator_tests
        PRIVATE
        railflow_core
    )

    railflow_enable_warnings(platform_crowd_estimator_tests)

    add_test(
        NAME PlatformCrowdEstimatorTests
        COMMAND platform_crowd_estimator_tests
    )


    add_executable(
        mock_input_tests
        tests/MockInputTests.cpp
    )

    target_link_libraries(
        mock_input_tests
        PRIVATE
        railflow_core
    )

    railflow_enable_warnings(mock_input_tests)

    add_test(
        NAME MockInputTests
        COMMAND mock_input_tests
    )

    add_executable(
        vehicle_load_estimator_tests
        tests/VehicleLoadEstimatorTests.cpp
    )
    target_link_libraries(vehicle_load_estimator_tests PRIVATE railflow_core)
    railflow_enable_warnings(vehicle_load_estimator_tests)
    add_test(
        NAME VehicleLoadEstimatorTests
        COMMAND vehicle_load_estimator_tests
    )

    add_executable(
        guidance_engine_tests
        tests/GuidanceEngineTests.cpp
    )
    target_link_libraries(guidance_engine_tests PRIVATE railflow_core)
    railflow_enable_warnings(guidance_engine_tests)
    add_test(
        NAME GuidanceEngineTests
        COMMAND guidance_engine_tests
    )
    add_executable(zone_config_tests tests/ZoneConfigTests.cpp)
    target_link_libraries(zone_config_tests PRIVATE railflow_core)
    railflow_enable_warnings(zone_config_tests)
    add_test(NAME ZoneConfigTests COMMAND zone_config_tests)
    add_executable(input_source_tests tests/InputSourceTests.cpp)
    target_link_libraries(input_source_tests PRIVATE railflow_core)
    railflow_enable_warnings(input_source_tests)
    add_test(NAME InputSourceTests COMMAND input_source_tests)
    add_executable(railflow_engine_tests tests/RailFlowEngineTests.cpp)
    target_link_libraries(railflow_engine_tests PRIVATE railflow_core)
    railflow_enable_warnings(railflow_engine_tests)
    add_test(NAME RailFlowEngineTests COMMAND railflow_engine_tests)
    add_executable(update_loop_tests tests/UpdateLoopTests.cpp)
    target_link_libraries(update_loop_tests PRIVATE railflow_core)
    railflow_enable_warnings(update_loop_tests)
    add_test(NAME UpdateLoopTests COMMAND update_loop_tests)
    add_executable(train_layout_tests tests/TrainLayoutTests.cpp)
    target_link_libraries(train_layout_tests PRIVATE railflow_core railflow_serial)
    railflow_enable_warnings(train_layout_tests)
    add_test(NAME TrainLayoutTests COMMAND train_layout_tests)
    add_test(NAME LayoutSmokeTest COMMAND railflow --layout)

    add_executable(runtime_options_tests tests/RuntimeOptionsTests.cpp)
    target_link_libraries(runtime_options_tests PRIVATE railflow_core)
    railflow_enable_warnings(runtime_options_tests)
    add_test(NAME RuntimeOptionsTests COMMAND runtime_options_tests)
    add_test(NAME DemoSmokeTest COMMAND railflow --demo --frames 3 --period-ms 1 --no-log)
    set_tests_properties(DemoSmokeTest PROPERTIES TIMEOUT 5)
    add_executable(coordinate_mapper_tests tests/CoordinateMapperTests.cpp)
    target_link_libraries(coordinate_mapper_tests PRIVATE railflow_core)
    railflow_enable_warnings(coordinate_mapper_tests)
    add_test(NAME CoordinateMapperTests COMMAND coordinate_mapper_tests)

    add_executable(serial_frame_parser_tests tests/SerialFrameParserTests.cpp)
    target_link_libraries(serial_frame_parser_tests PRIVATE railflow_serial)
    railflow_enable_warnings(serial_frame_parser_tests)
    add_test(NAME SerialFrameParserTests COMMAND serial_frame_parser_tests)

    add_executable(serial_frame_tracker_tests tests/SerialFrameTrackerTests.cpp)
    target_link_libraries(serial_frame_tracker_tests PRIVATE railflow_serial)
    railflow_enable_warnings(serial_frame_tracker_tests)
    add_test(NAME SerialFrameTrackerTests COMMAND serial_frame_tracker_tests)

    add_executable(serial_transport_tests tests/SerialTransportTests.cpp)
    target_link_libraries(serial_transport_tests PRIVATE railflow_serial)
    railflow_enable_warnings(serial_transport_tests)
    add_test(NAME SerialTransportTests COMMAND serial_transport_tests)
endif()
