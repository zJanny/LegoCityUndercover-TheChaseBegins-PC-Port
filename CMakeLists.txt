cmake_minimum_required(VERSION 3.16)
project(chase_input_foundation LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # build/compile_commands.json for clangd
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()
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
add_library(chase_assets src/assets/fuse.cpp src/assets/pooled.cpp src/assets/texture.cpp src/assets/model.cpp src/assets/material.cpp src/assets/level.cpp src/assets/object_collision.cpp src/assets/audio.cpp src/assets/anim.cpp src/assets/nav.cpp src/assets/text.cpp src/assets/cutscene.cpp src/assets/table.cpp src/assets/config_text.cpp src/assets/pregen.cpp src/assets/xaml.cpp src/assets/subtitles.cpp src/assets/particles.cpp)
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
add_library(chase_save_progress src/save/slot_progress.cpp src/save/continue_start.cpp src/save/save_writer.cpp)
target_link_libraries(chase_save_progress PUBLIC chase_save chase_assets)
target_compile_options(chase_save_progress PRIVATE -Wall -Wextra -Wpedantic)
# Game runtime core (plan S4e): recovered frame logic, no SDL dependency.
add_library(chase_runtime src/runtime/scheduler.cpp src/runtime/pad.cpp src/runtime/player_intent.cpp src/runtime/player_locomotion.cpp src/runtime/follow_camera.cpp src/runtime/collision.cpp src/runtime/camera_sweep.cpp src/runtime/camera_reset_probe.cpp src/runtime/camera_near_plane.cpp src/runtime/player_vertical.cpp src/runtime/player_motion.cpp src/runtime/camera_records.cpp src/runtime/camera_rig.cpp src/runtime/camera_lock_icon.cpp)
target_link_libraries(chase_runtime PUBLIC chase_assets)
target_include_directories(chase_runtime PUBLIC src)
target_compile_options(chase_runtime PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
set_target_properties(chase_runtime PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_sources(chase_runtime PRIVATE src/runtime/studs.cpp src/runtime/sound_events.cpp src/runtime/music.cpp src/runtime/level_start.cpp src/runtime/script_runner.cpp src/runtime/natives_objects.cpp src/runtime/natives_effects.cpp src/runtime/natives_world.cpp src/runtime/script_runner_natives.cpp src/runtime/natives_animation.cpp src/runtime/natives_cutscene.cpp src/runtime/natives_progress.cpp src/runtime/natives_music.cpp src/runtime/natives_sound.cpp src/runtime/music_block.cpp src/runtime/door.cpp src/runtime/scene_change.cpp src/runtime/room_streaming.cpp src/runtime/room_streamer.cpp src/runtime/scene_start.cpp src/runtime/dcam.cpp src/runtime/debug_metrics.cpp src/runtime/object_visibility.cpp src/runtime/script_text.cpp src/runtime/script_triggers.cpp src/runtime/objectives.cpp src/runtime/ai.cpp src/runtime/ai_simple_path.cpp src/runtime/fog.cpp src/runtime/lighting.cpp src/runtime/sky.cpp src/runtime/script_placement.cpp src/runtime/ai_go_to_target.cpp src/runtime/nav_grid.cpp src/runtime/nav_path.cpp src/runtime/nav_follow.cpp src/runtime/nav_load.cpp src/runtime/room_locator.cpp src/runtime/object_rooms.cpp src/runtime/ai_super_jump.cpp src/runtime/ai_face_target.cpp src/runtime/waypoints.cpp src/runtime/buildit.cpp src/runtime/buildit_set.cpp src/runtime/buildit_wobble.cpp src/runtime/prop_animation.cpp src/runtime/prop_nodes.cpp src/runtime/buildit_bricks.cpp src/runtime/mesh_bouncer.cpp src/runtime/reference_trigger.cpp src/runtime/vehicle.cpp src/runtime/vehicle_boarding.cpp src/runtime/vehicle_drive.cpp src/runtime/vehicle_body.cpp src/runtime/vehicle_wheels.cpp src/runtime/vehicle_drivetrain.cpp src/runtime/vehicle_wheel_pose.cpp src/runtime/vehicle_hud.cpp src/runtime/vehicle_damage.cpp src/runtime/vehicle_break.cpp src/runtime/head_icon.cpp src/runtime/floor_walk.cpp src/runtime/vehicle_collision.cpp src/runtime/vehicle_box_contact.cpp src/runtime/carry.cpp src/runtime/carry_animation.cpp src/runtime/scanner.cpp src/runtime/cutscene_cast.cpp src/runtime/collision_enable.cpp src/runtime/game_random.cpp src/runtime/npc_idle.cpp src/runtime/animation_slots.cpp src/runtime/director_hold.cpp src/runtime/script_thread_log.cpp src/runtime/vehicle_world.cpp src/runtime/vehicle_sphere_sweep.cpp src/runtime/buildit_player.cpp src/runtime/vehicle_prop_smash.cpp src/runtime/detective.cpp src/runtime/hand_item.cpp src/runtime/use_spot.cpp src/runtime/zipline.cpp src/runtime/brick_grab.cpp src/runtime/climb_bar.cpp src/runtime/pole.cpp)
target_sources(chase_runtime PRIVATE src/runtime/player_attack.cpp)
target_sources(chase_runtime PRIVATE src/runtime/vehicle_camera.cpp src/runtime/particles.cpp src/runtime/debris.cpp src/runtime/prop_destroy.cpp)
target_sources(chase_runtime PRIVATE src/runtime/prop_wobbly_destructible.cpp src/runtime/prop_bouncer.cpp)
# Front end (boot to the slot list): CPU-drawn screens, no SDL dependency.
add_library(chase_frontend src/frontend/script.cpp src/frontend/frontend.cpp src/frontend/pages.cpp src/frontend/canvas.cpp src/frontend/resources.cpp src/frontend/layout.cpp src/frontend/flow.cpp src/frontend/hud.cpp src/frontend/script_text_pages.cpp src/frontend/minimap.cpp src/frontend/map_marker.cpp src/frontend/slot_summaries.cpp)
target_link_libraries(chase_frontend PUBLIC chase_assets chase_runtime chase_save_progress)
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
  set(CHASE_SHADER_SOURCES sceneVertex:scene.vert.glsl sceneFragment:scene.frag.glsl hudVertex:hud.vert.glsl hudFragment:hud.frag.glsl particleVertex:particle.vert.glsl particleFragment:particle.frag.glsl)
  set(CHASE_SHADERS_CPP ${CMAKE_BINARY_DIR}/generated/shaders.cpp)
  add_custom_command(OUTPUT ${CHASE_SHADERS_CPP}
    COMMAND ${CMAKE_COMMAND} "-DSHADERS=${CHASE_SHADER_SOURCES}" -DSHADER_DIR=${CHASE_SHADER_DIR} -DOUTPUT=${CHASE_SHADERS_CPP}
      -P ${CMAKE_SOURCE_DIR}/tools/embed_shaders.cmake
    DEPENDS ${CHASE_SHADER_DIR}/scene.vert.glsl ${CHASE_SHADER_DIR}/scene.frag.glsl ${CHASE_SHADER_DIR}/hud.vert.glsl ${CHASE_SHADER_DIR}/hud.frag.glsl
      ${CHASE_SHADER_DIR}/particle.vert.glsl ${CHASE_SHADER_DIR}/particle.frag.glsl
      ${CMAKE_SOURCE_DIR}/tools/embed_shaders.cmake
    VERBATIM)
  add_library(chase_render src/render/scene.cpp src/render/character.cpp ${CHASE_SHADERS_CPP})
  target_include_directories(chase_render PUBLIC ${CMAKE_SOURCE_DIR}/src)
  target_link_libraries(chase_render PUBLIC chase_assets chase_runtime PkgConfig::SDL2 PkgConfig::EPOXY)
  target_compile_options(chase_render PRIVATE -Wall -Wextra -Wpedantic)
  target_sources(chase_render PRIVATE src/render/studs.cpp src/render/debug_text.cpp src/render/debug_overlay.cpp
    src/render/program.cpp src/render/particles.cpp src/render/window.cpp src/render/hud_overlay.cpp src/render/front_presenter.cpp src/render/frame_stats.cpp)
  add_executable(chase-viewer src/viewer.cpp)
  target_link_libraries(chase-viewer PRIVATE chase_render chase_audio chase_input)
  target_compile_options(chase-viewer PRIVATE -Wall -Wextra -Wpedantic)
  if(FFMPEG_FOUND)
    add_executable(chase-game src/game.cpp src/game_options.cpp src/game_player.cpp src/game_carry.cpp src/game_scanner.cpp src/game_doors.cpp src/game_prop_parts.cpp src/game_level.cpp src/game_level_update.cpp src/game_vehicle.cpp src/game_vehicle_break.cpp src/game_vehicle_place.cpp src/game_vehicle_camera.cpp src/game_level_rooms.cpp src/game_cutscene.cpp src/game_level_cutscene.cpp src/game_characters.cpp src/game_particles.cpp src/game_level_natives.cpp src/game_detective.cpp src/game_detective_glass.cpp src/game_effects.cpp src/game_vehicle_effects.cpp src/game_zipline.cpp src/game_brick_grab.cpp src/game_climb_bar.cpp src/game_pole.cpp)
    target_sources(chase-game PRIVATE src/game_props_destructible.cpp)
    target_sources(chase-game PRIVATE src/game_attack.cpp)
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
set(CHASE_CAMERA_VEHICLE_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/camera-vehicle-traces" CACHE PATH
    "Recorded original-game camera traces while driving the police car (exported CSV) for the vehicle camera test")
set(CHASE_CAMERA_WALL_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/camwall-out" CACHE PATH
    "Recorded original-game camera traces near walls (.mem memtraces and exported CSV) for the opt-in differential tests")
set(CHASE_STUDS_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/studs-traces" CACHE PATH
    "Recorded original-game stud pickup traces for the opt-in differential test")
set(CHASE_HUD_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/hud-traces" CACHE PATH
    "Recorded original-game HUD stud bar traces for the opt-in HUD test")
set(CHASE_AUDIO_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/audio-traces" CACHE PATH
    "Recorded original-game sound-call traces (CSV from tools/export_sound_trace.py) for the opt-in differential test")
option(CHASE_BUILD_TESTS "Build the test programs and register the CTest tests" ON)
set(BUILD_TESTING ${CHASE_BUILD_TESTS} CACHE BOOL "" FORCE)
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
    add_executable(script-text-pages-real-test tests/script_text_pages_real_test.cpp)
    target_link_libraries(script-text-pages-real-test PRIVATE chase_frontend)
    target_compile_options(script-text-pages-real-test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME script-text-pages-real-data COMMAND script-text-pages-real-test
      "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
    set_tests_properties(script-text-pages-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(frame-pacer-test tests/frame_pacer_test.cpp)
  target_link_libraries(frame-pacer-test PRIVATE chase_runtime)
  target_compile_options(frame-pacer-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME frame-pacer COMMAND frame-pacer-test)
  add_executable(ai-test tests/ai_test.cpp)
  target_link_libraries(ai-test PRIVATE chase_runtime)
  target_compile_options(ai-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME ai-core COMMAND ai-test)
  add_executable(npc-idle-test tests/npc_idle_test.cpp)
  target_link_libraries(npc-idle-test PRIVATE chase_runtime)
  target_compile_options(npc-idle-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME npc-idle COMMAND npc-idle-test)
  add_executable(animation-slots-test tests/animation_slots_test.cpp)
  target_link_libraries(animation-slots-test PRIVATE chase_runtime)
  target_compile_options(animation-slots-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME animation-slots COMMAND animation-slots-test)
  add_executable(nav-grid-test tests/nav_grid_test.cpp)
  target_link_libraries(nav-grid-test PRIVATE chase_runtime)
  target_compile_options(nav-grid-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME nav-grid COMMAND nav-grid-test)
  add_executable(nav-path-test tests/nav_path_test.cpp)
  target_link_libraries(nav-path-test PRIVATE chase_runtime)
  target_compile_options(nav-path-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME nav-path COMMAND nav-path-test)
  add_executable(script-runner-test tests/script_runner_test.cpp)
  target_link_libraries(script-runner-test PRIVATE chase_runtime)
  target_compile_options(script-runner-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME script-runner COMMAND script-runner-test)
  add_executable(script-triggers-test tests/script_triggers_test.cpp)
  target_link_libraries(script-triggers-test PRIVATE chase_runtime)
  target_compile_options(script-triggers-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME script-triggers COMMAND script-triggers-test)
  add_executable(natives-objects-test tests/natives_objects_test.cpp)
  target_link_libraries(natives-objects-test PRIVATE chase_runtime)
  target_compile_options(natives-objects-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-objects COMMAND natives-objects-test)
  add_executable(natives-world-test tests/natives_world_test.cpp)
  target_link_libraries(natives-world-test PRIVATE chase_runtime)
  target_compile_options(natives-world-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-world COMMAND natives-world-test)
  add_executable(natives-cutscene-test tests/natives_cutscene_test.cpp)
  target_link_libraries(natives-cutscene-test PRIVATE chase_runtime)
  target_compile_options(natives-cutscene-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-cutscene COMMAND natives-cutscene-test)
  add_executable(cutscene-cast-test tests/cutscene_cast_test.cpp)
  target_link_libraries(cutscene-cast-test PRIVATE chase_runtime chase_assets)
  target_compile_options(cutscene-cast-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME cutscene-cast COMMAND cutscene-cast-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  add_executable(natives-animation-test tests/natives_animation_test.cpp)
  target_link_libraries(natives-animation-test PRIVATE chase_runtime)
  target_compile_options(natives-animation-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-animation COMMAND natives-animation-test)
  add_executable(natives-progress-test tests/natives_progress_test.cpp)
  target_link_libraries(natives-progress-test PRIVATE chase_runtime)
  target_compile_options(natives-progress-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-progress COMMAND natives-progress-test)
  add_executable(natives-music-test tests/natives_music_test.cpp)
  target_link_libraries(natives-music-test PRIVATE chase_runtime)
  target_compile_options(natives-music-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-music COMMAND natives-music-test)
  add_executable(natives-sound-test tests/natives_sound_test.cpp)
  target_link_libraries(natives-sound-test PRIVATE chase_runtime)
  target_compile_options(natives-sound-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-sound COMMAND natives-sound-test)
  add_executable(music-block-test tests/music_block_test.cpp)
  target_link_libraries(music-block-test PRIVATE chase_runtime)
  target_compile_options(music-block-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME music-block COMMAND music-block-test)
  add_executable(script-text-test tests/script_text_test.cpp)
  target_link_libraries(script-text-test PRIVATE chase_runtime chase_assets)
  target_compile_options(script-text-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME script-text COMMAND script-text-test)
  add_test(NAME script-text-real-data COMMAND script-text-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(script-text-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(objectives-test tests/objectives_test.cpp)
  target_link_libraries(objectives-test PRIVATE chase_runtime chase_assets)
  target_compile_options(objectives-test PRIVATE -Wall -Wextra -Wpedantic)
  add_executable(waypoints-test tests/waypoints_test.cpp)
  target_link_libraries(waypoints-test PRIVATE chase_runtime chase_assets)
  target_compile_options(waypoints-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME waypoints COMMAND waypoints-test)
  add_test(NAME waypoints-real-data COMMAND waypoints-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(waypoints-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(buildit-test tests/buildit_test.cpp)
  target_link_libraries(buildit-test PRIVATE chase_runtime chase_assets)
  target_compile_options(buildit-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME buildit COMMAND buildit-test)
  add_executable(buildit-player-test tests/buildit_player_test.cpp)
  target_link_libraries(buildit-player-test PRIVATE chase_runtime chase_assets)
  target_compile_options(buildit-player-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME buildit-player COMMAND buildit-player-test)
  add_executable(detective-test tests/detective_test.cpp)
  target_link_libraries(detective-test PRIVATE chase_runtime chase_assets)
  target_compile_options(detective-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME detective COMMAND detective-test)
  add_executable(hand-item-test tests/hand_item_test.cpp)
  target_link_libraries(hand-item-test PRIVATE chase_runtime chase_assets)
  target_compile_options(hand-item-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME hand-item COMMAND hand-item-test)
  add_executable(hand-item-real-test tests/hand_item_real_test.cpp)
  target_link_libraries(hand-item-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(hand-item-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME hand-item-real-data COMMAND hand-item-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(hand-item-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(detective-real-test tests/detective_real_test.cpp)
  target_link_libraries(detective-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(detective-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME detective-real-data COMMAND detective-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(detective-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(use-spot-real-test tests/use_spot_real_test.cpp)
  target_link_libraries(use-spot-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(use-spot-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME use-spot-real-data COMMAND use-spot-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(use-spot-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(script-starters-real-test tests/script_starters_real_test.cpp)
  target_link_libraries(script-starters-real-test PRIVATE chase_assets)
  target_compile_options(script-starters-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME script-starters-real-data COMMAND script-starters-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(script-starters-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(buildit-wobble-test tests/buildit_wobble_test.cpp)
  target_link_libraries(buildit-wobble-test PRIVATE chase_runtime chase_assets)
  target_compile_options(buildit-wobble-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME buildit-wobble COMMAND buildit-wobble-test)
  add_executable(prop-parts-test tests/prop_parts_test.cpp)
  target_link_libraries(prop-parts-test PRIVATE chase_runtime chase_assets)
  target_compile_options(prop-parts-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME prop-parts COMMAND prop-parts-test)

  add_executable(reference-trigger-test tests/reference_trigger_test.cpp)
  target_link_libraries(reference-trigger-test PRIVATE chase_runtime chase_assets)
  target_compile_options(reference-trigger-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME reference-trigger COMMAND reference-trigger-test)
  add_executable(collision-enable-test tests/collision_enable_test.cpp)
  target_link_libraries(collision-enable-test PRIVATE chase_runtime chase_assets)
  target_compile_options(collision-enable-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME collision-enable COMMAND collision-enable-test)
  add_executable(carry-test tests/carry_test.cpp)
  target_link_libraries(carry-test PRIVATE chase_runtime chase_assets)
  target_compile_options(carry-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME carry COMMAND carry-test)
  add_executable(carry-real-test tests/carry_real_test.cpp)
  target_link_libraries(carry-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(carry-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME carry-real-data COMMAND carry-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(carry-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(scanner-test tests/scanner_test.cpp)
  target_link_libraries(scanner-test PRIVATE chase_runtime chase_assets)
  target_compile_options(scanner-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scanner COMMAND scanner-test)
  add_executable(scanner-real-test tests/scanner_real_test.cpp)
  target_link_libraries(scanner-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(scanner-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scanner-real-data COMMAND scanner-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(scanner-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(zipline-test tests/zipline_test.cpp)
  target_link_libraries(zipline-test PRIVATE chase_runtime chase_assets)
  target_compile_options(zipline-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME zipline COMMAND zipline-test)
  add_executable(zipline-real-test tests/zipline_real_test.cpp)
  target_link_libraries(zipline-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(zipline-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME zipline-real-data COMMAND zipline-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(zipline-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(brick-grab-test tests/brick_grab_test.cpp)
  target_link_libraries(brick-grab-test PRIVATE chase_runtime chase_assets)
  target_compile_options(brick-grab-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME brick-grab COMMAND brick-grab-test)
  add_executable(brick-grab-real-test tests/brick_grab_real_test.cpp)
  target_link_libraries(brick-grab-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(brick-grab-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME brick-grab-real-data COMMAND brick-grab-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(brick-grab-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(climb-bar-test tests/climb_bar_test.cpp)
  target_link_libraries(climb-bar-test PRIVATE chase_runtime chase_assets)
  target_compile_options(climb-bar-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME climb-bar COMMAND climb-bar-test)
  add_executable(climb-bar-real-test tests/climb_bar_real_test.cpp)
  target_link_libraries(climb-bar-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(climb-bar-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME climb-bar-real-data COMMAND climb-bar-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(climb-bar-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(pole-test tests/pole_test.cpp)
  target_link_libraries(pole-test PRIVATE chase_runtime chase_assets)
  target_compile_options(pole-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pole COMMAND pole-test)
  add_executable(pole-real-test tests/pole_real_test.cpp)
  target_link_libraries(pole-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(pole-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pole-real-data COMMAND pole-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(pole-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(carry-animation-real-test tests/carry_animation_real_test.cpp)
  target_link_libraries(carry-animation-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(carry-animation-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME carry-animation-real-data COMMAND carry-animation-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(carry-animation-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(buildit-real-test tests/buildit_real_test.cpp)
  target_link_libraries(buildit-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(buildit-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME buildit-real-data COMMAND buildit-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(buildit-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(prop-animation-real-test tests/prop_animation_real_test.cpp)
  target_link_libraries(prop-animation-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(prop-animation-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME prop-animation-real-data COMMAND prop-animation-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(prop-animation-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(prop-parts-real-test tests/prop_parts_real_test.cpp)
  target_link_libraries(prop-parts-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(prop-parts-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME prop-parts-real-data COMMAND prop-parts-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(prop-parts-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle-boarding-test tests/vehicle_boarding_test.cpp)
  target_link_libraries(vehicle-boarding-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-boarding-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-boarding COMMAND vehicle-boarding-test)
  add_test(NAME vehicle-boarding-real-data COMMAND vehicle-boarding-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(vehicle-boarding-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle-drive-test tests/vehicle_drive_test.cpp)
  target_link_libraries(vehicle-drive-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-drive-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-drive COMMAND vehicle-drive-test)
  add_test(NAME vehicle-drive-real-data COMMAND vehicle-drive-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(vehicle-drive-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle-body-test tests/vehicle_body_test.cpp)
  target_link_libraries(vehicle-body-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-body-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-body COMMAND vehicle-body-test)
  add_executable(vehicle-drivetrain-test tests/vehicle_drivetrain_test.cpp)
  target_link_libraries(vehicle-drivetrain-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-drivetrain-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-drivetrain COMMAND vehicle-drivetrain-test)
  add_executable(floor-graph-test tests/floor_graph_real_test.cpp)
  target_link_libraries(floor-graph-test PRIVATE chase_runtime chase_assets)
  target_compile_options(floor-graph-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME floor-graph-real-data COMMAND floor-graph-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(floor-graph-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle-collision-test tests/vehicle_collision_test.cpp)
  target_link_libraries(vehicle-collision-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-collision-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-collision COMMAND vehicle-collision-test)
  add_test(NAME vehicle-collision-real-data COMMAND vehicle-collision-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(vehicle-collision-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle-box-contact-test tests/vehicle_box_contact_test.cpp)
  target_link_libraries(vehicle-box-contact-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-box-contact-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-box-contact COMMAND vehicle-box-contact-test)
  add_test(NAME vehicle-box-contact-real-data COMMAND vehicle-box-contact-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(vehicle-box-contact-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle-sphere-sweep-test tests/vehicle_sphere_sweep_test.cpp)
  target_link_libraries(vehicle-sphere-sweep-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-sphere-sweep-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-sphere-sweep COMMAND vehicle-sphere-sweep-test)
  add_test(NAME vehicle-sphere-sweep-real-data COMMAND vehicle-sphere-sweep-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(vehicle-sphere-sweep-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle-prop-smash-test tests/vehicle_prop_smash_test.cpp)
  target_link_libraries(vehicle-prop-smash-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-prop-smash-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-prop-smash COMMAND vehicle-prop-smash-test)
  add_test(NAME vehicle-prop-smash-real-data COMMAND vehicle-prop-smash-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(vehicle-prop-smash-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(player-attack-test tests/player_attack_test.cpp)
  target_link_libraries(player-attack-test PRIVATE chase_runtime chase_assets)
  target_compile_options(player-attack-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME player-attack COMMAND player-attack-test)
  add_test(NAME player-attack-real-data COMMAND player-attack-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(player-attack-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle-world-test tests/vehicle_world_test.cpp)
  target_link_libraries(vehicle-world-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-world-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-world COMMAND vehicle-world-test)
  add_test(NAME vehicle-world-real-data COMMAND vehicle-world-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(vehicle-world-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle-rest-test tests/vehicle_rest_test.cpp)
  target_link_libraries(vehicle-rest-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-rest-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-rest-real-data COMMAND vehicle-rest-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(vehicle-rest-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle-wheels-test tests/vehicle_wheels_test.cpp)
  target_link_libraries(vehicle-wheels-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-wheels-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-wheels COMMAND vehicle-wheels-test)
  add_test(NAME vehicle-wheels-real-data COMMAND vehicle-wheels-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(vehicle-wheels-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vehicle-wheel-pose-test tests/vehicle_wheel_pose_test.cpp)
  target_link_libraries(vehicle-wheel-pose-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-wheel-pose-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-wheel-pose COMMAND vehicle-wheel-pose-test)
  add_executable(vehicle-damage-test tests/vehicle_damage_test.cpp)
  target_link_libraries(vehicle-damage-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-damage-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-damage COMMAND vehicle-damage-test)
  add_executable(debris-test tests/debris_test.cpp)
  target_link_libraries(debris-test PRIVATE chase_runtime chase_assets)
  target_compile_options(debris-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME debris-core COMMAND debris-test)
  add_executable(vehicle-break-test tests/vehicle_break_test.cpp)
  target_link_libraries(vehicle-break-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-break-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-break COMMAND vehicle-break-test)
  add_executable(vehicle-hud-test tests/vehicle_hud_test.cpp)
  target_link_libraries(vehicle-hud-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-hud-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-hud COMMAND vehicle-hud-test)
  add_test(NAME vehicle-hud-real-data COMMAND vehicle-hud-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(vehicle-hud-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(head-icon-test tests/head_icon_test.cpp)
  target_link_libraries(head-icon-test PRIVATE chase_runtime chase_assets)
  target_compile_options(head-icon-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME head-icon COMMAND head-icon-test)
  add_executable(vehicle-test tests/vehicle_test.cpp)
  target_link_libraries(vehicle-test PRIVATE chase_runtime chase_assets)
  target_compile_options(vehicle-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME vehicle-core COMMAND vehicle-test)
  add_test(NAME vehicle-real-data COMMAND vehicle-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(vehicle-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME objectives COMMAND objectives-test)
  add_executable(map-marker-test tests/map_marker_test.cpp)
  target_link_libraries(map-marker-test PRIVATE chase_frontend)
  target_compile_options(map-marker-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME map-marker COMMAND map-marker-test)
  add_test(NAME objectives-real-data COMMAND objectives-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(objectives-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(scene-start-test tests/scene_start_test.cpp)
  target_link_libraries(scene-start-test PRIVATE chase_runtime)
  target_compile_options(scene-start-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scene-start COMMAND scene-start-test)
  add_executable(object-rooms-test tests/object_rooms_test.cpp)
  target_link_libraries(object-rooms-test PRIVATE chase_runtime)
  target_compile_options(object-rooms-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME object-rooms COMMAND object-rooms-test)
  add_executable(dcam-test tests/dcam_test.cpp)
  target_link_libraries(dcam-test PRIVATE chase_runtime)
  target_compile_options(dcam-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME dcam COMMAND dcam-test)
  add_executable(director-hold-test tests/director_hold_test.cpp)
  target_link_libraries(director-hold-test PRIVATE chase_runtime)
  target_compile_options(director-hold-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME director-hold COMMAND director-hold-test)
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
  add_executable(camera-reset-probe-test tests/camera_reset_probe_test.cpp)
  target_link_libraries(camera-reset-probe-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-reset-probe-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-reset-probe COMMAND camera-reset-probe-test)
  add_executable(camera-near-plane-test tests/camera_near_plane_test.cpp)
  target_link_libraries(camera-near-plane-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-near-plane-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-near-plane COMMAND camera-near-plane-test)
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
  add_executable(clip-planes-test tests/clip_planes_test.cpp)
  target_link_libraries(clip-planes-test PRIVATE chase_runtime chase_assets)
  target_compile_options(clip-planes-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME clip-planes COMMAND clip-planes-test)
  add_executable(fog-test tests/fog_test.cpp)
  target_link_libraries(fog-test PRIVATE chase_runtime chase_assets)
  target_compile_options(fog-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME fog-core COMMAND fog-test)
  add_test(NAME fog-real-data COMMAND fog-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(fog-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(lighting-test tests/lighting_test.cpp)
  target_link_libraries(lighting-test PRIVATE chase_runtime chase_assets)
  target_compile_options(lighting-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME lighting-core COMMAND lighting-test)
  add_test(NAME lighting-real-data COMMAND lighting-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(lighting-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(sky-test tests/sky_test.cpp)
  target_link_libraries(sky-test PRIVATE chase_runtime chase_assets)
  target_compile_options(sky-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME sky-core COMMAND sky-test)
  add_test(NAME sky-real-data COMMAND sky-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(sky-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(npc-idle-real-test tests/npc_idle_real_test.cpp)
  target_link_libraries(npc-idle-real-test PRIVATE chase_assets)
  target_compile_options(npc-idle-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME npc-idle-real-data COMMAND npc-idle-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(npc-idle-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(ai-real-test tests/ai_real_test.cpp)
  target_link_libraries(ai-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(ai-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME ai-real-data COMMAND ai-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(ai-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(nav-path-real-test tests/nav_path_real_test.cpp)
  target_link_libraries(nav-path-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(nav-path-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME nav-path-real-data COMMAND nav-path-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(nav-path-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(script-triggers-real-test tests/script_triggers_real_test.cpp)
  target_link_libraries(script-triggers-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(script-triggers-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME script-triggers-real-data COMMAND script-triggers-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(script-triggers-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME script-dialog-real-data COMMAND script-dialog-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(script-dialog-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(natives-world-real-test tests/natives_world_real_test.cpp)
  target_link_libraries(natives-world-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(natives-world-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-world-real-data COMMAND natives-world-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(natives-world-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(natives-objects-real-test tests/natives_objects_real_test.cpp)
  target_link_libraries(natives-objects-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(natives-objects-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-objects-real-data COMMAND natives-objects-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(natives-objects-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(natives-cutscene-real-test tests/natives_cutscene_real_test.cpp)
  target_link_libraries(natives-cutscene-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(natives-cutscene-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-cutscene-real-data COMMAND natives-cutscene-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(natives-cutscene-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(natives-animation-real-test tests/natives_animation_real_test.cpp)
  target_link_libraries(natives-animation-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(natives-animation-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-animation-real-data COMMAND natives-animation-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(natives-animation-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(camera-vehicle-real-test tests/camera_vehicle_real_test.cpp)
  target_link_libraries(camera-vehicle-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-vehicle-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-vehicle-real-data COMMAND camera-vehicle-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib"
    "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_CAMERA_VEHICLE_TRACE_DIR}")
  set_tests_properties(camera-vehicle-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(save-writer-test tests/save_writer_test.cpp)
  target_link_libraries(save-writer-test PRIVATE chase_save_progress)
  target_compile_options(save-writer-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME save-writer COMMAND save-writer-test)
  add_executable(save-writer-real-test tests/save_writer_real_test.cpp)
  target_link_libraries(save-writer-real-test PRIVATE chase_save_progress chase_save_tables)
  target_compile_options(save-writer-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME save-writer-real-data COMMAND save-writer-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CHASE_REAL_SAVE}")
  set_tests_properties(save-writer-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(particles-test tests/particles_test.cpp)
  target_link_libraries(particles-test PRIVATE chase_runtime)
  target_compile_options(particles-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME particles-core COMMAND particles-test)
  add_executable(particles-real-test tests/particles_real_test.cpp)
  target_link_libraries(particles-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(particles-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME particles-real-data COMMAND particles-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(particles-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  # GCC 16 reports a false -Wfree-nonheap-object in the inlined std::vector<std::string> destructor at -O2.
  target_compile_options(game-options-test PRIVATE -Wall -Wextra -Wpedantic -Wno-free-nonheap-object)
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
    add_test(NAME game-option-start-cutscenes-value COMMAND chase-game --no-start-cutscenes yes)
    set_tests_properties(game-option-start-cutscenes-value PROPERTIES PASS_REGULAR_EXPRESSION "chase-game: yes: unknown option: 'yes'")
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
    add_test(NAME hudshot-option-ring COMMAND chase-hudshot --script "ring 4")
    set_tests_properties(hudshot-option-ring PROPERTIES PASS_REGULAR_EXPRESSION "chase-hudshot: --script: out of range, allowed 0 to 3: '4'")
    add_test(NAME hudshot-option-marker COMMAND chase-hudshot --script "marker 1")
    set_tests_properties(hudshot-option-marker PROPERTIES PASS_REGULAR_EXPRESSION "chase-hudshot: --script: expected marker X Z: 'marker 1'")
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
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --gameplay --script "0 0 2" --screenshot debug-overlay-off.bmp)
    set_tests_properties(debug-overlay-off PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" FIXTURES_SETUP debug_overlay_off
      PASS_REGULAR_EXPRESSION "saved debug-overlay-off.bmp after [0-9]+ update")
    add_test(NAME debug-overlay-on COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --gameplay --debug-overlay --script "0 0 2" --screenshot debug-overlay-on.bmp)
    set_tests_properties(debug-overlay-on PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" FIXTURES_SETUP debug_overlay_on
      PASS_REGULAR_EXPRESSION "saved debug-overlay-on.bmp after [0-9]+ update")
    add_test(NAME debug-overlay-area COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_debug_overlay_shots.py" debug-overlay-off.bmp debug-overlay-on.bmp)
    set_tests_properties(debug-overlay-area PROPERTIES FIXTURES_REQUIRED "debug_overlay_off;debug_overlay_on")
    add_test(NAME tip-text-shot COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --window 400x480 --call-script 1-1_OnScreenText_Script --screenshot tip-text-shot.bmp --frames 20)
    set_tests_properties(tip-text-shot PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" FIXTURES_SETUP tip_text_shot
      PASS_REGULAR_EXPRESSION "saved tip-text-shot.bmp after [0-9]+ update")
    add_test(NAME tip-text-panel COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_tip_shot.py" tip-text-shot.bmp)
    set_tests_properties(tip-text-panel PROPERTIES FIXTURES_REQUIRED tip_text_shot)
    add_test(NAME waypoint-marker-shot COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --window 400x480 --screenshot waypoint-marker.bmp --frames 700)
    set_tests_properties(waypoint-marker-shot PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" FIXTURES_SETUP waypoint_marker_shot
      PASS_REGULAR_EXPRESSION "saved waypoint-marker.bmp after [0-9]+ update")
    add_test(NAME waypoint-marker-position COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_waypoint_shot.py" waypoint-marker.bmp)
    set_tests_properties(waypoint-marker-position PROPERTIES FIXTURES_REQUIRED waypoint_marker_shot)
    add_test(NAME police-entrance-shot COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
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
    add_test(NAME front-overlay-off COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --front-script "until language 60; run 1" --screenshot front-overlay-off.bmp)
    set_tests_properties(front-overlay-off PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" FIXTURES_SETUP front_overlay_off
      PASS_REGULAR_EXPRESSION "saved front-overlay-off.bmp")
    add_test(NAME front-overlay-on COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --debug-overlay --front-script "until language 60; run 1" --screenshot front-overlay-on.bmp)
    set_tests_properties(front-overlay-on PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" FIXTURES_SETUP front_overlay_on
      PASS_REGULAR_EXPRESSION "saved front-overlay-on.bmp")
    add_test(NAME front-overlay-area COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_debug_overlay_shots.py" front-overlay-off.bmp front-overlay-on.bmp)
    set_tests_properties(front-overlay-area PROPERTIES FIXTURES_REQUIRED "front_overlay_off;front_overlay_on")
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
    # The lobby's first-visit camera tour (Lobby_Dcam_Intro 0) holds the player (docs/re/dcam.md); these runs start after it.
    # Scripted stick (raw circle pad "x y updates;..." at dt 0.033): without collision 31 run updates at 6.9 u/s
    # and 9 sprint updates at 10.35 u/s would move the player 10.13 units; the lobby steps raise him to height
    # 2.51 and the desk stops him after 8.10 units (the lobby has the record pose camera). 40 walk updates at 2.3 u/s move him 3.04 units on the floor.
    # These two run without camera collision (the camera yaw sets the walking direction).
    add_test(NAME game-script-run COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --set-variable Lobby_Dcam_Intro=1 --script "0 150 40" --no-camera-collision --screenshot game-script-run.bmp)
    set_tests_properties(game-script-run PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scripted: 40 updates, player moved 8\\.10[0-9]* units.*height 2\\.50[0-9]*.*saved game-script-run.bmp")
    add_test(NAME game-script-walk COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --set-variable Lobby_Dcam_Intro=1 --script "0 90 40" --no-camera-collision --screenshot game-script-walk.bmp)
    set_tests_properties(game-script-walk PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scripted: 40 updates, player moved 3\\.03[0-9]* units.*height 1\\.4[0-9]*.*saved game-script-walk.bmp")
    # In the police station lobby the record pose camera (docs/re/camera.md) is active, so the camera does not turn and
    # the run goes straight up the lobby steps: 8.10 units (replica output, not measured in the original).
    add_test(NAME game-script-camera COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --set-variable Lobby_Dcam_Intro=1 --script "0 150 40" --screenshot game-script-camera.bmp)
    set_tests_properties(game-script-camera PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scripted: 40 updates, player moved 8\\.10[0-9]* units.*height 2\\.50[0-9]*.*saved game-script-camera.bmp")
    # Run west off the 1.06 m platform at x 897 in the Cherry Tree Hills street: the heights are the recorded
    # fall (tools/oracle/fall-ledge.txt: 17.5898, 17.4908, 17.3423, 17.1443, 16.8968 after the platform updates).
    add_test(NAME game-script-ledge COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "899.5 17.6393 1101" --facing 249.363 --script "0 150 40" --screenshot game-script-ledge.bmp)
    set_tests_properties(game-script-ledge PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "heights: (17\\.6393 )+17\\.5898 17\\.4908 17\\.3423 17\\.1443 16\\.8968 16\\.57.*saved game-script-ledge.bmp")
    # Drive the police car of mission 1-1 (the dev options run VanBreakdown and Get_Car, which unlock it): X gets in after the 22 updates of the jump,
    # the release script of the mission holds the controls for about 9 s, A drives it 60 updates (about 24 units, oracle: 24.3 to 25.5 in 60 updates
    # from the first move, docs/re/vehicles-driving.md), X jumps out of the moved car.
    add_test(NAME game-vehicle-drive COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_VanBreakdown --call-script 1-1_Get_Car --start "886.0289 17.8243 1118.6516"
      --script "0 0 1500;0 0 3 X;0 0 330;0 0 60 A;0 0 3 X;0 0 90")
    set_tests_properties(game-vehicle-drive PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "entered at update 1523.*exiting at update 1893.*left at update 1916.*player moved 24\\.[0-9]+ units")
    # Mission 1-1 steps 7 and 8 (docs/re/vehicles-waypoints.md): 1-1_SO_VanFixed makes the van drivable; X at update 60 starts the jump in, the oracle (Azahar GDB,
    # state m11-4-van-parts, X at update 179) enters 23 updates later (202) and 1-1_Van_Waypoint_Enter starts its script 33 updates after the X (212): 83 and 93 here.
    # A short drive (60 updates of A), then the dev step "at" moves the seated van next to 1-1_Waypoint_05 (820, 18.5, 1047: in front of the waypoint, near the
    # oracle state m11-6-van-to-shop at 813.9 17.5 1046.8) instead of the 360 unit drive; the van reaches the waypoint at update 304 and 1-1_VanEscorted calls
    # VehicleJumpOut and PositionObject in one update, as the oracle's update 53 after the hit does (state 0x11b, then state 1). Update numbers are the port's.
    # The cutscene after the drive (1_1_d) shows only its cast copy of Chase: the placement after the jump out must not draw the hidden player figure again (MOVIE-31).
    add_test(NAME game-van-escort COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_SO_VanFixed --start "866.0 52.0 691.0"
      --script "0 0 60;0 0 3 X;0 0 140;0 0 60 A;at 820.0 18.5 1047.0;0 0 393;0 0 1 A")
    set_tests_properties(game-van-escort PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "jump in started at update 60.*entered at update 83.*Van_Waypoint_Enter_Script at update 93.*1-1_VanEscorted at update 305.*exiting at update 306.*left at update 306.*cutscene 1_1_d: 0 level figures drawn beside their cast copies")
    # The magnifying glass prop of cutscene 1_2_a starts at camera frame 478 and is not drawn before (MOVIE-34, user observation of the original; stand-in MOVIE-S33).
    add_test(NAME game-cutscene-glass-hidden COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-2_HouseArrival --script "0 0 40")
    set_tests_properties(game-cutscene-glass-hidden PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "cutscene 1_2_a started at update 0.*cutscene 1_2_a: 0 level figures drawn beside their cast copies, 0 models of their own drawn")
    # The police car hits the parked FP_Vehicle_03 (docs/re/vehicles-physics-collision.md, the vehicle pair contacts): the dev step "at" turns the seated
    # car to the worker's heading 10 m behind it, where the oracle state veh7-poked-trooper-behind-parked starts; A held from update 1833, the car
    # hits the worker and wakes it in update 1857 (the oracle's trooper hits it 26 updates after A from rest). Update numbers are the port's.
    # VEH-56: the seated police car is moved to the user's place (F3 overlay, 2026-10-16) and driven forward; it changes rooms with the player
    # (0x2e6ce4, docs/re/streaming-rooms.md) and so stays drawn when Room8, its placement room, is unloaded at the switch to Room21.
    # The update numbers are the port's own run.
    add_test(NAME game-vehicle-room COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_VanBreakdown --call-script 1-1_Get_Car --start "886.0289 17.8243 1118.6516"
      --script "0 0 1500;0 0 3 X;0 0 330;at 895.82 31.49 958.11 -162.4;0 0 5;0 0 60 A")
    set_tests_properties(game-vehicle-room PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "vehicle 1-1_Police_Car: room 8 -> 15 at update 1833.*vehicle 1-1_Police_Car: room 15 -> 21 at update 1879\nroom 15 -> 21 at update 1879")
    add_test(NAME game-vehicle-hit COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_VanBreakdown --call-script 1-1_Get_Car --start "886.0289 17.8243 1118.6516"
      --script "0 0 1500;0 0 3 X;0 0 330;at 936.39 17.85 1123.94 134.7;0 0 40 A;0 0 3 X;0 0 30")
    set_tests_properties(game-vehicle-hit PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "fp_vehicle_03: Bullet body, mass scale 1.*entered at update 1523.*fp_vehicle_03: moved by a contact at update 1857.*left at update 1896")
    # The seated police car is moved next to the static box at (846.0, 17.26, 1078.9) and held against it with A: the stuck timer of the driven body (0x33aea8,
    # 3 s below 3 m/s) makes 0x23019c send the type 6 message, the car breaks (0x16cccc) and comes back 62 updates later with 5 hearts, as in the oracle
    # trace of agent r18-B (break row 112, state 5 again in row 174, docs/re/vehicles-damage.md). The other update numbers are the port's.
    add_test(NAME game-vehicle-break COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_VanBreakdown --call-script 1-1_Get_Car --start "886.0289 17.8243 1118.6516"
      --script "0 0 1500;0 0 3 X;0 0 330;at 855.0 17.8 1085.0 -135;0 0 200 A")
    set_tests_properties(game-vehicle-break PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "heart lost \\(type 6, 0 m/s\\), 0 left, destroyed\nvehicle 1-1_police_car: broken at update 1965.*back with 5 hearts at update 2027")
    # The seated police car drives into the bus shelter (a wobbly prop with a box) 25 m from the oracle's phone box pass: the vehicle pass of the original sends
    # the props damage 100 and the prop is destroyed (docs/re/vehicles-physics-props.md, entry of the smash). The update number is the port's, not an oracle value.
    # The destroyed shelter starts its ExplosionParticleDef and bursts its DebrisAmount bricks (level data; docs/re/debris.md).
    add_test(NAME game-vehicle-smash COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_VanBreakdown --call-script 1-1_Get_Car --start "886.0289 17.8243 1118.6516"
      --script "0 0 1500;0 0 3 X;0 0 330;at 854.43 17.1 1087.36 134.7;0 0 60 A;0 0 3 X;0 0 30")
    set_tests_properties(game-vehicle-smash PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "prop Bus_Shelter_01: smashed by 1-1_police_car at update 1886\nprop Bus_Shelter_01: explosion particle 10, 6 bricks")
    # Mission 1-1 from the first waypoint to 1-1_MissionEnd in one run (docs/re/scripts-missions.md, entry of the first end-to-end run). The dev steps "at X Y Z"
    # stand in for the walks between the stops: onto the shop waypoint, the police car, the van waypoint, the build-it, the van, the donut box and the target;
    # one moves the seated van next to 1-1_Waypoint_05 instead of the drive there (as in game-van-escort). Provenance of the positions: waypoint, build-it and
    # van positions from the level data (tools/convert_levels.py), the target position is the one where the oracle pressed A (tests/carry_real_test.cpp).
    # The test checks the order of the scripts and cutscenes, not the update numbers the port prints (they are not oracle values).
    add_test(NAME game-mission-1-1 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --script "0 0 30;at 819 25 1072 225;0 120 80;0 0 700;at 888.6 16.58 1115.9 200;0 0 60;0 0 3 X;0 0 308;0 0 60 A;0 0 3 X;0 0 60;at 877.3 52.6 702.0 60;0 0 200;at 874.0 52.0 695.5 0;0 0 20;0 0 120 A;0 0 460;at 866.0 52.0 691.0 0;0 0 60;0 0 3 X;0 0 140;0 0 60 A;at 820.0 18.5 1047.0;0 0 393;0 0 1 A;0 0 900;at 817.9 23.94 1071.9 0;0 0 460;0 0 1 A;0 0 200;at 881.897 16.597 1120.189 0;0 0 10;0 0 1 A;0 0 1500")
    set_tests_properties(game-mission-1-1 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 900
      PASS_REGULAR_EXPRESSION "script 1-1_VanBreakdown started at update [0-9]+.*cutscene 1_1_b started at update [0-9]+.*cutscene 1_1_b ended at update [0-9]+.*script 1-1_Get_Car started at update [0-9]+.*police_car: entered at update [0-9]+.*script 1-1_Release_Car_Script ended at update [0-9]+.*script 1-1_Found_Van started at update [0-9]+.*script 1-1_SO_VanFixed started at update [0-9]+.*donutvan: entered at update [0-9]+.*1-1_Van_Waypoint_Enter_Script started at update [0-9]+.*script 1-1_VanEscorted started at update [0-9]+.*cutscene 1_1_d started at update [0-9]+.*cutscene 1_1_d ended at update [0-9]+.*script 1-1_Carry_Object_Tutorial started at update [0-9]+.*script 1-1_SO_Donuts_PickedUp started at update [0-9]+.*script 1-1_Deliver_Donuts started at update [0-9]+.*script 1-1_SO_MissionEnd started at update [0-9]+.*cutscene 1_1_e started at update [0-9]+.*cutscene 1_1_e ended at update [0-9]+.*script 1-1_MissionEnd started at update [0-9]+.*script 1-1_MissionEnd ended at update [0-9]+.*StoryProgress 2")
    # Mission 1-2 from the second detective point (docs/re/abilities-detective.md): 1-2_SO_IntroCamOff enables 1-2_Dtv_DogSpot_02, A on the point
    # (oracle state m12-7-detective-waypoint-2) enters detective mode, the X spot 1-2_Dtv_DogPath_XSpot01 ahead within 2.5 ends it, A on the X starts
    # the use state; its event (43 updates), the reveal time (32), RevealDelay (30) and the flight (18) lead to 1-2_SO_Dspot_02_X, the oracle's
    # intervals of states m12-4..m12-6. Teleports stand in for the walks.
    add_test(NAME game-detective-1-2 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "723.675 29.768 630.775" --facing 291.7 --call-script 1-2_SO_IntroCamOff
      --script "0 0 160;at 720.021 37.035 654.390 289.6;0 0 1 A;0 0 40;at 713.06 37.06 654.62 289.6;0 0 25;at 711.6 37.06 655.1 289.6;0 0 1 A;0 0 130")
    set_tests_properties(game-detective-1-2 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "detective: player state 1 at update 160.*1-2_dtv_dogspot_02 state 1 at update 161.*player state 0 at update 197.*1-2_dtv_dogspot_02 state 2 at update 202.*player state 2 at update 202.*player state 3 at update 226.*1-2_dtv_dogpath_xspot01 state 1 at update 270.*1-2_dtv_dogpath_xspot01 state 2 at update 302.*1-2_trapdoor_03 state 2 at update 333.*script 1-2_SO_Dspot_02_X started at update 351")
    # Mission 1-2, the dog cage (SCRIPT-111): 1-2_Pringles_Cage_Locked enables the use spot 1-2_Dog_Cage_Use_Spot, A on the user's trigger position
    # (oracle state m12-13-cage-trigger) fires 1-2_Cage_Key_Target_Script through the spot's TriggerObject.
    add_test(NAME game-use-spot-1-2 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "916.702 64.431 663.724" --facing 0 --call-script 1-2_Pringles_Cage_Locked
      --script "0 0 5;0 0 1 A;0 0 60")
    set_tests_properties(game-use-spot-1-2 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "use spot: 1-2_dog_cage_use_spot triggers 1-2_cage_key_target_script at update [0-9]+.*script 1-2_Cage_Key_Target_Script started at update [0-9]+")
    # Mission 1-2, the alley (DETECT-2): the Doorlock script 1-2_Spawn_Doorlock (the waypoint's TriggerScript) passes WaitForCurrentCharAnim on
    # 1-2_Doorlock_Holmes because PlayCharAnim leaves the idle state (state 0x50, docs/re/abilities-detective.md), ends, and then A on the first
    # detective point (oracle state m12-4-detective-point, the teleport stands in for the walk) switches the detective tool on and enables
    # 1-2_Dtv_DogSpot_01; the script's PositionObject moves Holmes to 1-2_Doorlock_Teleport (SCRIPT-287). The update numbers are the port's, not oracle values.
    add_test(NAME game-doorlock-detective-1-2 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "738.819 29.389 631.241" --facing 270 --call-script 1-2_Spawn_Doorlock
      --script "0 0 1100;at 723.675 29.768 630.775 291.7;0 0 2;0 0 1 A;0 0 40")
    set_tests_properties(game-doorlock-detective-1-2 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "PositionObject: 1-2_doorlock_holmes to 1-2_doorlock_teleport at update [0-9]+.*script 1-2_Spawn_Doorlock ended at update [0-9]+.*detective: player state 1 at update [0-9]+.*1-2_dtv_dogspot_01 state 1 at update [0-9]+")
    # The new-game start of the level (--no-start-cutscenes absent): 1-1_MissionStart plays the intro movie 01_GameIntro, then the in-engine cutscene 1_1_a
    # (docs/re/scripts-missions.md, 1-1_MissionStart order), then ends; this is also the order of MOVIE-23.
    add_test(NAME game-mission-1-1-start COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
      --script "0 0 2600")
    set_tests_properties(game-mission-1-1-start PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 900
      PASS_REGULAR_EXPRESSION "movie 01_GameIntro started at update 0.*movie 01_GameIntro ended at update 2066.*cutscene 1_1_a started at update 2066.*cutscene 1_1_a ended at update 2506.*script 1-1_MissionStart ended at update 2537")
    # Walk into Police_Station_Transition_Lower after 1-1_MissionSkip enabled it (the dev option --call-script): the script
    # Police_ExtToInt opens WarningDialogBox, A answers it and PositionObject puts the player on
    # Enter_Police_Station_Position_Lower (yaw 45.4542 deg = heading 8275, height 16.5773 as in the oracle); the AIMove that follows
    # turns the player along the nav path to the goal cell centre (910.81, 1119.46; oracle +0x58 of the path record, docs/re/ai-navigation.md; heading 8103) and walks it 1.44903 units in 6 updates (speed: stand-in SCRIPT-S31).
    add_test(NAME game-script-dialog COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_MissionSkip --start "904.2 16.58 1112.4" --facing 45.45
      --script "0 0 10;0 145 16;0 0 14;0 0 1 A;0 0 6" --screenshot game-script-dialog.bmp)
    set_tests_properties(game-script-dialog PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "script dialog opened at update 23.*script dialog answered at update 40.*player moved 1\\.44903 units, facing 8103, .*height 16\\.5773")
    # Lobby exit and station entry at run time: the walk-in bound fires SceneChange_LobbyToCherryTree (update 23), A on the
    # loading page (update 36) and the switch 4 updates later; the street start is the point LobbyToCherryTree_P1 (yaw
    # 249.25 deg = heading -20161, height 16.5773, oracle traces of agent H, docs/re/streaming.md). The entry runs the script
    # Police_ExtToInt (request at update 164: 123 updates after the director camera starts, as in the oracle trace of docs/re/dcam.md) and lands at Lobby_Dest_CherryTree_P1 (height 2.0901).
    add_test(NAME game-scene-change-exit COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --set-variable Lobby_Dcam_Intro=1
      --level levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl
      --start "-1.97 1.436 14.843" --facing 0
      --script "0 -145 30;0 0 6;0 0 1 A;0 0 10" --screenshot game-scene-change-exit.bmp)
    set_tests_properties(game-scene-change-exit PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scene change requested by SceneChange_LobbyToCherryTree at update 23.*scene change to WorldLevel_01_CherryTreeHills after 41 updates.*facing -20161, .*height 16\\.5773")
    add_test(NAME game-scene-change-entry COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_MissionSkip --start "904.2 16.58 1112.4" --facing 45.45
      --script "0 0 10;0 145 16;0 0 14;0 0 1 A;0 0 150;0 0 1 A;0 0 6" --screenshot game-scene-change-entry.bmp)
    set_tests_properties(game-scene-change-entry PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "scene change requested by SceneChange_Police_ExtToInt at update 164.*scene change to WorldLevel_00_PoliceStation after 196 updates.*facing 27729, .*height 2\\.0901")
    # Getting into the police car of mission 1-1 starts 1-1_Release_Car_Script, whose director cameras hold the player's controls
    # (docs/re/dcam.md, tools/oracle/dcam-hold-car.txt): in the oracle the hold lasts from the update after StartDCam to the update
    # after the end test, 278 updates after Drive began. X at update 763 and 1022 does nothing, X at 1023 (278 updates after Drive
    # began at 745) lets the player out.
    set(DIRECTOR_HOLD_ARGS --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_VanBreakdown --call-script 1-1_Get_Car --start "886.681 16.590 1112.930" --facing 16)
    add_test(NAME game-director-hold COMMAND chase-game ${DIRECTOR_HOLD_ARGS}
      --script "0 0 700;0 0 5;0 150 12;0 0 5;0 0 1 X;0 0 40;0 0 1 X;0 0 258;0 0 1 X;0 0 40")
    set_tests_properties(game-director-hold PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "entered at update 745.*scripted: 1063 updates"
      FAIL_REGULAR_EXPRESSION "exiting at update")
    add_test(NAME game-director-release COMMAND chase-game ${DIRECTOR_HOLD_ARGS}
      --script "0 0 700;0 0 5;0 150 12;0 0 5;0 0 1 X;0 0 40;0 0 1 X;0 0 259;0 0 1 X;0 0 30")
    set_tests_properties(game-director-release PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "entered at update 745.*exiting at update 1023.*left at update 1046"
      FAIL_REGULAR_EXPRESSION "exiting at update [0-9]{1,3}[^0-9]|exiting at update 10[01][0-9][^0-9]|exiting at update 102[0-2][^0-9]")
    # Zipline of mission 1-2: the player stands where the oracle state m12-9-zipline has him, jumps (B) and grabs 1-8_Zip_Line05 in the
    # 7th jump update (update 8, as the recorded ride, docs/re/player-traversal.md); the loop sound 107 starts with the grab. The ride lasts
    # 2 updates longer than the recorded one because the script step is 0.033 s (the rate of 0x275000 scales with dt squared): the dismount
    # starts at update 110 and state 5 at update 140.
    add_test(NAME game-script-zipline COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "870.226 66.001 630.961" --facing 102.8 --script "0 0 2;0 0 1 B;0 0 140")
    set_tests_properties(game-script-zipline PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "zipline: grab 1-8_zip_line05 at update 8.*zipline: fall at update 140.*sounds: 2:288 8:107")
    # The use press at the high anchor is the A press (0x2011b0 -> 0x1fef10 with pressed 1; 0x1b6664 needs it, TRAV-12): standing 0.19 before
    # Mission 1-2: Chase where the user's state m12-table-before has him (oracle position and facing ctrl+8 = -936), 2.06 m south of
    # DonutTable_14. A Y tap (held one update) enters the attack state 0xa3 on release, message 62 goes to the table at once, the hit
    # (message 0, damage 1) in the 5th update of the state and the state ends after 14 updates (oracle runs atap / ataphit,
    # docs/re/player-attack.md).
    add_test(NAME game-script-attack-table COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "715.44196 32.71405 641.61597" --facing -5.1416 --script "0 0 2;0 0 1 Y;0 0 20")
    set_tests_properties(game-script-attack-table PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "attack: message 62 to donuttable_14 at update 3.*attack: hit donuttable_14 \\(message 0, damage 1\\) at update 7.*attack: ended at update 17")
    # Five Y taps (15 updates apart) from the same spot: every hit lowers the table's WobbleHealth 5 by the damage 1 (oracle: 4, 3, 2, 1, 0 after the five
    # punches), the fifth destroys DonutTable_14 and DonutTable_14_Script runs in the update of the destroy (docs/re/props-wobbly-bouncer.md).
    add_test(NAME game-script-wobbly-table COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "715.44196 32.71405 641.61597" --facing -5.1416 --script "0 0 2;0 0 1 Y;0 0 15;0 0 1 Y;0 0 15;0 0 1 Y;0 0 15;0 0 1 Y;0 0 15;0 0 1 Y;0 0 30")
    set_tests_properties(game-script-wobbly-table PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "health 4 at update 7.*health 3 at update 23.*health 2 at update 39.*health 1 at update 55.*donuttable_14: destroyed at update 71.*script DonutTable_14_Script started at update 71")
    # B on the built trampoline (--call-script runs the script of the finished build): the landing launches at the lowest take-off speed 0.3 every 13
    # updates (oracle state m12-trampoline-built: take-offs 13 updates apart, speed 0.3 without B held).
    add_test(NAME game-script-bounce COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script Trampoline_03_Buildits_Script --start "720.2 32.62 637.47" --facing 0 --script "0 0 3;0 0 2 B;0 0 60")
    set_tests_properties(game-script-bounce PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "bounce: trampoline_03 launches at 0.3 at update 22.*at update 35.*at update 48.*at update 61")
    # 1-8_Zip_Line05 on its platform, facing it, A gets on and B does not. The spot is derived from the level data (anchor 868.094 69.992 630.977,
    # UseBound), not an oracle position (TRAV-8).
    add_test(NAME game-script-zipline-use-a COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "867.9 69.95 630.98" --facing 90 --script "0 0 2;0 0 1 A;0 0 20")
    set_tests_properties(game-script-zipline-use-a PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "zipline: use 1-8_zip_line05 at update 2")
    add_test(NAME game-script-zipline-use-b COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "867.9 69.95 630.98" --facing 90 --script "0 0 2;0 0 1 B;0 0 20")
    set_tests_properties(game-script-zipline-use-b PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "player ended at" FAIL_REGULAR_EXPRESSION "zipline: use")
    # Climbing bricks of mission 1-2: the player stands under 1-2_Brick_Grab_08 where the oracle run `drop` from m12-11-brick-climb landed,
    # jumps (B) and grabs 08 in the 7th jump update (state 141, as the zipline grab), hangs (145 after the 22 updates of brickgrab_grab),
    # reaches up with the stick (146), jumps up to 07 with B (149, 16 updates), grabs it (142, 14 updates), and B on 07, which has no
    # adjacent brick, starts the jump state 4 (docs/re/player-traversal.md).
    add_test(NAME game-script-brick-grab COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "908.8964 64.4281 673.1982" --facing 121.998 --script "0 0 2;0 0 1 B;0 0 40;0 120 2;0 120 1 B;0 120 35;0 120 1 B;0 120 3")
    set_tests_properties(game-script-brick-grab PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "state 141 1-2_brick_grab_08 at update 8\n.*state 145 1-2_brick_grab_08 at update 30\n.*state 146 1-2_brick_grab_08 at update 43\n.*state 149 1-2_brick_grab_07 at update 45\n.*state 142 1-2_brick_grab_07 at update 61\n.*state 145 1-2_brick_grab_07 at update 75\n.*jump at update 81")
    # Mission 1-2, the jump off the last climbing brick onto the rail (TRAV-30): B on 1-2_Brick_Grab_07 (no neighbour) starts the
    # normal jump and the rail 1-2_ClimbBar_02 is grabbed 7 updates later, as in the oracle (m12-11: B in update 37, state 88 in 44).
    add_test(NAME game-script-brick-rail COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "908.8964 64.4281 673.1982" --facing 121.998 --script "0 0 2;0 0 1 B;0 0 40;0 120 2;0 120 1 B;0 120 35;0 120 1 B;0 120 10")
    set_tests_properties(game-script-brick-rail PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "brick grab: jump at update 81\n.*climb bar: grab 1-2_climbbar_02 at update 88")
    # Drain pipe of mission 1-2: the player stands where the oracle state m12-10a-pole-foot has him (0.77 m from 1-5_Drain_Pipe05, facing
    # away), the stick points at the pipe (the stick value stands for the camera yaw of this runtime) and he mounts it by walking into it in
    # update 5 as in the recorded walk (mount in the 3rd walking update); the A press in update 15 drops him (state 5).
    add_test(NAME game-script-pole COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "941.112 58.336 683.057" --facing 116.9 --script "0 0 2;69 -86 8;0 0 5;0 0 1 A;0 0 20")
    set_tests_properties(game-script-pole PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "pole: walk-in 1-5_drain_pipe05 at update 5.*pole: fall at update 15"
      FAIL_REGULAR_EXPRESSION "pole: fall at update 1[0-4][^0-9]")
    # Standing jump in the Cherry Tree Hills street (B button for one update): the heights are the recorded rise
    # (tools/oracle/jump-ground.txt, jstand: 16.8593, 17.1297, 17.4068, 17.6840, ... to the apex 18.6815).
    add_test(NAME game-script-jump COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "886.764 16.589 1111.166" --script "0 0 2;0 0 1 B;0 0 12" --screenshot game-script-jump.bmp)
    set_tests_properties(game-script-jump PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "heights: 16\\.589 16\\.589 16\\.589 16\\.859[0-9]* 17\\.129[0-9]* 17\\.40[67][0-9]* 17\\.68[34][0-9]* 17\\.92[89][0-9]* 18\\.17[0-9]* 18\\.34[0-9]* 18\\.52[0-9]* 18\\.5[89][0-9]* 18\\.65[0-9]* 18\\.66[0-9]* 18\\.68[0-9]*.*saved game-script-jump.bmp")
    # The display during play: the player stands on a silver stud (tools/oracle/hud-bar.txt); after 70 updates the stud has flown to
    # the counter, the total is 10 and the HUD shows it (four hearts: the class attribute Health defaults to 4).
    add_test(NAME game-script-hud COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "862.5931 16.1988 1060.5875" --window 400x480 --script "0 0 70" --screenshot game-script-hud.bmp)
    set_tests_properties(game-script-hud PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "studs: counter 10, 0 collecting, 1 collected.*hud: 4 hearts, stud display 10, bar state 2.*saved game-script-hud.bmp")
    # Room streaming in the level: starting in Room7 (position and view of the oracle state street-at-entrance +1 update, 9 rooms
    # loaded), walking east to west enters Room8, Room6 unloads in that update and Room16 finishes loading 9 updates later
    # (the update numbers are the port's own walk, the rule is pinned by room-streaming-real-data). Room16 finishes loading with its
    # nearest box point 120.0 from the camera (oracle street-at-entrance, same walk: 120.3, docs/re/rendering-fog.md RENDER-31).
    add_test(NAME game-room-streaming COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "903.71875 16.573267 1113.328247" --facing 11.3 --window 400x480 --script "20 150 70")
    set_tests_properties(game-room-streaming PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "room streaming: 43 rooms, start Room7, 9 loaded.*room 7 -> 8 at update 54.*rooms loaded: 8 at update 54.*rooms loaded: 9 at update 63.*room 16 loaded at update 63: nearest box point 120\\.0")
    # 1-2_Cage_Key_Target_Script puts the player into Room40 (update 106), StreamRoomIn("Room40") waits for the 9-update load (update 115,
    # WSTREAM-S30) and the 1.5 s wait after it ends at update 162, where cutscene 1_2_c starts (replica output, not measured in the original).
    add_test(NAME game-stream-room-in COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-2_Cage_Key_Target_Script --script "0 0 200")
    set_tests_properties(game-stream-room-in PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "room 32 -> 40 at update 106.*room 40 loaded at update 115.*cutscene 1_2_c started at update 162")
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

add_executable(prop-wobbly-test tests/prop_wobbly_test.cpp)
target_link_libraries(prop-wobbly-test PRIVATE chase_runtime chase_assets)
target_compile_options(prop-wobbly-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME prop-wobbly COMMAND prop-wobbly-test)
add_test(NAME prop-wobbly-real-data COMMAND prop-wobbly-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(prop-wobbly-real-data PROPERTIES SKIP_RETURN_CODE 77)
add_executable(prop-bouncer-test tests/prop_bouncer_test.cpp)
target_link_libraries(prop-bouncer-test PRIVATE chase_runtime chase_assets)
target_compile_options(prop-bouncer-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME prop-bouncer COMMAND prop-bouncer-test)
add_test(NAME prop-bouncer-real-data COMMAND prop-bouncer-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(prop-bouncer-real-data PROPERTIES SKIP_RETURN_CODE 77)

add_executable(camera-lock-census tools/camera_lock_census.cpp)
target_link_libraries(camera-lock-census PRIVATE chase_runtime chase_assets)
target_compile_options(camera-lock-census PRIVATE -Wall -Wextra -Wpedantic)

foreach(slow_test game-doorlock-detective-1-2 game-mission-1-1 game-mission-1-1-start game-vehicle-drive game-vehicle-room game-vehicle-hit game-vehicle-smash game-vehicle-break game-director-hold game-director-release game-van-escort waypoint-marker-shot audio-real-data)
  if(TEST ${slow_test})
    set_property(TEST ${slow_test} APPEND PROPERTY LABELS slow)
  endif()
endforeach()
