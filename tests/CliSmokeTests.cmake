foreach(required RAILFLOW_EXECUTABLE RAILFLOW_MONITOR RAILFLOW_CALIBRATION RAILFLOW_TEST_ROOT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing CLI test setting ${required}")
    endif()
endforeach()

# Keep CSV writes in a new test directory, including on repeated runs.
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef run_id)
set(work_dir "${RAILFLOW_TEST_ROOT}/${run_id}")
file(MAKE_DIRECTORY "${work_dir}/data/logs")

function(run_cli executable)
    execute_process(
        COMMAND ${RAILFLOW_RUNNER} "${executable}" ${ARGN}
        WORKING_DIRECTORY "${work_dir}"
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE output
        ERROR_VARIABLE errors
        TIMEOUT 10
    )
    if(NOT "${exit_code}" STREQUAL "0")
        message(FATAL_ERROR "CLI failed (${exit_code}): ${executable} ${ARGN}\n${output}\n${errors}")
    endif()
    set(cli_output "${output}" PARENT_SCOPE)
endfunction()

run_cli("${RAILFLOW_EXECUTABLE}" --demo --frames 3 --period-ms 1 --no-log)
string(REGEX MATCHALL "P3" recommendations "${cli_output}")
list(LENGTH recommendations recommendation_count)
# Each frame mentions P3 in classification, platform state, prediction and recommendation.
if(recommendation_count LESS 3 OR EXISTS "${work_dir}/data/logs/railflow_log.csv")
    message(FATAL_ERROR "Bounded demo did not produce P3 output or --no-log wrote a CSV")
endif()

run_cli("${RAILFLOW_EXECUTABLE}" --demo --frames 3 --period-ms 1)
file(STRINGS "${work_dir}/data/logs/railflow_log.csv" rows)
list(LENGTH rows row_count)
if(NOT row_count EQUAL 4)
    message(FATAL_ERROR "Expected a header and exactly three CSV rows, received ${row_count}")
endif()
list(REMOVE_AT rows 0)
foreach(row IN LISTS rows)
    if(NOT row MATCHES ",P3$")
        message(FATAL_ERROR "Expected P3 recommendation in CSV row: ${row}")
    endif()
endforeach()

run_cli("${RAILFLOW_MONITOR}" --help)
if(NOT cli_output MATCHES "Usage: railflow_serial_monitor")
    message(FATAL_ERROR "Serial monitor help was not displayed")
endif()
run_cli("${RAILFLOW_CALIBRATION}" --help)
if(NOT cli_output MATCHES "Usage: railflow_calibration_record")
    message(FATAL_ERROR "Calibration recorder help was not displayed")
endif()

execute_process(
    COMMAND ${RAILFLOW_RUNNER} "${RAILFLOW_EXECUTABLE}" --unknown
    RESULT_VARIABLE exit_code
    ERROR_VARIABLE errors
    TIMEOUT 10
)
if(NOT "${exit_code}" STREQUAL "1" OR NOT errors MATCHES "2d 2d 75 6e 6b 6e 6f 77 6e")
    message(FATAL_ERROR "Rejected option must report exit 1 and received argument bytes: ${errors}")
endif()

file(REMOVE_RECURSE "${work_dir}")
