#!/bin/bash
root="$(cd "$(dirname "$0")" && pwd)"
/bin/bash "$root/scripts/macos-check.sh"
result=$?
if [ -t 0 ]; then read -r -p "Press Return to close: " answer; fi
exit "$result"
