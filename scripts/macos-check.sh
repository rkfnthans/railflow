#!/bin/bash
set -o pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$root/system-checks"
check="$(mktemp -d "$root/system-checks/macos-$(date +%Y%m%d-%H%M%S)-XXXXXX")" || exit 1
report="$check/system-test.txt"
export RAILFLOW_WORK_DIR="$check"
failures=0
checks=0
{
    echo "RailFlow cross-platform check"
    uname -sm
    sw_vers
} > "$report"

run() {
    echo "COMMAND $*" | tee -a "$report"
    /bin/bash "$root/scripts/launch-posix.sh" "$@" 2>&1 | tee -a "$report"
    result=${PIPESTATUS[0]}
    echo "EXIT=$result" | tee -a "$report"
    checks=$((checks + 1))
    if [ "$result" -ne 0 ]; then failures=$((failures + 1)); fi
}

run railflow --layout
run railflow --demo --frames 3 --period-ms 100 --no-log
run railflow --demo --frames 3 --period-ms 100
run railflow_serial_monitor --help
run railflow_calibration_record --help
run railflow_camera_preview --test-pattern --frames 3 --no-preview
checks=$((checks + 1))
csv="$check/data/logs/railflow_log.csv"
if [ -f "$csv" ] && [ "$(wc -l < "$csv" | tr -d ' ')" = 4 ] \
    && [ "$(awk -F, 'NR>1 && $17=="P3" {count++} END {print count+0}' "$csv")" = 3 ]; then
    echo "PASS: CSV contains 3 P3 rows" | tee -a "$report"
else
    failures=$((failures + 1))
    echo "FAIL: Mock CSV validation" | tee -a "$report"
fi
echo "Checks=$checks Failures=$failures" | tee -a "$report"
echo "Results: $check"
exit "$failures"
