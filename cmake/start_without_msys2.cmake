cmake_minimum_required(VERSION 3.19)

foreach(variable PROGRAM CONFIG)
  if(NOT DEFINED ${variable})
    message(FATAL_ERROR "start_without_msys2.cmake: ${variable} is not set")
  endif()
endforeach()

if("$ENV{SystemRoot}" STREQUAL "")
  message(FATAL_ERROR "SystemRoot is not set")
endif()
file(TO_NATIVE_PATH "$ENV{SystemRoot}\\System32" system_folder)
file(TO_NATIVE_PATH "$ENV{SystemRoot}" windows_folder)
set(ENV{PATH} "${system_folder};${windows_folder}")
set(ENV{SDL_VIDEODRIVER} "dummy")

execute_process(COMMAND "${PROGRAM}" --smoke-test --config "${CONFIG}"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error_output)
message("${output}${error_output}")
if(NOT result EQUAL 0)
  message(FATAL_ERROR "${PROGRAM} did not start with only the Windows folders on the PATH (exit ${result})")
endif()
