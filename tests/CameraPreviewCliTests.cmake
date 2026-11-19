execute_process(
    COMMAND ${RAILFLOW_RUNNER} "${RAILFLOW_PREVIEW}" --help
    RESULT_VARIABLE exit_code
    OUTPUT_VARIABLE output
    ERROR_VARIABLE errors
    TIMEOUT 10
)
if(NOT "${exit_code}" STREQUAL "0" OR NOT output MATCHES "Usage: railflow_camera_preview")
    message(FATAL_ERROR "Preview help failed: ${exit_code}\n${output}\n${errors}")
endif()

execute_process(
    COMMAND ${RAILFLOW_RUNNER} "${RAILFLOW_PREVIEW}" --test-pattern --frames 3 --no-preview
    RESULT_VARIABLE exit_code
    OUTPUT_VARIABLE output
    ERROR_VARIABLE errors
    TIMEOUT 10
)
if(NOT "${exit_code}" STREQUAL "0" OR NOT output MATCHES "source=TEST_PATTERN" OR NOT output MATCHES "frames=3 ")
    message(FATAL_ERROR "Bounded test pattern failed: ${exit_code}\n${output}\n${errors}")
endif()

execute_process(
    COMMAND ${RAILFLOW_RUNNER} "${RAILFLOW_PREVIEW}" --camera -1
    RESULT_VARIABLE exit_code
    ERROR_VARIABLE errors
    TIMEOUT 10
)
if(NOT "${exit_code}" STREQUAL "1" OR NOT errors MATCHES "allowed range")
    message(FATAL_ERROR "Invalid camera index must fail before opening hardware: ${exit_code}\n${errors}")
endif()
