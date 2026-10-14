cmake_minimum_required(VERSION 3.16)
project(chase_input_foundation LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # build/compile_commands.json for clangd
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
add_library(chase_input src/input.cpp)
target_include_directories(chase_input PUBLIC src)
target_compile_options(chase_input PRIVATE -Wall -Wextra -Wpedantic)
add_executable(chase-input-lab src/main.cpp)
target_link_libraries(chase-input-lab PRIVATE chase_input PkgConfig::SDL2)
target_compile_options(chase-input-lab PRIVATE -Wall -Wextra -Wpedantic)
configure_file(config/controls.cfg config/controls.cfg COPYONLY)
# Native asset runtime (plan S4a): no SDL dependency.
add_library(chase_assets src/assets/fuse.cpp src/assets/pooled.cpp src/assets/texture.cpp src/assets/model.cpp src/assets/material.cpp src/assets/level.cpp src/assets/object_collision.cpp src/assets/audio.cpp src/assets/anim.cpp src/assets/nav.cpp src/assets/text.cpp src/assets/cutscene.cpp src/assets/table.cpp src/assets/config_text.cpp src/assets/xaml.cpp src/assets/subtitles.cpp src/assets/particles.cpp)
target_include_directories(chase_assets PUBLIC src)
target_compile_options(chase_assets PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(chase_assets PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_sources(chase_assets PRIVATE src/assets/object_classes.cpp src/assets/file_decoders.cpp)
# C ABI over chase_assets for the Python tools (tools/chase_native.py).
add_library(chase_assets_c SHARED src/assets/c_api.cpp)
target_link_libraries(chase_assets_c PRIVATE chase_assets chase_runtime)
target_compile_options(chase_assets_c PRIVATE -Wall -Wextra -Wpedantic)
# Audio mixer core (plan S4d): no SDL dependency; chase-soundtest is the SDL front end.
add_library(chase_audio src/audio/mixer.cpp src/audio/spatial.cpp src/audio/soundplayer.cpp src/audio/effectvoices.cpp src/audio/musicplayer.cpp)
target_link_libraries(chase_audio PUBLIC chase_assets)
target_compile_options(chase_audio PRIVATE -Wall -Wextra -Wpedantic)
# Save image (plan S4f): no SDL dependency.
add_library(chase_save src/save/save.cpp)
target_include_directories(chase_save PUBLIC src)
target_compile_options(chase_save PRIVATE -Wall -Wextra -Wpedantic)
add_library(chase_save_tables src/save/completion_tables.cpp)
target_link_libraries(chase_save_tables PUBLIC chase_save chase_assets)
target_compile_options(chase_save_tables PRIVATE -Wall -Wextra -Wpedantic)
add_library(chase_save_progress src/save/slot_progress.cpp src/save/continue_start.cpp)
target_link_libraries(chase_save_progress PUBLIC chase_save chase_assets)
target_compile_options(chase_save_progress PRIVATE -Wall -Wextra -Wpedantic)
# Game runtime core (plan S4e): recovered frame logic, no SDL dependency.
add_library(chase_runtime src/runtime/scheduler.cpp src/runtime/pad.cpp src/runtime/player_intent.cpp src/runtime/player_locomotion.cpp src/runtime/follow_camera.cpp src/runtime/collision.cpp src/runtime/camera_sweep.cpp src/runtime/player_vertical.cpp src/runtime/player_motion.cpp src/runtime/camera_records.cpp src/runtime/camera_rig.cpp src/runtime/camera_lock_icon.cpp)
target_link_libraries(chase_runtime PUBLIC chase_assets)
target_include_directories(chase_runtime PUBLIC src)
target_compile_options(chase_runtime PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
set_target_properties(chase_runtime PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_sources(chase_runtime PRIVATE src/runtime/studs.cpp src/runtime/sound_events.cpp src/runtime/music.cpp src/runtime/level_start.cpp src/runtime/script_runner.cpp src/runtime/natives_objects.cpp src/runtime/natives_progress.cpp src/runtime/door.cpp src/runtime/scene_change.cpp src/runtime/room_streaming.cpp src/runtime/room_streamer.cpp src/runtime/scene_start.cpp src/runtime/dcam.cpp src/runtime/debug_metrics.cpp src/runtime/object_visibility.cpp)
# Front end (boot to the slot list): CPU-drawn screens, no SDL dependency.
add_library(chase_frontend src/frontend/script.cpp src/frontend/frontend.cpp src/frontend/pages.cpp src/frontend/canvas.cpp src/frontend/resources.cpp src/frontend/layout.cpp src/frontend/flow.cpp src/frontend/hud.cpp src/frontend/minimap.cpp)
target_link_libraries(chase_frontend PUBLIC chase_assets chase_runtime)
target_compile_options(chase_frontend PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
add_executable(chase-hudshot src/hudshot.cpp)
target_link_libraries(chase-hudshot PRIVATE chase_frontend)
target_compile_options(chase-hudshot PRIVATE -Wall -Wextra -Wpedantic)
add_executable(chase-savecheck src/savecheck.cpp)
target_link_libraries(chase-savecheck PRIVATE chase_save_tables)
target_compile_options(chase-savecheck PRIVATE -Wall -Wextra -Wpedantic)
add_executable(chase-tabledump src/tabledump.cpp)
target_link_libraries(chase-tabledump PRIVATE chase_assets)
target_compile_options(chase-tabledump PRIVATE -Wall -Wextra -Wpedantic)
add_executable(chase-materials src/materials.cpp)
target_link_libraries(chase-materials PRIVATE chase_assets)
target_compile_options(chase-materials PRIVATE -Wall -Wextra -Wpedantic)
add_executable(chase-soundtest src/soundtest.cpp)
target_link_libraries(chase-soundtest PRIVATE chase_audio PkgConfig::SDL2)
target_compile_options(chase-soundtest PRIVATE -Wall -Wextra -Wpedantic)
# Movie decoding (FFmpeg) shared by chase-fmv and the front end: no SDL dependency.
pkg_check_modules(FFMPEG IMPORTED_TARGET libavformat libavcodec libswscale libswresample libavutil)
if(FFMPEG_FOUND)
  add_library(chase_movie src/movie/movie.cpp)
  target_include_directories(chase_movie PUBLIC src)
  target_link_libraries(chase_movie PUBLIC PkgConfig::FFMPEG)
  target_compile_options(chase_movie PRIVATE -Wall -Wextra -Wpedantic)
  add_library(chase_frontend_movie src/frontend/movie_host.cpp)
  target_link_libraries(chase_frontend_movie PUBLIC chase_frontend chase_movie)
  target_compile_options(chase_frontend_movie PRIVATE -Wall -Wextra -Wpedantic)
  add_executable(chase-frontshot src/frontshot.cpp)
  target_link_libraries(chase-frontshot PRIVATE chase_frontend_movie)
  target_compile_options(chase-frontshot PRIVATE -Wall -Wextra -Wpedantic)
  add_executable(chase-fmv src/fmv.cpp)
  target_link_libraries(chase-fmv PRIVATE chase_assets chase_movie PkgConfig::SDL2)
  target_compile_options(chase-fmv PRIVATE -Wall -Wextra -Wpedantic)
endif()
# Development model viewer (plan S4b); built when libepoxy is available.
pkg_check_modules(EPOXY IMPORTED_TARGET epoxy)
if(EPOXY_FOUND)
  set(CHASE_SHADER_DIR ${CMAKE_SOURCE_DIR}/src/render/shaders)
  set(CHASE_SHADER_SOURCES sceneVertex:scene.vert.glsl sceneFragment:scene.frag.glsl hudVertex:hud.vert.glsl hudFragment:hud.frag.glsl)
  set(CHASE_SHADERS_CPP ${CMAKE_BINARY_DIR}/generated/shaders.cpp)
  add_custom_command(OUTPUT ${CHASE_SHADERS_CPP}
    COMMAND ${CMAKE_COMMAND} "-DSHADERS=${CHASE_SHADER_SOURCES}" -DSHADER_DIR=${CHASE_SHADER_DIR} -DOUTPUT=${CHASE_SHADERS_CPP}
      -P ${CMAKE_SOURCE_DIR}/tools/embed_shaders.cmake
    DEPENDS ${CHASE_SHADER_DIR}/scene.vert.glsl ${CHASE_SHADER_DIR}/scene.frag.glsl ${CHASE_SHADER_DIR}/hud.vert.glsl ${CHASE_SHADER_DIR}/hud.frag.glsl
      ${CMAKE_SOURCE_DIR}/tools/embed_shaders.cmake
    VERBATIM)
  add_library(chase_render src/render/scene.cpp src/render/character.cpp ${CHASE_SHADERS_CPP})
  target_include_directories(chase_render PUBLIC ${CMAKE_SOURCE_DIR}/src)
  target_link_libraries(chase_render PUBLIC chase_assets PkgConfig::SDL2 PkgConfig::EPOXY)
  target_compile_options(chase_render PRIVATE -Wall -Wextra -Wpedantic)
  target_sources(chase_render PRIVATE src/render/studs.cpp src/render/debug_text.cpp src/render/debug_overlay.cpp
    src/render/program.cpp src/render/window.cpp src/render/hud_overlay.cpp src/render/front_presenter.cpp)
  add_executable(chase-viewer src/viewer.cpp)
  target_link_libraries(chase-viewer PRIVATE chase_render chase_audio chase_input)
  target_compile_options(chase-viewer PRIVATE -Wall -Wextra -Wpedantic)
  if(FFMPEG_FOUND)
    add_executable(chase-game src/game.cpp src/game_options.cpp src/game_player.cpp src/game_doors.cpp src/game_level.cpp src/game_level_update.cpp src/game_level_rooms.cpp)
    target_link_libraries(chase-game PRIVATE chase_render chase_runtime chase_input chase_audio chase_frontend_movie chase_save_progress chase_save_tables)
    target_compile_options(chase-game PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()
set(CHASE_ROMFS_DIR "${CMAKE_SOURCE_DIR}/artifacts/romfs/romfs" CACHE PATH
    "Extracted RomFS directory for the opt-in real-data asset test")
set(CHASE_LOCO_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/loco-out" CACHE PATH
    "Recorded original-game locomotion traces for the opt-in differential test")
set(CHASE_COLLISION_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/coll-out" CACHE PATH
    "Recorded original-game collision traces for the opt-in differential test")
set(CHASE_OBJECT_COLLISION_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/objcoll-out" CACHE PATH
    "Recorded original-game traces of the player against placed objects for the opt-in differential test")
set(CHASE_FALL_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/fall-out" CACHE PATH
    "Recorded original-game fall and landing traces for the opt-in differential test")
set(CHASE_JUMP_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/jump-traces" CACHE PATH
    "Recorded original-game jump traces for the opt-in differential test")
set(CHASE_SLIDE_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/slide-traces" CACHE PATH
    "Recorded original-game slide (state 8) traces for the opt-in differential test")
set(CHASE_SLOPE_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/slope-traces" CACHE PATH
    "Recorded original-game run-on-slope traces for the opt-in differential test")
set(CHASE_CAMERA_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/cam-out" CACHE PATH
    "Recorded original-game camera traces for the opt-in differential test")
set(CHASE_CAMERA_BLEND_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/camera-blend-traces" CACHE PATH
    "Recorded original-game camera traces of walks between camera records (exported CSV) for the opt-in blend test")
set(CHASE_CAMERA_WALL_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/camwall-out" CACHE PATH
    "Recorded original-game camera traces near walls (.mem memtraces and exported CSV) for the opt-in differential tests")
set(CHASE_STUDS_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/studs-traces" CACHE PATH
    "Recorded original-game stud pickup traces for the opt-in differential test")
set(CHASE_HUD_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/hud-traces" CACHE PATH
    "Recorded original-game HUD stud bar traces for the opt-in HUD test")
set(CHASE_AUDIO_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/audio-traces" CACHE PATH
    "Recorded original-game sound-call traces (CSV from tools/export_sound_trace.py) for the opt-in differential test")
include(CTest)
if(BUILD_TESTING)
  add_executable(input-test tests/input_test.cpp)
  target_link_libraries(input-test PRIVATE chase_input)
  target_compile_options(input-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME input-core COMMAND input-test)
  add_test(NAME input-sdl-smoke COMMAND chase-input-lab --smoke-test)
  set_tests_properties(input-sdl-smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy")
  add_executable(assets-test tests/assets_test.cpp)
  target_link_libraries(assets-test PRIVATE chase_assets)
  target_compile_options(assets-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME assets-core COMMAND assets-test)
  add_executable(frontend-test tests/frontend_test.cpp)
  target_link_libraries(frontend-test PRIVATE chase_frontend)
  target_compile_options(frontend-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME frontend-flow COMMAND frontend-test)
  if(TARGET chase_frontend_movie)
    add_executable(frontend-real-test tests/frontend_real_test.cpp)
    target_link_libraries(frontend-real-test PRIVATE chase_frontend_movie)
    target_compile_options(frontend-real-test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME frontend-real-data COMMAND frontend-real-test
      "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_ROMFS_DIR}/cutscenes")
    set_tests_properties(frontend-real-data PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(hud-real-test tests/hud_real_test.cpp)
    target_link_libraries(hud-real-test PRIVATE chase_frontend)
    target_compile_options(hud-real-test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME hud-real-data COMMAND hud-real-test
      "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_HUD_TRACE_DIR}")
    set_tests_properties(hud-real-data PROPERTIES SKIP_RETURN_CODE 77)
  endif()
  add_executable(mixer-test tests/mixer_test.cpp)
  target_link_libraries(mixer-test PRIVATE chase_audio)
  target_compile_options(mixer-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME audio-mixer COMMAND mixer-test)
  add_executable(audio-test tests/audio_test.cpp)
  target_link_libraries(audio-test PRIVATE chase_audio chase_runtime)
  target_compile_options(audio-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME audio-core COMMAND audio-test)
  if(TARGET chase-game)
    add_executable(audio-real-test tests/audio_real_test.cpp)
    target_link_libraries(audio-real-test PRIVATE chase_audio chase_runtime chase_assets)
    target_compile_options(audio-real-test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME audio-real-data COMMAND audio-real-test
      "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_AUDIO_TRACE_DIR}"
      "$<TARGET_FILE:chase-game>" "${CMAKE_BINARY_DIR}/config/controls.cfg")
    set_tests_properties(audio-real-data PROPERTIES SKIP_RETURN_CODE 77)
  endif()
  add_executable(runtime-test tests/runtime_test.cpp)
  target_link_libraries(runtime-test PRIVATE chase_runtime)
  target_compile_options(runtime-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME runtime-core COMMAND runtime-test)
  add_executable(script-runner-test tests/script_runner_test.cpp)
  target_link_libraries(script-runner-test PRIVATE chase_runtime)
  target_compile_options(script-runner-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME script-runner COMMAND script-runner-test)
  add_executable(natives-objects-test tests/natives_objects_test.cpp)
  target_link_libraries(natives-objects-test PRIVATE chase_runtime)
  target_compile_options(natives-objects-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-objects COMMAND natives-objects-test)
  add_executable(natives-progress-test tests/natives_progress_test.cpp)
  target_link_libraries(natives-progress-test PRIVATE chase_runtime)
  target_compile_options(natives-progress-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-progress COMMAND natives-progress-test)
  add_executable(scene-start-test tests/scene_start_test.cpp)
  target_link_libraries(scene-start-test PRIVATE chase_runtime)
  target_compile_options(scene-start-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scene-start COMMAND scene-start-test)
  add_executable(dcam-test tests/dcam_test.cpp)
  target_link_libraries(dcam-test PRIVATE chase_runtime)
  target_compile_options(dcam-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME dcam COMMAND dcam-test)
  add_executable(progress-tables-test tests/progress_tables_test.cpp)
  target_link_libraries(progress-tables-test PRIVATE chase_runtime)
  target_compile_options(progress-tables-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME progress-tables COMMAND progress-tables-test)
  add_test(NAME progress-readme COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/progress_report.py" --check "${CMAKE_SOURCE_DIR}/README.md")
  set_tests_properties(progress-readme PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "CHASE_ASSETS_LIB=$<TARGET_FILE:chase_assets_c>")
  add_test(NAME no-inline-shaders COMMAND ${CMAKE_COMMAND} -DSRC_DIR=${CMAKE_SOURCE_DIR}/src -P ${CMAKE_SOURCE_DIR}/tools/check_no_inline_shaders.cmake)
  add_test(NAME gl-only-in-render COMMAND ${CMAKE_COMMAND} -DSRC_DIR=${CMAKE_SOURCE_DIR}/src -P ${CMAKE_SOURCE_DIR}/tools/check_gl_only_in_render.cmake)
  add_test(NAME gl-only-in-render-rejects COMMAND ${CMAKE_COMMAND} -DSRC_DIR=${CMAKE_SOURCE_DIR}/tests/gl_only_in_render/src -P ${CMAKE_SOURCE_DIR}/tools/check_gl_only_in_render.cmake)
  set_tests_properties(gl-only-in-render-rejects PROPERTIES WILL_FAIL TRUE)
  add_test(NAME game-cpp-wiring COMMAND ${CMAKE_COMMAND} -DGAME_CPP=${CMAKE_SOURCE_DIR}/src/game.cpp -P ${CMAKE_SOURCE_DIR}/tools/check_game_cpp_wiring.cmake)
  add_test(NAME doc-links COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_doc_links.py" --root "${CMAKE_SOURCE_DIR}")
  add_executable(door-test tests/door_test.cpp)
  target_link_libraries(door-test PRIVATE chase_runtime)
  target_compile_options(door-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME door-core COMMAND door-test)
  add_executable(camera-sweep-test tests/camera_sweep_test.cpp)
  target_link_libraries(camera-sweep-test PRIVATE chase_runtime)
  target_compile_options(camera-sweep-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-sweep COMMAND camera-sweep-test)
  add_executable(camera-records-test tests/camera_records_test.cpp)
  target_link_libraries(camera-records-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-records-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-records COMMAND camera-records-test)
  add_executable(camera-records-real-test tests/camera_records_real_test.cpp)
  target_link_libraries(camera-records-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-records-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-records-real-data COMMAND camera-records-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(camera-records-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(locomotion-real-test tests/locomotion_real_test.cpp)
  target_link_libraries(locomotion-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(locomotion-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME locomotion-real-data COMMAND locomotion-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_LOCO_TRACE_DIR}")
  set_tests_properties(locomotion-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(streaming-real-test tests/streaming_real_test.cpp)
  target_link_libraries(streaming-real-test PRIVATE chase_assets)
  target_compile_options(streaming-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME streaming-real-data COMMAND streaming-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(streaming-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(room-streaming-real-test tests/room_streaming_real_test.cpp)
  target_link_libraries(room-streaming-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(room-streaming-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME room-streaming-real-data COMMAND room-streaming-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(room-streaming-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(collision-real-test tests/collision_real_test.cpp)
  target_link_libraries(collision-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(collision-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME collision-real-data COMMAND collision-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_LOCO_TRACE_DIR}" "${CHASE_COLLISION_TRACE_DIR}"
    "${CHASE_OBJECT_COLLISION_TRACE_DIR}")
  set_tests_properties(collision-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(level-start-real-test tests/level_start_real_test.cpp)
  target_link_libraries(level-start-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(level-start-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME level-start-real-data COMMAND level-start-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(level-start-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(dcam-real-test tests/dcam_real_test.cpp)
  target_link_libraries(dcam-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(dcam-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME dcam-real-data COMMAND dcam-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(dcam-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(script-dialog-real-test tests/script_dialog_real_test.cpp)
  target_link_libraries(script-dialog-real-test PRIVATE chase_runtime chase_frontend chase_assets)
  target_compile_options(script-dialog-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME script-dialog-real-data COMMAND script-dialog-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(script-dialog-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(natives-objects-real-test tests/natives_objects_real_test.cpp)
  target_link_libraries(natives-objects-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(natives-objects-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-objects-real-data COMMAND natives-objects-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(natives-objects-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(natives-progress-real-test tests/natives_progress_real_test.cpp)
  target_link_libraries(natives-progress-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(natives-progress-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-progress-real-data COMMAND natives-progress-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(natives-progress-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(scene-change-real-test tests/scene_change_real_test.cpp)
  target_link_libraries(scene-change-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(scene-change-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scene-change-real-data COMMAND scene-change-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(scene-change-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(door-real-test tests/door_real_test.cpp)
  target_link_libraries(door-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(door-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME door-real-data COMMAND door-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(door-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(fall-real-test tests/fall_real_test.cpp)
  target_link_libraries(fall-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(fall-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME fall-real-data COMMAND fall-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_FALL_TRACE_DIR}")
  set_tests_properties(fall-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(jump-real-test tests/jump_real_test.cpp)
  target_link_libraries(jump-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(jump-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME jump-real-data COMMAND jump-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_JUMP_TRACE_DIR}")
  set_tests_properties(jump-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(slide-real-test tests/slide_real_test.cpp)
  target_link_libraries(slide-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(slide-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slide-real-data COMMAND slide-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_SLIDE_TRACE_DIR}")
  set_tests_properties(slide-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(slope-real-test tests/slope_real_test.cpp)
  target_link_libraries(slope-real-test PRIVATE chase_runtime)
  target_compile_options(slope-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slope-real-data COMMAND slope-real-test "${CHASE_SLOPE_TRACE_DIR}")
  set_tests_properties(slope-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(camera-real-test tests/camera_real_test.cpp)
  target_link_libraries(camera-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-real-data COMMAND camera-real-test "${CHASE_CAMERA_TRACE_DIR}"
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_CAMERA_WALL_TRACE_DIR}")
  set_tests_properties(camera-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(camera-blend-real-test tests/camera_blend_real_test.cpp)
  target_link_libraries(camera-blend-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-blend-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-blend-real-data COMMAND camera-blend-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib"
    "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_CAMERA_BLEND_TRACE_DIR}")
  set_tests_properties(camera-blend-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(camera-sweep-real-test tests/camera_sweep_real_test.cpp)
  target_link_libraries(camera-sweep-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-sweep-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-sweep-real-data COMMAND camera-sweep-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_CAMERA_WALL_TRACE_DIR}")
  set_tests_properties(camera-sweep-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(save-test tests/save_test.cpp)
  target_link_libraries(save-test PRIVATE chase_save)
  target_compile_options(save-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME save-core COMMAND save-test)
  add_executable(save-real-test tests/save_real_test.cpp)
  target_link_libraries(save-real-test PRIVATE chase_save_tables)
  target_compile_options(save-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME save-real-data COMMAND save-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(save-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(slot-progress-test tests/slot_progress_test.cpp)
  target_link_libraries(slot-progress-test PRIVATE chase_save_progress)
  target_compile_options(slot-progress-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slot-progress COMMAND slot-progress-test)
  set(CHASE_REAL_SAVE "${CMAKE_SOURCE_DIR}/artifacts/oracle/agentJ-saves/savegame.dat" CACHE FILEPATH
    "A real savegame.dat (a copy of the user's save) for slot-progress-real-data; the test skips when it is missing")
  add_executable(slot-progress-real-test tests/slot_progress_real_test.cpp)
  target_link_libraries(slot-progress-real-test PRIVATE chase_save_progress chase_save_tables chase_runtime)
  target_compile_options(slot-progress-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slot-progress-real-data COMMAND slot-progress-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_REAL_SAVE}")
  set_tests_properties(slot-progress-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(assets-real-test tests/assets_real_test.cpp)
  target_link_libraries(assets-real-test PRIVATE chase_assets)
  target_compile_options(assets-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME assets-real-data COMMAND assets-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(assets-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(studs-test tests/studs_test.cpp)
  target_link_libraries(studs-test PRIVATE chase_runtime)
  target_compile_options(studs-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME studs-core COMMAND studs-test)
  add_executable(studs-real-test tests/studs_real_test.cpp)
  target_link_libraries(studs-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(studs-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME studs-real-data COMMAND studs-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_STUDS_TRACE_DIR}")
  set_tests_properties(studs-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(debug-metrics-test tests/debug_metrics_test.cpp)
  target_link_libraries(debug-metrics-test PRIVATE chase_runtime)
  target_compile_options(debug-metrics-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME debug-metrics-core COMMAND debug-metrics-test)
  if(TARGET chase_render)
    add_executable(debug-text-test tests/debug_text_test.cpp)
    target_link_libraries(debug-text-test PRIVATE chase_render)
    target_compile_options(debug-text-test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME debug-text-core COMMAND debug-text-test)
  endif()
  add_executable(game-options-test tests/game_options_test.cpp src/game_options.cpp)
  target_link_libraries(game-options-test PRIVATE chase_frontend chase_runtime)
  target_compile_options(game-options-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME game-options-core COMMAND game-options-test)
  if(TARGET chase-game)
    # Rejected command lines stop before any data is read: the error names the option and the bad text.
    add_test(NAME game-option-slot-range COMMAND chase-game --slot 2)
    set_tests_properties(game-option-slot-range PROPERTIES PASS_REGULAR_EXPRESSION "chase-game: --slot: out of range, allowed 0 to 1: '2'")
    add_test(NAME game-option-number-trailing COMMAND chase-game --sound-volume 5x)
    set_tests_properties(game-option-number-trailing PROPERTIES PASS_REGULAR_EXPRESSION "chase-game: --sound-volume: not a whole number: '5x'")
    add_test(NAME game-option-start-malformed COMMAND chase-game --start "1 2")
    set_tests_properties(game-option-start-malformed PROPERTIES PASS_REGULAR_EXPRESSION "chase-game: --start: expected X Y Z: '1 2'")
    add_test(NAME game-option-script-step COMMAND chase-game --script "0 150 40 AQ")
    set_tests_properties(game-option-script-step PROPERTIES PASS_REGULAR_EXPRESSION "chase-game: --script: unknown button: 'Q'")
    add_test(NAME game-option-front-script-step COMMAND chase-game --front-script "run 1;press")
    set_tests_properties(game-option-front-script-step PROPERTIES PASS_REGULAR_EXPRESSION "chase-game: --front-script: wrong number of fields: 'press'")
    add_test(NAME game-option-debug-overlay-value COMMAND chase-game --debug-overlay on)
    set_tests_properties(game-option-debug-overlay-value PROPERTIES PASS_REGULAR_EXPRESSION "chase-game: on: unknown option: 'on'")
    add_test(NAME game-option-unknown COMMAND chase-game --bogus)
    set_tests_properties(game-option-unknown PROPERTIES PASS_REGULAR_EXPRESSION "chase-game: --bogus: unknown option: '--bogus'")
  endif()
  if(TARGET chase-viewer)
    add_test(NAME viewer-option-number COMMAND chase-viewer --frame 1x)
    set_tests_properties(viewer-option-number PROPERTIES PASS_REGULAR_EXPRESSION "chase-viewer: --frame: not a number: '1x'")
    add_test(NAME viewer-option-cull COMMAND chase-viewer --cull sideways)
    set_tests_properties(viewer-option-cull PROPERTIES PASS_REGULAR_EXPRESSION "chase-viewer: --cull: expected none, cw or ccw: 'sideways'")
    add_test(NAME viewer-option-fps COMMAND chase-viewer --fps 0)
    set_tests_properties(viewer-option-fps PROPERTIES PASS_REGULAR_EXPRESSION "chase-viewer: --fps: expected above 0: '0'")
    add_test(NAME viewer-option-eye COMMAND chase-viewer --eye 1,2)
    set_tests_properties(viewer-option-eye PROPERTIES PASS_REGULAR_EXPRESSION "chase-viewer: --eye: expected X,Y,Z: '1,2'")
    add_test(NAME viewer-option-unknown COMMAND chase-viewer --bogus)
    set_tests_properties(viewer-option-unknown PROPERTIES PASS_REGULAR_EXPRESSION "chase-viewer: --bogus: unknown option: '--bogus'")
  endif()
  if(TARGET chase-soundtest)
    add_test(NAME soundtest-option-id-range COMMAND chase-soundtest --id -1)
    set_tests_properties(soundtest-option-id-range PROPERTIES PASS_REGULAR_EXPRESSION "chase-soundtest: --id: out of range, allowed 0 to 4294967295: '-1'")
    add_test(NAME soundtest-option-seconds COMMAND chase-soundtest --seconds 1,5)
    set_tests_properties(soundtest-option-seconds PROPERTIES PASS_REGULAR_EXPRESSION "chase-soundtest: --seconds: not a number: '1,5'")
    add_test(NAME soundtest-option-missing COMMAND chase-soundtest --names)
    set_tests_properties(soundtest-option-missing PROPERTIES PASS_REGULAR_EXPRESSION "chase-soundtest: --names: missing value: ''")
  endif()
  if(TARGET chase-fmv)
    add_test(NAME fmv-option-time COMMAND chase-fmv --time 2s video.moflex)
    set_tests_properties(fmv-option-time PROPERTIES PASS_REGULAR_EXPRESSION "chase-fmv: --time: not a number: '2s'")
  endif()
  if(TARGET chase-frontshot)
    add_test(NAME frontshot-option-language COMMAND chase-frontshot --language 3x)
    set_tests_properties(frontshot-option-language PROPERTIES PASS_REGULAR_EXPRESSION "chase-frontshot: --language: not a whole number: '3x'")
    add_test(NAME frontshot-option-script COMMAND chase-frontshot --script "run x")
    set_tests_properties(frontshot-option-script PROPERTIES PASS_REGULAR_EXPRESSION "chase-frontshot: --script: not a number: 'x'")
  endif()
  if(TARGET chase-hudshot)
    add_test(NAME hudshot-option-script COMMAND chase-hudshot --script "run 1x")
    set_tests_properties(hudshot-option-script PROPERTIES PASS_REGULAR_EXPRESSION "chase-hudshot: --script: not a whole number: '1x'")
    add_test(NAME hudshot-option-unknown COMMAND chase-hudshot --bogus)
    set_tests_properties(hudshot-option-unknown PROPERTIES PASS_REGULAR_EXPRESSION "chase-hudshot: --bogus: unknown option: '--bogus'")
  endif()
  if(TARGET chase-savecheck)
    add_test(NAME savecheck-option-unknown COMMAND chase-savecheck --bogus)
    set_tests_properties(savecheck-option-unknown PROPERTIES PASS_REGULAR_EXPRESSION "chase-savecheck: --bogus: unknown option: '--bogus'")
  endif()
  if(TARGET chase-tabledump)
    add_test(NAME tabledump-option-unknown COMMAND chase-tabledump --bogus)
    set_tests_properties(tabledump-option-unknown PROPERTIES PASS_REGULAR_EXPRESSION "chase-tabledump: --bogus: unknown option: '--bogus'")
  endif()
  if(TARGET chase-input-lab)
    add_test(NAME input-lab-option-unknown COMMAND chase-input-lab --bogus)
    set_tests_properties(input-lab-option-unknown PROPERTIES PASS_REGULAR_EXPRESSION "chase-input-lab: --bogus: unknown option: '--bogus'")
  endif()
  if(TARGET chase-viewer AND EXISTS "${CHASE_ROMFS_DIR}/lego_city3ds.fib")
    # Opt-in: renders one model offscreen (SDL offscreen driver + EGL) and saves a screenshot.
    add_test(NAME viewer-real-smoke COMMAND chase-viewer
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --screenshot viewer-smoke.bmp
      csdata/models/1_1_e/trolley/trolley.bfnmdl)
    set_tests_properties(viewer-real-smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "850 triangles, 0 missing textures.*saved viewer-smoke.bmp")
    add_test(NAME viewer-cutscene-smoke COMMAND chase-viewer
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --screenshot viewer-cutscene.bmp
      --cutscene csdata/models/1_1_a --frame 200)
    set_tests_properties(viewer-cutscene-smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "cutscene: 5 actor\\(s\\), 405 camera frames.*0 missing textures.*saved viewer-cutscene.bmp")
    add_test(NAME debug-overlay-off COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --gameplay --screenshot debug-overlay-off.bmp --frames 2)
    set_tests_properties(debug-overlay-off PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" FIXTURES_SETUP debug_overlay_off
      PASS_REGULAR_EXPRESSION "saved debug-overlay-off.bmp after [0-9]+ update")
    add_test(NAME debug-overlay-on COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --gameplay --debug-overlay --screenshot debug-overlay-on.bmp --frames 2)
    set_tests_properties(debug-overlay-on PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" FIXTURES_SETUP debug_overlay_on
      PASS_REGULAR_EXPRESSION "saved debug-overlay-on.bmp after [0-9]+ update")
    add_test(NAME debug-overlay-area COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_debug_overlay_shots.py" debug-overlay-off.bmp debug-overlay-on.bmp)
    set_tests_properties(debug-overlay-area PROPERTIES FIXTURES_REQUIRED "debug_overlay_off;debug_overlay_on")
    add_test(NAME police-entrance-shot COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
      --call-script 1-1_MissionSkip --call-script Police_ExtToInt --start "904.2 16.58 1112.4" --facing 45.45
      --script "0 0 5;0 0 1 A;0 0 60" --screenshot police-entrance.bmp)
    set_tests_properties(police-entrance-shot PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" FIXTURES_SETUP police_entrance
      PASS_REGULAR_EXPRESSION "saved police-entrance.bmp after [0-9]+ update")
    add_test(NAME police-entrance-no-disc COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_police_entrance_shot.py" police-entrance.bmp)
    set_tests_properties(police-entrance-no-disc PROPERTIES FIXTURES_REQUIRED police_entrance)
    add_test(NAME game-real-smoke COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --gameplay --screenshot game-smoke.bmp --frames 2)
    set_tests_properties(game-real-smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "0 missing textures.*saved game-smoke.bmp after [0-9]+ update")
    if(EXISTS "${CHASE_REAL_SAVE}")
      add_test(NAME game-save-continue COMMAND chase-game
        --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
        --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --save "${CHASE_REAL_SAVE}" --slot 0 --frames 1 --screenshot game-continue.bmp)
      set_tests_properties(game-save-continue PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
        PASS_REGULAR_EXPRESSION "continue: slot 0 starts level 1 \\(lvl_00_policestation\\).*player at -2\\.1713 1\\.4203 17\\.2821")
      add_test(NAME game-save-continue-empty-slot COMMAND chase-game
        --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
        --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --save "${CHASE_REAL_SAVE}" --slot 1 --frames 1)
      set_tests_properties(game-save-continue-empty-slot PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
        PASS_REGULAR_EXPRESSION "the save slot is empty")
    endif()
    # Front-end mode: runs the scripted pad to the language screen and saves the window; a second run goes through every
    # screen to the hand-off and then shows the level of a new game.
    add_test(NAME front-game-smoke COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --front-script "until language 60; run 1" --screenshot front-smoke.bmp)
    set_tests_properties(front-game-smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "front end: [0-9]+ update\\(s\\), screen language, saved front-smoke.bmp")
    add_test(NAME front-game-handoff COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --front-script "until language 60; run 0.5; press A; run 0.5; press B; run 0.5; press A; run 0.5; press A; run 0.5; press A; until card 200; press A; until done 30"
      --screenshot front-handoff.bmp --frames 2)
    set_tests_properties(front-game-handoff PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "player at 891\\.472 16\\.5773 1109\\.06.*0 missing textures.*saved front-handoff.bmp after [0-9]+ update")
    add_test(NAME frontshot-real-smoke COMMAND chase-frontshot
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv" --movies "${CHASE_ROMFS_DIR}/cutscenes"
      --script "until logos 20; run 1; shot logos; until language 60; run 1; shot language")
    set_tests_properties(frontshot-real-smoke PROPERTIES PASS_REGULAR_EXPRESSION "language: screen language.*frontshot: [0-9]+ updates, screen language")
    # Scripted stick (raw circle pad "x y updates;..." at dt 0.033): without collision 31 run updates at 6.9 u/s
    # and 9 sprint updates at 10.35 u/s would move the player 10.13 units; the lobby steps raise him to height
    # 2.51 and the desk stops him after 8.05 units (the lobby has the record pose camera). 40 walk updates at 2.3 u/s move him 3.04 units on the floor.
    # These two run without camera collision (the camera yaw sets the walking direction).
    add_test(NAME game-script-run COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --script "0 150 40" --no-camera-collision --screenshot game-script-run.bmp)
    set_tests_properties(game-script-run PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scripted: 40 updates, player moved 8\\.05[0-9]* units.*height 2\\.50[0-9]*.*saved game-script-run.bmp")
    add_test(NAME game-script-walk COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --script "0 90 40" --no-camera-collision --screenshot game-script-walk.bmp)
    set_tests_properties(game-script-walk PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scripted: 40 updates, player moved 3\\.03[0-9]* units.*height 1\\.4[0-9]*.*saved game-script-walk.bmp")
    # In the police station lobby the record pose camera (docs/re/camera.md) is active, so the camera does not turn and
    # the run goes straight up the lobby steps: 8.05 units (replica output, not measured in the original).
    add_test(NAME game-script-camera COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --script "0 150 40" --screenshot game-script-camera.bmp)
    set_tests_properties(game-script-camera PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scripted: 40 updates, player moved 8\\.05[0-9]* units.*height 2\\.50[0-9]*.*saved game-script-camera.bmp")
    # Run west off the 1.06 m platform at x 897 in the Cherry Tree Hills street: the heights are the recorded
    # fall (tools/oracle/fall-ledge.txt: 17.5898, 17.4908, 17.3423, 17.1443, 16.8968 after the platform updates).
    add_test(NAME game-script-ledge COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
      --start "899.5 17.6393 1101" --facing 249.363 --script "0 150 40" --screenshot game-script-ledge.bmp)
    set_tests_properties(game-script-ledge PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "heights: (17\\.6393 )+17\\.5898 17\\.4908 17\\.3423 17\\.1443 16\\.8968 16\\.57.*saved game-script-ledge.bmp")
    # Walk into Police_Station_Transition_Lower after 1-1_MissionSkip enabled it (the dev option --call-script): the script
    # Police_ExtToInt opens WarningDialogBox, A answers it and PositionObject puts the player on
    # Enter_Police_Station_Position_Lower (yaw 45.4542 deg = heading 8275, height 16.5773 as in the oracle).
    add_test(NAME game-script-dialog COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
      --call-script 1-1_MissionSkip --start "904.2 16.58 1112.4" --facing 45.45
      --script "0 0 10;0 145 16;0 0 14;0 0 1 A;0 0 6" --screenshot game-script-dialog.bmp)
    set_tests_properties(game-script-dialog PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "script dialog opened at update 23.*script dialog answered at update 40.*facing 8275, .*height 16\\.5773")
    # Lobby exit and station entry at run time: the walk-in bound fires SceneChange_LobbyToCherryTree (update 23), A on the
    # loading page (update 36) and the switch 4 updates later; the street start is the point LobbyToCherryTree_P1 (yaw
    # 249.25 deg = heading -20161, height 16.5773, oracle traces of agent H, docs/re/streaming.md). The entry runs the script
    # Police_ExtToInt (request at update 164: 123 updates after the director camera starts, as in the oracle trace of docs/re/dcam.md) and lands at Lobby_Dest_CherryTree_P1 (height 2.0901).
    add_test(NAME game-scene-change-exit COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl
      --start "-1.97 1.436 14.843" --facing 0
      --script "0 -145 30;0 0 6;0 0 1 A;0 0 10" --screenshot game-scene-change-exit.bmp)
    set_tests_properties(game-scene-change-exit PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scene change requested by SceneChange_LobbyToCherryTree at update 23.*scene change to WorldLevel_01_CherryTreeHills after 41 updates.*facing -20161, .*height 16\\.5773")
    add_test(NAME game-scene-change-entry COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
      --call-script 1-1_MissionSkip --start "904.2 16.58 1112.4" --facing 45.45
      --script "0 0 10;0 145 16;0 0 14;0 0 1 A;0 0 150;0 0 1 A;0 0 6" --screenshot game-scene-change-entry.bmp)
    set_tests_properties(game-scene-change-entry PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scene change requested by SceneChange_Police_ExtToInt at update 164.*scene change to WorldLevel_00_PoliceStation after 196 updates.*facing 27729, .*height 2\\.0901")
    # Standing jump in the Cherry Tree Hills street (B button for one update): the heights are the recorded rise
    # (tools/oracle/jump-ground.txt, jstand: 16.8593, 17.1297, 17.4068, 17.6840, ... to the apex 18.6815).
    add_test(NAME game-script-jump COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
      --start "886.764 16.589 1111.166" --script "0 0 2;0 0 1 B;0 0 12" --screenshot game-script-jump.bmp)
    set_tests_properties(game-script-jump PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "heights: 16\\.589 16\\.589 16\\.589 16\\.859[0-9]* 17\\.129[0-9]* 17\\.40[67][0-9]* 17\\.68[34][0-9]* 17\\.92[89][0-9]* 18\\.17[0-9]* 18\\.34[0-9]* 18\\.52[0-9]* 18\\.5[89][0-9]* 18\\.65[0-9]* 18\\.66[0-9]* 18\\.68[0-9]*.*saved game-script-jump.bmp")
    # The display during play: the player stands on a silver stud (tools/oracle/hud-bar.txt); after 70 updates the stud has flown to
    # the counter, the total is 10 and the HUD shows it (four hearts: the class attribute Health defaults to 4).
    add_test(NAME game-script-hud COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
      --start "862.5931 16.1988 1060.5875" --window 400x480 --script "0 0 70" --screenshot game-script-hud.bmp)
    set_tests_properties(game-script-hud PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "studs: counter 10, 0 collecting, 1 collected.*hud: 4 hearts, stud display 10, bar state 2.*saved game-script-hud.bmp")
    # Room streaming in the level: starting in Room7 (position and view of the oracle state street-at-entrance +1 update, 9 rooms
    # loaded), walking east to west enters Room8, Room6 unloads in that update and Room16 finishes loading 9 updates later
    # (the update numbers are the port's own walk, the rule is pinned by room-streaming-real-data).
    add_test(NAME game-room-streaming COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
      --start "903.71875 16.573267 1113.328247" --facing 11.3 --window 400x480 --script "20 150 70")
    set_tests_properties(game-room-streaming PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "room streaming: 43 rooms, start Room7, 9 loaded.*room 7 -> 8 at update 54.*rooms loaded: 8 at update 54.*rooms loaded: 9 at update 63")
    # Opt-in: decodes and plays one looping music track for 0.3 s on SDL's dummy audio driver.
    add_test(NAME soundtest-real-smoke COMMAND chase-soundtest --seconds 0.3
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      music/act_bass_break.bwav)
    set_tests_properties(soundtest-real-smoke PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "12000 Hz, 2 channel")
    add_test(NAME soundtest-id-smoke COMMAND chase-soundtest --seconds 0.3
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --id 1)
    set_tests_properties(soundtest-id-smoke PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "id 1 ui/brickcounter1, volume 80%.*sounds/ui/brickcounter1.bwav")
  endif()
  if(TARGET chase-fmv AND EXISTS "${CHASE_ROMFS_DIR}/lego_city3ds.fib" AND EXISTS "${CHASE_ROMFS_DIR}/cutscenes/01_gameintro.moflex")
    add_test(NAME fmv-real-smoke COMMAND chase-fmv
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --lang us --screenshot fmv-smoke.bmp --time 3.0
      "${CHASE_ROMFS_DIR}/cutscenes/01_gameintro.moflex")
    set_tests_properties(fmv-real-smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "subtitle lines: 1, saved fmv-smoke.bmp")
  endif()
endif()

add_executable(camera-lock-census tools/camera_lock_census.cpp)
target_link_libraries(camera-lock-census PRIVATE chase_runtime chase_assets)
target_compile_options(camera-lock-census PRIVATE -Wall -Wextra -Wpedantic)
