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
# Game runtime core (plan S4e): recovered frame logic, no SDL dependency.
add_library(chase_runtime src/runtime/scheduler.cpp src/runtime/pad.cpp src/runtime/player_intent.cpp src/runtime/player_locomotion.cpp src/runtime/follow_camera.cpp src/runtime/collision.cpp src/runtime/camera_sweep.cpp src/runtime/player_vertical.cpp src/runtime/player_motion.cpp)
target_link_libraries(chase_runtime PUBLIC chase_assets)
target_include_directories(chase_runtime PUBLIC src)
target_compile_options(chase_runtime PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
set_target_properties(chase_runtime PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_sources(chase_runtime PRIVATE src/runtime/studs.cpp src/runtime/sound_events.cpp src/runtime/music.cpp src/runtime/level_start.cpp src/runtime/door.cpp)
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
  add_library(chase_render src/render/scene.cpp src/render/character.cpp)
  target_link_libraries(chase_render PUBLIC chase_assets PkgConfig::SDL2 PkgConfig::EPOXY)
  target_compile_options(chase_render PRIVATE -Wall -Wextra -Wpedantic)
  target_sources(chase_render PRIVATE src/render/studs.cpp)
  add_executable(chase-viewer src/viewer.cpp)
  target_link_libraries(chase-viewer PRIVATE chase_render chase_audio chase_input)
  target_compile_options(chase-viewer PRIVATE -Wall -Wextra -Wpedantic)
  if(FFMPEG_FOUND)
    add_executable(chase-game src/game.cpp)
    target_link_libraries(chase-game PRIVATE chase_render chase_runtime chase_input chase_audio chase_frontend_movie)
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
  add_executable(progress-tables-test tests/progress_tables_test.cpp)
  target_link_libraries(progress-tables-test PRIVATE chase_runtime)
  target_compile_options(progress-tables-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME progress-tables COMMAND progress-tables-test)
  add_test(NAME progress-readme COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/progress_report.py" --check "${CMAKE_SOURCE_DIR}/README.md")
  set_tests_properties(progress-readme PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "CHASE_ASSETS_LIB=$<TARGET_FILE:chase_assets_c>")
  add_executable(door-test tests/door_test.cpp)
  target_link_libraries(door-test PRIVATE chase_runtime)
  target_compile_options(door-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME door-core COMMAND door-test)
  add_executable(camera-sweep-test tests/camera_sweep_test.cpp)
  target_link_libraries(camera-sweep-test PRIVATE chase_runtime)
  target_compile_options(camera-sweep-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-sweep COMMAND camera-sweep-test)
  add_executable(locomotion-real-test tests/locomotion_real_test.cpp)
  target_link_libraries(locomotion-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(locomotion-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME locomotion-real-data COMMAND locomotion-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_LOCO_TRACE_DIR}")
  set_tests_properties(locomotion-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
    add_test(NAME game-real-smoke COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --gameplay --screenshot game-smoke.bmp --frames 2)
    set_tests_properties(game-real-smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "0 missing textures.*saved game-smoke.bmp after [0-9]+ update")
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
    # 2.51 and the desk stops him after 8.10 units. 40 walk updates at 2.3 u/s move him 3.04 units on the floor.
    # These two run without camera collision (the camera yaw sets the walking direction).
    add_test(NAME game-script-run COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --script "0 150 40" --no-camera-collision --screenshot game-script-run.bmp)
    set_tests_properties(game-script-run PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scripted: 40 updates, player moved 8\\.10[0-9]* units.*height 2\\.50[0-9]*.*saved game-script-run.bmp")
    add_test(NAME game-script-walk COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --script "0 90 40" --no-camera-collision --screenshot game-script-walk.bmp)
    set_tests_properties(game-script-walk PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scripted: 40 updates, player moved 3\\.03[0-9]* units.*height 1\\.4[0-9]*.*saved game-script-walk.bmp")
    # With camera collision the start corridor of the police station pushes the camera 6.4 degrees round (side push of
    # docs/re/camera.md), so the same run no longer reaches the lobby steps; the placed objects of the level (docs/re/collision.md) shorten
    # the run from 8.83 to 8.77 units (replica output, not measured in the original).
    add_test(NAME game-script-camera COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --script "0 150 40" --screenshot game-script-camera.bmp)
    set_tests_properties(game-script-camera PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scripted: 40 updates, player moved 8\\.76[0-9]* units.*height 1\\.43[0-9]*.*saved game-script-camera.bmp")
    # Run west off the 1.06 m platform at x 897 in the Cherry Tree Hills street: the heights are the recorded
    # fall (tools/oracle/fall-ledge.txt: 17.5898, 17.4908, 17.3423, 17.1443, 16.8968 after the platform updates).
    add_test(NAME game-script-ledge COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
      --start "899.5 17.6393 1101" --script "0 150 40" --screenshot game-script-ledge.bmp)
    set_tests_properties(game-script-ledge PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "heights: (17\\.6393 )+17\\.5898 17\\.4908 17\\.3423 17\\.1443 16\\.8968 16\\.57.*saved game-script-ledge.bmp")
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
