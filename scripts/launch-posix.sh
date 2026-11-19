#!/bin/bash
set -e

root="$(cd "$(dirname "$0")/.." && pwd)"
program="${1:-railflow}"
if [ "$#" -gt 0 ]; then shift; fi
case "$program" in
    railflow|railflow_serial_monitor|railflow_calibration_record|railflow_camera_preview|railflow_detect) ;;
    *) echo "Unknown RailFlow program: $program" >&2; exit 2 ;;
esac

system="$(uname -s)"
architecture="$(uname -m)"
case "$system" in
    Darwin) platform=macos; binary="$root/bin/macos/$program" ;;
    Linux) platform=linux; binary="$root/bin/linux/$program" ;;
    MINGW*|MSYS*|CYGWIN*)
        platform=windows; binary="$root/bin/windows-x64/$program.exe"
        if [ ! -f "$binary" ]; then binary="$root/$program.exe"; fi
        ;;
    *) echo "Unsupported operating system: $system" >&2; exit 1 ;;
esac

echo "RailFlow host: $system / $architecture"
if [ ! -x "$binary" ]; then
    if [ ! -f "$root/CMakeLists.txt" ]; then
        echo "Missing executable: $binary. Extract the entire runtime ZIP." >&2
        exit 1
    fi
    if ! command -v cmake >/dev/null 2>&1; then
        echo "Source build requires CMake and a C++20 compiler. The runtime ZIP includes ready-built programs." >&2
        exit 1
    fi
    build="$root/build/native/$platform-$architecture"
    opencv=OFF
    case "$program" in railflow_camera_preview|railflow_detect) opencv=ON ;; esac
    cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release \
        -DRAILFLOW_ENABLE_OPENCV="$opencv" -DRAILFLOW_ENABLE_CAMERA_PREVIEW=ON
    cmake --build "$build" --config Release --target "$program" --parallel
    binary="$build/$program"
    if [ "$platform" = windows ]; then
        binary="$build/Release/$program.exe"
        if [ ! -f "$binary" ]; then binary="$build/$program.exe"; fi
    fi
fi

work="${RAILFLOW_WORK_DIR:-$root}"
mkdir -p "$work/data/logs"
cd "$work"
exec "$binary" "$@"
