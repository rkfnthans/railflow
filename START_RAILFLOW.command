#!/bin/bash
set -o pipefail
root="$(cd "$(dirname "$0")" && pwd)"
launcher="$root/scripts/launch-posix.sh"

if [ "$#" -gt 0 ]; then
    exec /bin/bash "$launcher" railflow "$@"
fi

run_program() {
    mkdir -p "$root/logs"
    local report
    report="$(mktemp "$root/logs/run-$(date +%Y%m%d-%H%M%S)-XXXXXX")"
    /bin/bash "$launcher" "$@" 2>&1 | tee "$report"
    local result=${PIPESTATUS[0]}
    echo "EXIT=$result" | tee -a "$report"
    echo "Log: $report"
    return "$result"
}

serial_port() {
    local device
    echo "USB serial ports:"
    for device in /dev/cu.usb* /dev/cu.wch*; do
        if [ -e "$device" ]; then echo "  $device"; fi
    done
    read -r -p "Serial port (empty cancels): " port
    [ -n "$port" ]
}

while true; do
    printf '\nRailFlow\n1. Mock demo (3 frames and CSV)\n2. Train layout\n3. Camera preview\n4. Find cameras\n5. Test display\n6. Serial raw ADC monitor\n7. Calibration recorder\n0. Exit\n'
    read -r -p "Select: " selection || exit 0
    case "$selection" in
        1) run_program railflow --demo --frames 3 --period-ms 100 ;;
        2) run_program railflow --layout ;;
        3)
            read -r -p "Camera index [0]: " camera
            run_program railflow_camera_preview --camera "${camera:-0}"
            ;;
        4) run_program railflow_camera_preview --probe ;;
        5) run_program railflow_camera_preview --test-pattern --frames 150 ;;
        6)
            if serial_port; then
                run_program railflow_serial_monitor "$port" --baud 115200 --record data/raw_adc.csv
            fi
            ;;
        7)
            if ! serial_port; then continue; fi
            read -r -p "Vehicle (CAR1/CAR2/CAR3): " vehicle
            read -r -p "Measured reference weight in grams: " weight
            read -r -p "Position (CENTER/LEFT/RIGHT/FRONT/REAR): " position
            read -r -p "Trial number: " trial
            if [ -z "$vehicle" ] || [ -z "$weight" ] || [ -z "$position" ] || [ -z "$trial" ]; then
                echo "All calibration labels are required."
                continue
            fi
            mkdir -p "$root/data/calibration"
            session="$(mktemp -d "$root/data/calibration/session-$(date +%Y%m%d-%H%M%S)-XXXXXX")"
            dataset="$session/adc.csv"
            run_program railflow_calibration_record "$port" --vehicle "$vehicle" --weight-g "$weight" \
                --position "$position" --trial "$trial" --output "$dataset"
            ;;
        0) exit 0 ;;
        *) echo "Choose 0 through 7." ;;
    esac
done
