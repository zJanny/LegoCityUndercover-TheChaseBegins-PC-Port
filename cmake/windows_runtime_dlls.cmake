function(chase_collect_binary_targets directory out)
  get_property(targets DIRECTORY ${directory} PROPERTY BUILDSYSTEM_TARGETS)
  set(found)
  foreach(target IN LISTS targets)
    get_target_property(type ${target} TYPE)
    if(type STREQUAL "EXECUTABLE" OR type STREQUAL "SHARED_LIBRARY" OR type STREQUAL "MODULE_LIBRARY")
      list(APPEND found ${target})
    endif()
  endforeach()
  get_property(subdirectories DIRECTORY ${directory} PROPERTY SUBDIRECTORIES)
  foreach(subdirectory IN LISTS subdirectories)
    chase_collect_binary_targets(${subdirectory} below)
    list(APPEND found ${below})
  endforeach()
  set(${out} ${found} PARENT_SCOPE)
endfunction()

set(CHASE_RUNTIME_LOADED_DLLS "SDL3.dll" CACHE STRING "DLLs that a DLL loads at run time, which no import table shows: sdl2-compat loads SDL3.dll")

function(chase_copy_runtime_dlls directory)
  chase_collect_binary_targets(${directory} targets)
  get_filename_component(compiler_folder "${CMAKE_CXX_COMPILER}" DIRECTORY)
  set(program_list "${CMAKE_BINARY_DIR}/runtime-dlls-programs.txt")
  set(stamp "${CMAKE_BINARY_DIR}/runtime-dlls.stamp")
  set(depfile "${CMAKE_BINARY_DIR}/runtime-dlls.d")
  set(program_files)
  set(content)
  foreach(target IN LISTS targets)
    string(APPEND content "$<TARGET_FILE:${target}>\n")
    list(APPEND program_files "$<TARGET_FILE:${target}>")
  endforeach()
  file(GENERATE OUTPUT "${program_list}" CONTENT "${content}")
  add_custom_command(OUTPUT "${stamp}"
    COMMAND ${CMAKE_COMMAND}
      "-DPROGRAM_LIST=${program_list}" "-DSEARCH_FOLDERS=${compiler_folder}" "-DOBJDUMP=${CMAKE_OBJDUMP}"
      "-DRUNTIME_LOADED=${CHASE_RUNTIME_LOADED_DLLS}" "-DDESTINATION=${CMAKE_BINARY_DIR}" "-DSTAMP=${stamp}" "-DDEPFILE=${depfile}"
      -P "${CMAKE_SOURCE_DIR}/cmake/copy_runtime_dlls.cmake"
    DEPENDS ${program_files} "${CMAKE_SOURCE_DIR}/cmake/copy_runtime_dlls.cmake" "${CMAKE_SOURCE_DIR}/cmake/windows_dll_scan.cmake"
    DEPFILE "${depfile}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Copying the DLLs the programs need into the build folder"
    VERBATIM)
  add_custom_target(chase_runtime_dlls ALL DEPENDS "${stamp}")
  add_dependencies(chase_runtime_dlls ${targets})
endfunction()

function(chase_add_runtime_dll_tests)
  if(NOT BUILD_TESTING OR NOT TARGET chase-input-lab)
    return()
  endif()
  if(NOT WIN32)
    chase_platform_skip_test(runtime-dlls-present "the Linux programs find their libraries through the system; the DLL copy of cmake/windows_runtime_dlls.cmake is Windows only")
    return()
  endif()
  add_test(NAME runtime-dlls-present COMMAND ${CMAKE_COMMAND}
    "-DPROGRAM_LIST=${CMAKE_BINARY_DIR}/runtime-dlls-programs.txt" "-DOBJDUMP=${CMAKE_OBJDUMP}" "-DDESTINATION=${CMAKE_BINARY_DIR}"
    -P "${CMAKE_SOURCE_DIR}/cmake/check_runtime_dlls.cmake")
  set_tests_properties(runtime-dlls-present PROPERTIES LABELS slow PASS_REGULAR_EXPRESSION "every imported DLL is in the build folder")
  add_test(NAME runtime-dlls-start-without-msys2 COMMAND ${CMAKE_COMMAND}
    "-DPROGRAM=$<TARGET_FILE:chase-input-lab>" "-DCONFIG=${CMAKE_BINARY_DIR}/config/controls.cfg"
    -P "${CMAKE_SOURCE_DIR}/cmake/start_without_msys2.cmake")
  set_tests_properties(runtime-dlls-start-without-msys2 PROPERTIES PASS_REGULAR_EXPRESSION "SDL dummy input smoke test passed")
endfunction()

if(WIN32)
  if(CMAKE_VERSION VERSION_LESS 3.19)
    message(FATAL_ERROR "Building on Windows needs CMake 3.19 or newer")
  endif()
  cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL chase_copy_runtime_dlls ${CMAKE_SOURCE_DIR})
endif()
if(NOT CMAKE_VERSION VERSION_LESS 3.19)
  cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL chase_add_runtime_dll_tests)
endif()
