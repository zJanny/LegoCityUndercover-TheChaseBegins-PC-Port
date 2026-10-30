cmake_minimum_required(VERSION 3.16)
project(chase_input_foundation LANGUAGES CXX)
include(cmake/ccache.cmake)
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
add_library(chase_pad_events src/runtime/pad_events.cpp)
target_include_directories(chase_pad_events PUBLIC src)
target_link_libraries(chase_pad_events PUBLIC chase_input)
target_compile_options(chase_pad_events PRIVATE -Wall -Wextra -Wpedantic)
add_executable(chase-input-lab src/main.cpp)
target_link_libraries(chase-input-lab PRIVATE chase_input chase_pad_events PkgConfig::SDL2)
target_compile_options(chase-input-lab PRIVATE -Wall -Wextra -Wpedantic)
configure_file(config/controls.cfg config/controls.cfg COPYONLY)
configure_file(config/controls.default.cfg config/controls.default.cfg COPYONLY)
file(WRITE ${CMAKE_BINARY_DIR}/config/qol.cfg "# Test window: the size the screenshot tests were recorded with (the default is fullscreen at the desktop size).\nwindow_mode windowed\nresolution 960x600\n")
# Native asset runtime (plan S4a): no SDL dependency.
add_library(chase_assets src/assets/fuse.cpp src/assets/pooled.cpp src/assets/texture.cpp src/assets/model.cpp src/assets/material.cpp src/assets/level.cpp src/assets/object_collision.cpp src/assets/audio.cpp src/assets/anim.cpp src/assets/nav.cpp src/assets/text.cpp src/assets/cutscene.cpp src/assets/table.cpp src/assets/config_text.cpp src/assets/pregen.cpp src/assets/pregen_traffic.cpp src/assets/xaml.cpp src/assets/subtitles.cpp src/assets/particles.cpp)
target_include_directories(chase_assets PUBLIC src)
target_compile_options(chase_assets PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(chase_assets PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_sources(chase_assets PRIVATE src/assets/object_classes.cpp src/assets/file_decoders.cpp)
# C ABI over chase_assets for the Python tools (tools/chase_native.py).
add_library(chase_assets_c SHARED src/assets/c_api.cpp)
target_link_libraries(chase_assets_c PRIVATE chase_assets chase_runtime)
target_compile_options(chase_assets_c PRIVATE -Wall -Wextra -Wpedantic)
# Audio mixer core (plan S4d): no SDL dependency; chase-soundtest is the SDL front end.
add_library(chase_audio src/audio/mixer.cpp src/audio/spatial.cpp src/audio/soundplayer.cpp src/audio/effectvoices.cpp src/audio/musicplayer.cpp src/audio/voice_params.cpp)
target_link_libraries(chase_audio PUBLIC chase_assets)
target_compile_options(chase_audio PRIVATE -Wall -Wextra -Wpedantic)
# Save image (plan S4f): no SDL dependency.
add_library(chase_save src/save/save.cpp)
target_include_directories(chase_save PUBLIC src)
target_compile_options(chase_save PRIVATE -Wall -Wextra -Wpedantic)
add_library(chase_save_tables src/save/completion_tables.cpp)
target_link_libraries(chase_save_tables PUBLIC chase_save chase_assets)
target_compile_options(chase_save_tables PRIVATE -Wall -Wextra -Wpedantic)
add_library(chase_save_progress src/save/slot_progress.cpp src/save/continue_start.cpp src/save/save_writer.cpp src/save/new_game.cpp src/save/level_state.cpp)
target_link_libraries(chase_save_progress PUBLIC chase_save chase_assets)
target_compile_options(chase_save_progress PRIVATE -Wall -Wextra -Wpedantic)
# Game runtime core (plan S4e): recovered frame logic, no SDL dependency.
add_library(chase_runtime src/runtime/scheduler.cpp src/runtime/pad.cpp src/runtime/player_intent.cpp src/runtime/player_locomotion.cpp src/runtime/follow_camera.cpp src/runtime/collision.cpp src/runtime/camera_sweep.cpp src/runtime/camera_reset_probe.cpp src/runtime/camera_near_plane.cpp src/runtime/player_vertical.cpp src/runtime/player_motion.cpp src/runtime/camera_records.cpp src/runtime/camera_blocker.cpp src/runtime/camera_rig.cpp src/runtime/camera_lock_icon.cpp src/runtime/cutscene_camera.cpp)
target_link_libraries(chase_runtime PUBLIC chase_assets)
target_include_directories(chase_runtime PUBLIC src)
target_compile_options(chase_runtime PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
set_target_properties(chase_runtime PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_sources(chase_runtime PRIVATE src/runtime/pregen_derive.cpp src/runtime/studs.cpp src/runtime/sound_events.cpp src/runtime/music.cpp src/runtime/level_start.cpp src/runtime/script_runner.cpp src/runtime/natives_objects.cpp src/runtime/natives_effects.cpp src/runtime/natives_world.cpp src/runtime/natives_party.cpp src/runtime/script_runner_natives.cpp src/runtime/natives_animation.cpp src/runtime/natives_cutscene.cpp src/runtime/screen_fade.cpp src/runtime/dialog_bg_fade.cpp src/runtime/natives_progress.cpp src/runtime/natives_music.cpp src/runtime/natives_sound.cpp src/runtime/entity_ids.cpp src/runtime/music_block.cpp src/runtime/door.cpp src/runtime/scene_change.cpp src/runtime/same_level_entry.cpp src/runtime/room_streaming.cpp src/runtime/room_streamer.cpp src/runtime/scene_start.cpp src/runtime/dcam.cpp src/runtime/debug_metrics.cpp src/runtime/object_visibility.cpp src/runtime/script_text.cpp src/runtime/script_triggers.cpp src/runtime/objectives.cpp src/runtime/sub_objectives.cpp src/runtime/ai.cpp src/runtime/ai_simple_path.cpp src/runtime/fog.cpp src/runtime/lighting.cpp src/runtime/sky.cpp src/runtime/script_placement.cpp src/runtime/ai_go_to_target.cpp src/runtime/nav_grid.cpp src/runtime/nav_path.cpp src/runtime/nav_follow.cpp src/runtime/nav_load.cpp src/runtime/room_locator.cpp src/runtime/object_rooms.cpp src/runtime/pedestrian_rooms.cpp src/runtime/player_controller_state.cpp src/runtime/ai_super_jump.cpp src/runtime/ai_face_target.cpp src/runtime/waypoints.cpp src/runtime/waypoint_global.cpp src/runtime/buildit.cpp src/runtime/buildit_set.cpp src/runtime/buildit_wobble.cpp src/runtime/prop_animation.cpp src/runtime/prop_root_motion.cpp src/runtime/prop_nodes.cpp src/runtime/buildit_bricks.cpp src/runtime/mesh_bouncer.cpp src/runtime/reference_trigger.cpp src/runtime/vehicle.cpp src/runtime/vehicle_boarding.cpp src/runtime/vehicle_drive.cpp src/runtime/vehicle_body.cpp src/runtime/vehicle_wheels.cpp src/runtime/vehicle_drivetrain.cpp src/runtime/vehicle_wheel_pose.cpp src/runtime/vehicle_hud.cpp src/runtime/vehicle_damage.cpp src/runtime/vehicle_break.cpp src/runtime/head_icon.cpp src/runtime/floor_walk.cpp src/runtime/vehicle_collision.cpp src/runtime/vehicle_box_contact.cpp src/runtime/carry.cpp src/runtime/carry_arrows.cpp src/runtime/carry_animation.cpp src/runtime/scanner.cpp src/runtime/cutscene_cast.cpp src/runtime/cutscene_hold.cpp src/runtime/collision_enable.cpp src/runtime/game_random.cpp src/runtime/npc_idle.cpp src/runtime/animation_slots.cpp src/runtime/player_script_animation.cpp src/runtime/director_hold.cpp src/runtime/script_thread_log.cpp src/runtime/vehicle_world.cpp src/runtime/vehicle_sphere_sweep.cpp src/runtime/buildit_player.cpp src/runtime/vehicle_prop_smash.cpp src/runtime/detective.cpp src/runtime/hand_item.cpp src/runtime/hand_item_set.cpp src/runtime/use_spot.cpp src/runtime/use_marker.cpp src/runtime/use_marker_ground.cpp src/runtime/ability_table.cpp src/runtime/zipline.cpp src/runtime/zipline_rope.cpp src/runtime/zipline_pole.cpp src/runtime/nested_props.cpp src/runtime/brick_grab.cpp src/runtime/climb_bar.cpp src/runtime/acrobat_pole.cpp src/runtime/pole.cpp src/runtime/ladder.cpp src/runtime/ladder_sounds.cpp src/runtime/fence_vault.cpp src/runtime/cat_rescue.cpp src/runtime/screen_shake.cpp src/runtime/placement_message.cpp src/runtime/super_buildit.cpp src/runtime/superbuildit_messages.cpp src/runtime/super_brick_display.cpp src/runtime/fast_travel_pad.cpp src/runtime/map_buttons.cpp src/runtime/update_order.cpp src/runtime/save_hud_counter.cpp)
target_sources(chase_runtime PRIVATE src/runtime/shadow.cpp)
target_sources(chase_runtime PRIVATE src/runtime/award_viewer.cpp)
target_sources(chase_runtime PRIVATE src/runtime/reference_trigger_level.cpp src/runtime/reference_trigger_apply.cpp)
target_sources(chase_runtime PRIVATE src/runtime/switch_base.cpp src/runtime/switch_set.cpp src/runtime/lever_use.cpp)
target_sources(chase_runtime PRIVATE src/runtime/player_attack.cpp src/runtime/attack_sounds.cpp)
target_sources(chase_runtime PRIVATE src/runtime/pedestrian_walk.cpp)
target_sources(chase_runtime PRIVATE src/runtime/vehicle_camera.cpp src/runtime/particles.cpp src/runtime/debris.cpp src/runtime/prop_destroy.cpp src/runtime/destroy_message.cpp src/runtime/character_death.cpp src/runtime/fall_apart.cpp src/runtime/stud_drop.cpp src/runtime/character_respawn.cpp)
target_sources(chase_runtime PRIVATE src/runtime/prop_wobbly_destructible.cpp src/runtime/prop_bouncer.cpp src/runtime/stud_callers.cpp src/runtime/stud_drop_classes.cpp src/runtime/super_brick.cpp src/runtime/prop_hit_sequence.cpp)
target_sources(chase_runtime PRIVATE src/runtime/prop_rocking.cpp src/runtime/platform_wait.cpp src/runtime/game_generator_natives_glue.cpp
  src/runtime/platform_node_move.cpp src/runtime/platform_path.cpp src/runtime/game_platform_glue.cpp)
target_sources(chase_runtime PRIVATE src/runtime/platform_riding.cpp)
target_sources(chase_runtime PRIVATE src/runtime/node_attach.cpp src/runtime/attach_children.cpp)
target_sources(chase_runtime PRIVATE src/runtime/script_particles.cpp src/runtime/game_mission_end_1_3_glue.cpp)
target_sources(chase_runtime PRIVATE src/runtime/pushable.cpp src/runtime/mission13_end.cpp)
target_sources(chase_runtime PRIVATE src/runtime/pedestrian_manager.cpp)
target_sources(chase_runtime PRIVATE src/runtime/pedestrian_blocked.cpp src/runtime/room_layers.cpp)
target_sources(chase_runtime PRIVATE src/runtime/pedestrian_system.cpp)
target_sources(chase_runtime PRIVATE src/runtime/options_page.cpp src/runtime/languages_page.cpp src/runtime/qol_game_options.cpp)
target_sources(chase_runtime PRIVATE src/runtime/debris_pickup.cpp src/runtime/hud_pending_add.cpp)
target_sources(chase_runtime PRIVATE src/runtime/node_fade.cpp src/runtime/pedestrian_fade.cpp src/runtime/semi_trans_near_camera.cpp)
add_library(chase_draw_order src/render/draw_order.cpp)
target_include_directories(chase_draw_order PUBLIC src)
target_compile_options(chase_draw_order PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
# Front end (boot to the slot list): CPU-drawn screens, no SDL dependency.
add_library(chase_frontend src/frontend/script.cpp src/frontend/frontend.cpp src/frontend/pages.cpp src/frontend/canvas.cpp src/frontend/resources.cpp src/frontend/layout.cpp src/frontend/flow.cpp src/frontend/hud.cpp src/frontend/hud_parts.cpp src/frontend/super_brick_display.cpp src/frontend/script_text_pages.cpp src/frontend/minimap.cpp src/frontend/map_buttons_view.cpp src/frontend/map_marker.cpp src/frontend/map_file.cpp src/frontend/map_hub.cpp src/frontend/slot_summaries.cpp src/frontend/super_build_prompt.cpp src/frontend/round_menu.cpp src/frontend/pause_menu.cpp src/frontend/award_viewer.cpp)
target_sources(chase_frontend PRIVATE src/frontend/scanner_overlay.cpp)
target_sources(chase_runtime PRIVATE src/runtime/scanner_reticle.cpp src/runtime/scanner_markers.cpp src/runtime/scanner_info.cpp src/runtime/scanner_bottom.cpp src/runtime/scanner_render.cpp src/runtime/glow.cpp src/runtime/reflection.cpp)
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
add_executable(chase-layoutdump src/layoutdump.cpp)
target_link_libraries(chase-layoutdump PRIVATE chase_assets)
target_compile_options(chase-layoutdump PRIVATE -Wall -Wextra -Wpedantic)
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
  set(CHASE_SHADER_SOURCES sceneVertex:scene.vert.glsl sceneFragment:scene.frag.glsl hudVertex:hud.vert.glsl hudFragment:hud.frag.glsl particleVertex:particle.vert.glsl particleFragment:particle.frag.glsl glowVertex:glow.vert.glsl glowFragment:glow.frag.glsl shadowVertex:shadow.vert.glsl shadowFragment:shadow.frag.glsl)
  set(CHASE_SHADERS_CPP ${CMAKE_BINARY_DIR}/generated/shaders.cpp)
  add_custom_command(OUTPUT ${CHASE_SHADERS_CPP}
    COMMAND ${CMAKE_COMMAND} "-DSHADERS=${CHASE_SHADER_SOURCES}" -DSHADER_DIR=${CHASE_SHADER_DIR} -DOUTPUT=${CHASE_SHADERS_CPP}
      -P ${CMAKE_SOURCE_DIR}/tools/embed_shaders.cmake
    DEPENDS ${CHASE_SHADER_DIR}/scene.vert.glsl ${CHASE_SHADER_DIR}/scene.frag.glsl ${CHASE_SHADER_DIR}/hud.vert.glsl ${CHASE_SHADER_DIR}/hud.frag.glsl
      ${CHASE_SHADER_DIR}/particle.vert.glsl ${CHASE_SHADER_DIR}/particle.frag.glsl ${CHASE_SHADER_DIR}/glow.vert.glsl ${CHASE_SHADER_DIR}/glow.frag.glsl
      ${CHASE_SHADER_DIR}/shadow.vert.glsl ${CHASE_SHADER_DIR}/shadow.frag.glsl
      ${CMAKE_SOURCE_DIR}/tools/embed_shaders.cmake
    VERBATIM)
  add_library(chase_render src/render/scene.cpp src/render/character.cpp ${CHASE_SHADERS_CPP})
  target_include_directories(chase_render PUBLIC ${CMAKE_SOURCE_DIR}/src)
  target_link_libraries(chase_render PUBLIC chase_assets chase_runtime chase_draw_order PkgConfig::SDL2 PkgConfig::EPOXY)
  target_compile_options(chase_render PRIVATE -Wall -Wextra -Wpedantic)
  target_sources(chase_render PRIVATE src/render/studs.cpp src/render/debug_text.cpp src/render/debug_overlay.cpp
    src/render/program.cpp src/render/particles.cpp src/render/rope.cpp src/render/window.cpp src/render/hud_overlay.cpp src/render/overlay_quad.cpp src/render/front_presenter.cpp src/render/frame_stats.cpp src/render/glow.cpp src/render/reflection.cpp src/render/view_region.cpp)
  target_sources(chase_render PRIVATE src/render/shadow.cpp)
  add_executable(render-viewport-rect-test tests/render_viewport_rect_test.cpp)
  target_link_libraries(render-viewport-rect-test PRIVATE chase_render)
  target_compile_options(render-viewport-rect-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME render-viewport-rect COMMAND render-viewport-rect-test)
  add_executable(render-overlay-quad-test tests/render_overlay_quad_test.cpp)
  target_link_libraries(render-overlay-quad-test PRIVATE chase_render)
  target_compile_options(render-overlay-quad-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME render-overlay-quad COMMAND render-overlay-quad-test)
  add_executable(chase-viewer src/viewer.cpp)
  target_link_libraries(chase-viewer PRIVATE chase_render chase_audio chase_input chase_pad_events)
  target_compile_options(chase-viewer PRIVATE -Wall -Wextra -Wpedantic)
  if(FFMPEG_FOUND)
    add_executable(chase-game src/game.cpp src/game_options.cpp src/game_player.cpp src/game_carry.cpp src/game_scanner.cpp src/game_doors.cpp src/game_prop_parts.cpp src/game_level.cpp src/game_pause_menu.cpp src/game_award_viewer.cpp src/game_level_update.cpp src/game_vehicle.cpp src/game_vehicle_break.cpp src/game_vehicle_place.cpp src/game_vehicle_camera.cpp src/game_level_rooms.cpp src/game_cutscene.cpp src/game_level_cutscene.cpp src/game_characters.cpp src/game_pedestrians.cpp src/game_pedestrian_fade.cpp src/game_semi_trans.cpp src/game_pedestrian_rooms.cpp src/game_particles.cpp src/game_level_natives.cpp src/game_detective.cpp src/game_use_spot.cpp src/game_objective_markers.cpp src/game_hand_items.cpp src/game_effects.cpp src/game_shadows.cpp src/game_vehicle_effects.cpp src/game_zipline.cpp src/game_brick_grab.cpp src/game_climb_bar.cpp src/game_acrobat_pole.cpp src/game_pole.cpp src/game_ladder.cpp src/game_fence_vault.cpp src/game_animals.cpp src/game_object_visibility.cpp src/game_nested_props.cpp src/game_super_buildit.cpp src/game_fast_travel.cpp src/game_key_pickup.cpp)
    target_sources(chase-game PRIVATE src/game_pedestrian_blocking.cpp src/game_traffic.cpp)
    target_sources(chase-game PRIVATE src/game_props_destructible.cpp src/game_prop_destroy.cpp src/game_character_death.cpp src/game_fall_apart.cpp src/game_stud_drops.cpp src/game_super_bricks.cpp)
    target_sources(chase-game PRIVATE src/game_debris_pickup.cpp)
    target_sources(chase-game PRIVATE src/game_switch_glue.cpp src/game_switch_use_glue.cpp)
    target_sources(chase-game PRIVATE src/game_debris_super.cpp)
    target_sources(chase-game PRIVATE src/game_attack.cpp src/game_screen_fade.cpp src/game_screen_shake.cpp src/game_save.cpp)
    target_sources(chase-game PRIVATE src/game_prop_rocking.cpp src/runtime/game_rocking_follow_glue.cpp src/runtime/game_attach_children_glue.cpp src/game_platform_nodes.cpp)
    target_sources(chase-game PRIVATE src/runtime/game_push_cart_glue.cpp src/runtime/qol_window.cpp src/runtime/qol_controls_bindings.cpp)
    target_sources(chase-game PRIVATE src/runtime/game_platform_riding_glue.cpp)
    target_link_libraries(chase-game PRIVATE chase_pad_events chase_qol chase_render chase_runtime chase_input chase_audio chase_frontend_movie chase_save_progress chase_save_tables)
    target_link_libraries(chase-game PRIVATE chase_qol_view chase_qol_hud chase_qol_window)
    target_compile_options(chase-game PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()
set(CHASE_ROMFS_DIR "${CMAKE_SOURCE_DIR}/artifacts/romfs/romfs" CACHE PATH
    "Extracted RomFS directory for the opt-in real-data asset test")
option(CHASE_BUILD_TESTS "Build the test programs and register the CTest tests" ON)
set(BUILD_TESTING ${CHASE_BUILD_TESTS} CACHE BOOL "" FORCE)
include(CTest)
add_subdirectory(src/qol)
if(BUILD_TESTING)
  add_subdirectory(tests/qol)
  add_executable(input-test tests/input_test.cpp)
  target_link_libraries(input-test PRIVATE chase_input)
  target_compile_options(input-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME input-core COMMAND input-test)
  add_executable(pad-events-test tests/pad_events_test.cpp)
  target_link_libraries(pad-events-test PRIVATE chase_pad_events)
  target_compile_options(pad-events-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pad-events COMMAND pad-events-test)
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
  add_executable(hud-parts-test tests/hud_parts_test.cpp)
  target_link_libraries(hud-parts-test PRIVATE chase_frontend)
  target_compile_options(hud-parts-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME hud-parts COMMAND hud-parts-test)
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
      "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
    set_tests_properties(hud-real-data PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(hud-parts-real-test tests/hud_parts_real_test.cpp)
    target_link_libraries(hud-parts-real-test PRIVATE chase_frontend)
    target_compile_options(hud-parts-real-test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME hud-parts-real-data COMMAND hud-parts-real-test
      "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
    set_tests_properties(hud-parts-real-data PROPERTIES SKIP_RETURN_CODE 77)
  endif()
  add_executable(mixer-test tests/mixer_test.cpp)
  target_link_libraries(mixer-test PRIVATE chase_audio)
  target_compile_options(mixer-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME audio-mixer COMMAND mixer-test)
  add_executable(audio-test tests/audio_test.cpp)
  target_link_libraries(audio-test PRIVATE chase_audio chase_runtime)
  target_compile_options(audio-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME audio-core COMMAND audio-test)
  add_executable(voice-params-test tests/voice_params_test.cpp)
  target_link_libraries(voice-params-test PRIVATE chase_audio chase_assets)
  target_compile_options(voice-params-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME voice-params COMMAND voice-params-test)
  add_test(NAME voice-params-real-data COMMAND voice-params-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(voice-params-real-data PROPERTIES SKIP_RETURN_CODE 77)
  if(TARGET chase-game)
    add_executable(audio-real-test tests/audio_real_test.cpp)
    target_link_libraries(audio-real-test PRIVATE chase_audio chase_runtime chase_assets)
    target_compile_options(audio-real-test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME audio-real-data COMMAND audio-real-test
      "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
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
  add_executable(pedestrian-walk-test tests/pedestrian_walk_test.cpp)
  target_link_libraries(pedestrian-walk-test PRIVATE chase_runtime)
  target_compile_options(pedestrian-walk-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pedestrian-walk COMMAND pedestrian-walk-test)
  add_executable(pedestrian-manager-test tests/pedestrian_manager_test.cpp)
  target_link_libraries(pedestrian-manager-test PRIVATE chase_runtime)
  target_compile_options(pedestrian-manager-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pedestrian-manager COMMAND pedestrian-manager-test)
  add_executable(pedestrian-blocked-test tests/pedestrian_blocked_test.cpp)
  target_link_libraries(pedestrian-blocked-test PRIVATE chase_runtime)
  target_compile_options(pedestrian-blocked-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pedestrian-blocked COMMAND pedestrian-blocked-test)
  add_executable(pedestrian-fade-test tests/pedestrian_fade_test.cpp)
  target_link_libraries(pedestrian-fade-test PRIVATE chase_runtime chase_assets)
  target_compile_options(pedestrian-fade-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pedestrian-fade COMMAND pedestrian-fade-test)
  add_test(NAME pedestrian-fade-real-data COMMAND pedestrian-fade-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(pedestrian-fade-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(draw-order-test tests/draw_order_test.cpp)
  target_link_libraries(draw-order-test PRIVATE chase_draw_order)
  target_compile_options(draw-order-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME draw-order COMMAND draw-order-test)
  add_executable(draw-order-real-test tests/draw_order_real_test.cpp)
  target_link_libraries(draw-order-real-test PRIVATE chase_assets)
  target_compile_options(draw-order-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME draw-order-real-data COMMAND draw-order-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(draw-order-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(semi-trans-near-camera-test tests/semi_trans_near_camera_test.cpp)
  target_link_libraries(semi-trans-near-camera-test PRIVATE chase_runtime)
  target_compile_options(semi-trans-near-camera-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME semi-trans-near-camera COMMAND semi-trans-near-camera-test)
  add_executable(room-layers-test tests/room_layers_test.cpp)
  target_link_libraries(room-layers-test PRIVATE chase_runtime)
  target_compile_options(room-layers-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME room-layers COMMAND room-layers-test)
  add_executable(pedestrian-system-test tests/pedestrian_system_test.cpp)
  target_link_libraries(pedestrian-system-test PRIVATE chase_runtime chase_assets)
  target_compile_options(pedestrian-system-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pedestrian-system COMMAND pedestrian-system-test)
  add_test(NAME pedestrian-system-real-data COMMAND pedestrian-system-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(pedestrian-system-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(natives-party-test tests/natives_party_test.cpp)
  target_link_libraries(natives-party-test PRIVATE chase_runtime)
  target_compile_options(natives-party-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-party COMMAND natives-party-test)
  add_executable(natives-world-test tests/natives_world_test.cpp)
  target_link_libraries(natives-world-test PRIVATE chase_runtime)
  target_compile_options(natives-world-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-world COMMAND natives-world-test)
  add_executable(screen-fade-test tests/screen_fade_test.cpp)
  target_link_libraries(screen-fade-test PRIVATE chase_runtime)
  target_compile_options(screen-fade-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME screen-fade COMMAND screen-fade-test)
  add_executable(cutscene-camera-test tests/cutscene_camera_test.cpp)
  target_link_libraries(cutscene-camera-test PRIVATE chase_runtime)
  target_compile_options(cutscene-camera-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME cutscene-camera COMMAND cutscene-camera-test)
  add_executable(dialog-bg-fade-test tests/dialog_bg_fade_test.cpp)
  target_link_libraries(dialog-bg-fade-test PRIVATE chase_runtime chase_assets)
  target_compile_options(dialog-bg-fade-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME dialog-bg-fade COMMAND dialog-bg-fade-test)
  add_executable(natives-cutscene-test tests/natives_cutscene_test.cpp)
  target_link_libraries(natives-cutscene-test PRIVATE chase_runtime)
  target_compile_options(natives-cutscene-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-cutscene COMMAND natives-cutscene-test)
  add_executable(update-order-test tests/update_order_test.cpp)
  target_link_libraries(update-order-test PRIVATE chase_runtime)
  target_compile_options(update-order-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME update-order COMMAND update-order-test)
  add_executable(use-marker-test tests/use_marker_test.cpp)
  target_link_libraries(use-marker-test PRIVATE chase_runtime)
  target_compile_options(use-marker-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME use-marker COMMAND use-marker-test)
  add_executable(cutscene-cast-test tests/cutscene_cast_test.cpp)
  target_link_libraries(cutscene-cast-test PRIVATE chase_runtime chase_assets)
  target_compile_options(cutscene-cast-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME cutscene-cast COMMAND cutscene-cast-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  add_executable(cutscene-hold-test tests/cutscene_hold_test.cpp)
  target_link_libraries(cutscene-hold-test PRIVATE chase_runtime)
  target_compile_options(cutscene-hold-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME cutscene-hold COMMAND cutscene-hold-test)
  add_executable(node-draw-matrix-real-test tests/node_draw_matrix_real_test.cpp)
  target_link_libraries(node-draw-matrix-real-test PRIVATE chase_assets)
  target_compile_options(node-draw-matrix-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME node-draw-matrix-real-data COMMAND node-draw-matrix-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(node-draw-matrix-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(sub-objectives-test tests/sub_objectives_test.cpp)
  target_link_libraries(sub-objectives-test PRIVATE chase_runtime chase_assets)
  target_compile_options(sub-objectives-test PRIVATE -Wall -Wextra -Wpedantic)
  add_executable(waypoints-test tests/waypoints_test.cpp)
  target_link_libraries(waypoints-test PRIVATE chase_runtime chase_assets)
  target_compile_options(waypoints-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME waypoints COMMAND waypoints-test)
  add_executable(waypoint-global-test tests/waypoint_global_test.cpp)
  target_link_libraries(waypoint-global-test PRIVATE chase_runtime chase_assets)
  target_compile_options(waypoint-global-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME waypoint-global COMMAND waypoint-global-test)
  add_test(NAME waypoint-global-real-data COMMAND waypoint-global-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(waypoint-global-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME waypoints-real-data COMMAND waypoints-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(waypoints-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(buildit-test tests/buildit_test.cpp)
  target_link_libraries(buildit-test PRIVATE chase_runtime chase_assets)
  target_compile_options(buildit-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME buildit COMMAND buildit-test)
  add_executable(super-buildit-test tests/super_buildit_test.cpp)
  target_link_libraries(super-buildit-test PRIVATE chase_runtime chase_assets)
  target_compile_options(super-buildit-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME super-buildit COMMAND super-buildit-test)
  add_executable(superbuildit-messages-test tests/superbuildit_messages_test.cpp)
  target_link_libraries(superbuildit-messages-test PRIVATE chase_runtime chase_save_progress chase_assets)
  target_compile_options(superbuildit-messages-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME superbuildit-messages COMMAND superbuildit-messages-test)
  add_executable(fast-travel-pad-test tests/fast_travel_pad_test.cpp)
  target_link_libraries(fast-travel-pad-test PRIVATE chase_runtime chase_assets)
  target_compile_options(fast-travel-pad-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME fast-travel-pad COMMAND fast-travel-pad-test)
  add_executable(map-buttons-test tests/map_buttons_test.cpp)
  target_link_libraries(map-buttons-test PRIVATE chase_runtime)
  target_compile_options(map-buttons-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME map-buttons COMMAND map-buttons-test)
  add_executable(pause-menu-test tests/pause_menu_test.cpp)
  target_link_libraries(pause-menu-test PRIVATE chase_frontend chase_runtime chase_assets)
  target_compile_options(pause-menu-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pause-menu COMMAND pause-menu-test)
  add_executable(options-page-test tests/options_page_test.cpp)
  target_link_libraries(options-page-test PRIVATE chase_runtime)
  target_compile_options(options-page-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME options-page COMMAND options-page-test)
  add_executable(languages-page-test tests/languages_page_test.cpp)
  target_link_libraries(languages-page-test PRIVATE chase_runtime)
  target_compile_options(languages-page-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME languages-page COMMAND languages-page-test)
  add_executable(award-viewer-test tests/award_viewer_test.cpp)
  target_link_libraries(award-viewer-test PRIVATE chase_frontend chase_runtime chase_assets)
  target_compile_options(award-viewer-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME award-viewer COMMAND award-viewer-test)
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
  add_executable(hand-item-set-test tests/hand_item_set_test.cpp)
  target_link_libraries(hand-item-set-test PRIVATE chase_runtime chase_assets)
  target_compile_options(hand-item-set-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME hand-item-set COMMAND hand-item-set-test)
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
  add_executable(use-marker-real-test tests/use_marker_real_test.cpp)
  target_link_libraries(use-marker-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(use-marker-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME use-marker-real-data COMMAND use-marker-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(use-marker-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(reference-trigger-real-test tests/reference_trigger_real_test.cpp)
  target_link_libraries(reference-trigger-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(reference-trigger-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME reference-trigger-real-data COMMAND reference-trigger-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(reference-trigger-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(switch-test tests/switch_test.cpp)
  target_link_libraries(switch-test PRIVATE chase_runtime chase_assets)
  target_compile_options(switch-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME switch COMMAND switch-test)
  add_executable(lever-use-test tests/lever_use_test.cpp)
  target_link_libraries(lever-use-test PRIVATE chase_runtime chase_assets)
  target_compile_options(lever-use-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME lever-use COMMAND lever-use-test)
  add_executable(collision-enable-test tests/collision_enable_test.cpp)
  target_link_libraries(collision-enable-test PRIVATE chase_runtime chase_assets)
  target_compile_options(collision-enable-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME collision-enable COMMAND collision-enable-test)
  add_executable(carry-test tests/carry_test.cpp)
  target_link_libraries(carry-test PRIVATE chase_runtime chase_assets)
  target_compile_options(carry-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME carry COMMAND carry-test)
  add_executable(carry-arrows-test tests/carry_arrows_test.cpp)
  target_link_libraries(carry-arrows-test PRIVATE chase_runtime chase_assets)
  target_compile_options(carry-arrows-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME carry-arrows COMMAND carry-arrows-test)
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
  add_executable(scanner-reticle-test tests/scanner_reticle_test.cpp)
  target_link_libraries(scanner-reticle-test PRIVATE chase_runtime chase_assets)
  target_compile_options(scanner-reticle-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scanner-reticle COMMAND scanner-reticle-test)
  add_executable(scanner-markers-test tests/scanner_markers_test.cpp)
  target_link_libraries(scanner-markers-test PRIVATE chase_runtime)
  target_compile_options(scanner-markers-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scanner-markers COMMAND scanner-markers-test)
  add_executable(scanner-info-test tests/scanner_info_test.cpp)
  target_link_libraries(scanner-info-test PRIVATE chase_runtime)
  target_compile_options(scanner-info-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scanner-info COMMAND scanner-info-test)
  add_executable(scanner-bottom-test tests/scanner_bottom_test.cpp)
  target_link_libraries(scanner-bottom-test PRIVATE chase_runtime)
  target_compile_options(scanner-bottom-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scanner-bottom COMMAND scanner-bottom-test)
  add_executable(pause-ticker-test tests/pause_ticker_test.cpp)
  target_link_libraries(pause-ticker-test PRIVATE chase_runtime)
  target_compile_options(pause-ticker-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pause-ticker COMMAND pause-ticker-test)
  add_executable(scanner-render-test tests/scanner_render_test.cpp)
  target_link_libraries(scanner-render-test PRIVATE chase_runtime chase_assets)
  target_compile_options(scanner-render-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scanner-render COMMAND scanner-render-test)
  add_executable(glow-test tests/glow_test.cpp)
  target_link_libraries(glow-test PRIVATE chase_runtime chase_assets)
  target_compile_options(glow-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME glow COMMAND glow-test)
  add_executable(reflection-test tests/reflection_test.cpp)
  target_link_libraries(reflection-test PRIVATE chase_runtime chase_assets)
  target_compile_options(reflection-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME reflection-core COMMAND reflection-test)
  add_test(NAME reflection-real-data COMMAND reflection-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(reflection-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(scanner-overlay-test tests/scanner_overlay_test.cpp)
  target_link_libraries(scanner-overlay-test PRIVATE chase_frontend)
  target_compile_options(scanner-overlay-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scanner-overlay-data COMMAND scanner-overlay-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(scanner-overlay-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(zipline-test tests/zipline_test.cpp)
  target_link_libraries(zipline-test PRIVATE chase_runtime chase_assets)
  target_compile_options(zipline-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME zipline COMMAND zipline-test)
  add_executable(zipline-real-test tests/zipline_real_test.cpp)
  target_link_libraries(zipline-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(zipline-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME zipline-real-data COMMAND zipline-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(zipline-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(zipline-pole-test tests/zipline_pole_test.cpp)
  target_link_libraries(zipline-pole-test PRIVATE chase_runtime chase_assets)
  target_compile_options(zipline-pole-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME zipline-pole COMMAND zipline-pole-test)
  add_executable(nested-props-real-test tests/nested_props_real_test.cpp)
  target_link_libraries(nested-props-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(nested-props-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME nested-props-real-data COMMAND nested-props-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(nested-props-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(entity-ids-real-test tests/entity_ids_real_test.cpp)
  target_link_libraries(entity-ids-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(entity-ids-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME entity-ids-real-data COMMAND entity-ids-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(entity-ids-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(zipline-pole-real-test tests/zipline_pole_real_test.cpp)
  target_link_libraries(zipline-pole-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(zipline-pole-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME zipline-pole-real-data COMMAND zipline-pole-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(zipline-pole-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(zipline-rope-test tests/zipline_rope_test.cpp)
  target_link_libraries(zipline-rope-test PRIVATE chase_runtime chase_assets)
  target_compile_options(zipline-rope-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME zipline-rope COMMAND zipline-rope-test)
  add_executable(zipline-rope-real-test tests/zipline_rope_real_test.cpp)
  target_link_libraries(zipline-rope-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(zipline-rope-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME zipline-rope-real-data COMMAND zipline-rope-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(zipline-rope-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(acrobat-pole-test tests/acrobat_pole_test.cpp)
  target_link_libraries(acrobat-pole-test PRIVATE chase_runtime chase_assets)
  target_compile_options(acrobat-pole-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME acrobat-pole COMMAND acrobat-pole-test)
  add_executable(acrobat-pole-real-test tests/acrobat_pole_real_test.cpp)
  target_link_libraries(acrobat-pole-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(acrobat-pole-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME acrobat-pole-real-data COMMAND acrobat-pole-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(acrobat-pole-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(pole-test tests/pole_test.cpp)
  target_link_libraries(pole-test PRIVATE chase_runtime chase_assets)
  target_compile_options(pole-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pole COMMAND pole-test)
  add_executable(pole-real-test tests/pole_real_test.cpp)
  target_link_libraries(pole-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(pole-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pole-real-data COMMAND pole-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(pole-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(ladder-test tests/ladder_test.cpp)
  target_link_libraries(ladder-test PRIVATE chase_runtime chase_assets)
  target_compile_options(ladder-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME ladder COMMAND ladder-test)
  add_executable(fence-vault-test tests/fence_vault_test.cpp)
  target_link_libraries(fence-vault-test PRIVATE chase_runtime chase_assets)
  target_compile_options(fence-vault-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME fence-vault COMMAND fence-vault-test)
  add_executable(fence-vault-real-test tests/fence_vault_real_test.cpp)
  target_link_libraries(fence-vault-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(fence-vault-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME fence-vault-real-data COMMAND fence-vault-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(fence-vault-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(ladder-sounds-test tests/ladder_sounds_test.cpp)
  target_link_libraries(ladder-sounds-test PRIVATE chase_runtime chase_assets)
  target_compile_options(ladder-sounds-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME ladder-sounds COMMAND ladder-sounds-test)
  add_executable(ladder-sounds-real-test tests/ladder_sounds_real_test.cpp)
  target_link_libraries(ladder-sounds-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(ladder-sounds-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME ladder-sounds-real-data COMMAND ladder-sounds-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(ladder-sounds-real-data PROPERTIES SKIP_RETURN_CODE 77)
  set_property(TEST ladder-sounds-real-data APPEND PROPERTY LABELS needs-game-data)
  add_executable(ladder-real-test tests/ladder_real_test.cpp)
  target_link_libraries(ladder-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(ladder-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME ladder-real-data COMMAND ladder-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(ladder-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(carry-animation-real-test tests/carry_animation_real_test.cpp)
  target_link_libraries(carry-animation-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(carry-animation-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME carry-animation-real-data COMMAND carry-animation-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(carry-animation-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(lever-use-real-test tests/lever_use_real_test.cpp)
  target_link_libraries(lever-use-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(lever-use-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME lever-use-real-data COMMAND lever-use-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(lever-use-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(super-buildit-real-test tests/super_buildit_real_test.cpp)
  target_link_libraries(super-buildit-real-test PRIVATE chase_runtime chase_frontend chase_assets)
  target_compile_options(super-buildit-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME super-buildit-real-data COMMAND super-buildit-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(super-buildit-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(layout-text-real-test tests/layout_text_real_test.cpp)
  target_link_libraries(layout-text-real-test PRIVATE chase_frontend chase_assets)
  target_compile_options(layout-text-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME layout-text-real-data COMMAND layout-text-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(layout-text-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(layout-scale-real-test tests/layout_scale_real_test.cpp)
  target_link_libraries(layout-scale-real-test PRIVATE chase_frontend chase_assets)
  target_compile_options(layout-scale-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME layout-scale-real-data COMMAND layout-scale-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(layout-scale-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(fast-travel-pad-real-test tests/fast_travel_pad_real_test.cpp)
  target_link_libraries(fast-travel-pad-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(fast-travel-pad-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME fast-travel-pad-real-data COMMAND fast-travel-pad-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(fast-travel-pad-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(map-buttons-real-test tests/map_buttons_real_test.cpp)
  target_link_libraries(map-buttons-real-test PRIVATE chase_frontend chase_assets)
  target_compile_options(map-buttons-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME map-buttons-real-data COMMAND map-buttons-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(map-buttons-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(pause-menu-real-test tests/pause_menu_real_test.cpp)
  target_link_libraries(pause-menu-real-test PRIVATE chase_frontend chase_assets)
  target_compile_options(pause-menu-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pause-menu-real-data COMMAND pause-menu-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(pause-menu-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(award-viewer-real-test tests/award_viewer_real_test.cpp)
  target_link_libraries(award-viewer-real-test PRIVATE chase_frontend chase_save_tables chase_assets)
  target_compile_options(award-viewer-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME award-viewer-real-data COMMAND award-viewer-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(award-viewer-real-data PROPERTIES SKIP_RETURN_CODE 77 LABELS needs-game-data)
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
  add_executable(prop-root-motion-test tests/prop_root_motion_test.cpp)
  target_link_libraries(prop-root-motion-test PRIVATE chase_runtime chase_assets)
  target_compile_options(prop-root-motion-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME prop-root-motion COMMAND prop-root-motion-test)
  add_executable(animal-props-real-test tests/animal_props_real_test.cpp)
  target_link_libraries(animal-props-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(animal-props-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME animal-props-real-data COMMAND animal-props-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(animal-props-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(nested-object-collision-test tests/nested_object_collision_test.cpp)
  target_link_libraries(nested-object-collision-test PRIVATE chase_runtime chase_assets)
  target_compile_options(nested-object-collision-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME nested-object-collision-real-data COMMAND nested-object-collision-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(nested-object-collision-real-data PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(cat-rescue-real-test tests/cat_rescue_real_test.cpp)
  target_link_libraries(cat-rescue-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(cat-rescue-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME cat-rescue-real-data COMMAND cat-rescue-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(cat-rescue-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(destroy-effects-test tests/destroy_effects_test.cpp)
  target_link_libraries(destroy-effects-test PRIVATE chase_runtime chase_assets)
  target_compile_options(destroy-effects-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME destroy-effects COMMAND destroy-effects-test)
  add_test(NAME destroy-effects-real-data COMMAND destroy-effects-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(destroy-effects-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(character-death-test tests/character_death_test.cpp)
  target_link_libraries(character-death-test PRIVATE chase_runtime chase_assets)
  target_compile_options(character-death-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME character-death COMMAND character-death-test)
  add_test(NAME character-death-real-data COMMAND character-death-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(character-death-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(player-attack-test tests/player_attack_test.cpp)
  target_link_libraries(player-attack-test PRIVATE chase_runtime chase_assets)
  target_compile_options(player-attack-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME player-attack COMMAND player-attack-test)
  add_test(NAME player-attack-real-data COMMAND player-attack-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(player-attack-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(attack-sounds-test tests/attack_sounds_test.cpp)
  target_link_libraries(attack-sounds-test PRIVATE chase_runtime chase_assets)
  target_compile_options(attack-sounds-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME attack-sounds COMMAND attack-sounds-test)
  add_test(NAME attack-sounds-real-data COMMAND attack-sounds-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(attack-sounds-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(stud-drop-test tests/stud_drop_test.cpp)
  target_link_libraries(stud-drop-test PRIVATE chase_runtime chase_assets)
  target_compile_options(stud-drop-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME stud-drop COMMAND stud-drop-test)
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
  add_test(NAME sub-objectives COMMAND sub-objectives-test)
  add_executable(map-marker-test tests/map_marker_test.cpp)
  target_link_libraries(map-marker-test PRIVATE chase_frontend)
  target_compile_options(map-marker-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME map-marker COMMAND map-marker-test)
  add_executable(map-file-test tests/map_file_test.cpp)
  target_link_libraries(map-file-test PRIVATE chase_frontend)
  target_compile_options(map-file-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME map-file COMMAND map-file-test)
  add_executable(map-hub-test tests/map_hub_test.cpp)
  target_link_libraries(map-hub-test PRIVATE chase_frontend)
  target_compile_options(map-hub-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME map-hub COMMAND map-hub-test)
  add_test(NAME map-file-real-data COMMAND map-file-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(map-file-real-data PROPERTIES SKIP_RETURN_CODE 77 LABELS needs-game-data)
  add_test(NAME objectives-real-data COMMAND objectives-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(objectives-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(object-kinds-test tests/object_kinds_test.cpp)
  target_link_libraries(object-kinds-test PRIVATE chase_assets)
  target_compile_options(object-kinds-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME object-kinds COMMAND object-kinds-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(object-kinds PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(scene-start-test tests/scene_start_test.cpp)
  target_link_libraries(scene-start-test PRIVATE chase_runtime)
  target_compile_options(scene-start-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scene-start COMMAND scene-start-test)
  add_executable(object-rooms-test tests/object_rooms_test.cpp)
  target_link_libraries(object-rooms-test PRIVATE chase_runtime)
  target_compile_options(object-rooms-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME object-rooms COMMAND object-rooms-test)
  add_executable(pedestrian-rooms-test tests/pedestrian_rooms_test.cpp)
  target_link_libraries(pedestrian-rooms-test PRIVATE chase_runtime)
  target_compile_options(pedestrian-rooms-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pedestrian-rooms COMMAND pedestrian-rooms-test)
  add_executable(player-controller-state-test tests/player_controller_state_test.cpp)
  target_link_libraries(player-controller-state-test PRIVATE chase_runtime)
  target_compile_options(player-controller-state-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME player-controller-state COMMAND player-controller-state-test)
  add_executable(dcam-test tests/dcam_test.cpp)
  target_link_libraries(dcam-test PRIVATE chase_runtime)
  target_compile_options(dcam-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME dcam COMMAND dcam-test)
  add_executable(screen-shake-test tests/screen_shake_test.cpp)
  target_link_libraries(screen-shake-test PRIVATE chase_runtime)
  target_compile_options(screen-shake-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME screen-shake COMMAND screen-shake-test)
  add_executable(director-hold-test tests/director_hold_test.cpp)
  target_link_libraries(director-hold-test PRIVATE chase_runtime)
  target_compile_options(director-hold-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME director-hold COMMAND director-hold-test)
  add_executable(progress-tables-test tests/progress_tables_test.cpp)
  target_link_libraries(progress-tables-test PRIVATE chase_runtime)
  target_compile_options(progress-tables-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME progress-tables COMMAND progress-tables-test)
  add_test(NAME progress-readme COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_on_main.py" python3 "${CMAKE_SOURCE_DIR}/tools/progress_report.py" --check "${CMAKE_SOURCE_DIR}/README.md")
  set_tests_properties(progress-readme PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "CHASE_ASSETS_LIB=$<TARGET_FILE:chase_assets_c>")
  add_test(NAME priorities-current COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_on_main.py" python3 "${CMAKE_SOURCE_DIR}/tools/priority_report.py" --check "${CMAKE_SOURCE_DIR}/docs/plans/priorities.md")
  set_tests_properties(priorities-current PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "CHASE_ASSETS_LIB=$<TARGET_FILE:chase_assets_c>")
  add_test(NAME no-inline-shaders COMMAND ${CMAKE_COMMAND} -DSRC_DIR=${CMAKE_SOURCE_DIR}/src -P ${CMAKE_SOURCE_DIR}/tools/check_no_inline_shaders.cmake)
  add_test(NAME gl-only-in-render COMMAND ${CMAKE_COMMAND} -DSRC_DIR=${CMAKE_SOURCE_DIR}/src -P ${CMAKE_SOURCE_DIR}/tools/check_gl_only_in_render.cmake)
  add_test(NAME gl-only-in-render-rejects COMMAND ${CMAKE_COMMAND} -DSRC_DIR=${CMAKE_SOURCE_DIR}/tests/gl_only_in_render/src -P ${CMAKE_SOURCE_DIR}/tools/check_gl_only_in_render.cmake)
  set_tests_properties(gl-only-in-render-rejects PROPERTIES WILL_FAIL TRUE)
  add_test(NAME qol-separation COMMAND ${CMAKE_COMMAND} -DROOT_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/tests/qol_separation.cmake)
  set(qolSeparationFixtures ${CMAKE_SOURCE_DIR}/tests/qol_separation_fixtures)
  add_test(NAME qol-separation-accepts COMMAND ${CMAKE_COMMAND} -DROOT_DIR=${qolSeparationFixtures}/ok -P ${CMAKE_SOURCE_DIR}/tests/qol_separation.cmake)
  foreach(rejection IN ITEMS
      leak_identifier|src/runtime/leak.cpp
      leak_chase_qol|src/runtime/leak.cpp
      include_without_hook|src/runtime/inc.cpp
      unlisted_hook|src/runtime/door.cpp
      listed_without_code|src/runtime/door.cpp
      malformed_entry|qol-settings-menu.md
      copied_port_file|src/qol/other.cpp
      test_leak|tests/door_test.cpp)
    string(REPLACE "|" ";" rejectionParts "${rejection}")
    list(GET rejectionParts 0 fixture)
    list(GET rejectionParts 1 expected)
    add_test(NAME qol-separation-rejects-${fixture} COMMAND ${CMAKE_COMMAND} -DROOT_DIR=${qolSeparationFixtures}/${fixture} -DGUARD=${CMAKE_SOURCE_DIR}/tests/qol_separation.cmake -DEXPECT=${expected} -P ${CMAKE_SOURCE_DIR}/tests/qol_separation_rejects.cmake)
  endforeach()
  add_test(NAME game-cpp-wiring COMMAND ${CMAKE_COMMAND} -DGAME_CPP=${CMAKE_SOURCE_DIR}/src/game.cpp -P ${CMAKE_SOURCE_DIR}/tools/check_game_cpp_wiring.cmake)
  add_test(NAME doc-links COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_doc_links.py" --root "${CMAKE_SOURCE_DIR}")
  add_test(NAME no-local-data COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_local_data.py" --root "${CMAKE_SOURCE_DIR}")
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
  add_executable(camera-blocker-test tests/camera_blocker_test.cpp)
  target_link_libraries(camera-blocker-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-blocker-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-blocker COMMAND camera-blocker-test)
  add_executable(camera-records-real-test tests/camera_records_real_test.cpp)
  target_link_libraries(camera-records-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-records-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-records-real-data COMMAND camera-records-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(camera-records-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(locomotion-real-test tests/locomotion_real_test.cpp)
  target_link_libraries(locomotion-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(locomotion-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME locomotion-real-data COMMAND locomotion-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
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
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(collision-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(clip-planes-test tests/clip_planes_test.cpp)
  target_link_libraries(clip-planes-test PRIVATE chase_runtime chase_assets)
  target_compile_options(clip-planes-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME clip-planes COMMAND clip-planes-test)
  add_executable(canvas-clip-test tests/canvas_clip_test.cpp)
  target_link_libraries(canvas-clip-test PRIVATE chase_frontend chase_assets)
  target_compile_options(canvas-clip-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME canvas-clip COMMAND canvas-clip-test)
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
  add_executable(shadow-test tests/shadow_test.cpp)
  target_link_libraries(shadow-test PRIVATE chase_runtime chase_assets)
  target_compile_options(shadow-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME shadow-core COMMAND shadow-test)
  add_test(NAME shadow-real-data COMMAND shadow-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(shadow-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(pedestrian-limit-real-test tests/pedestrian_limit_real_test.cpp)
  target_link_libraries(pedestrian-limit-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(pedestrian-limit-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pedestrian-limit-real-data COMMAND pedestrian-limit-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(pedestrian-limit-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(natives-party-real-test tests/natives_party_real_test.cpp)
  target_link_libraries(natives-party-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(natives-party-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME natives-party-real-data COMMAND natives-party-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(natives-party-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  add_executable(script-call-targets-real-test tests/script_call_targets_real_test.cpp)
  target_link_libraries(script-call-targets-real-test PRIVATE chase_assets)
  target_compile_options(script-call-targets-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME script-call-targets-real-data COMMAND script-call-targets-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(script-call-targets-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(scene-change-real-test tests/scene_change_real_test.cpp)
  target_link_libraries(scene-change-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(scene-change-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME scene-change-real-data COMMAND scene-change-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(scene-change-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(same-level-entry-real-test tests/same_level_entry_real_test.cpp)
  target_link_libraries(same-level-entry-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(same-level-entry-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME same-level-entry-real-data COMMAND same-level-entry-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(same-level-entry-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(door-real-test tests/door_real_test.cpp)
  target_link_libraries(door-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(door-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME door-real-data COMMAND door-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(door-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(fall-real-test tests/fall_real_test.cpp)
  target_link_libraries(fall-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(fall-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME fall-real-data COMMAND fall-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(fall-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(jump-real-test tests/jump_real_test.cpp)
  target_link_libraries(jump-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(jump-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME jump-real-data COMMAND jump-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(jump-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(slide-real-test tests/slide_real_test.cpp)
  target_link_libraries(slide-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(slide-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slide-real-data COMMAND slide-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(slide-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(slope-real-test tests/slope_real_test.cpp)
  target_link_libraries(slope-real-test PRIVATE chase_runtime)
  target_compile_options(slope-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slope-real-data COMMAND slope-real-test)
  add_executable(camera-real-test tests/camera_real_test.cpp)
  target_link_libraries(camera-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-real-data COMMAND camera-real-test "${CMAKE_SOURCE_DIR}/tests/data/camera"
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(camera-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(camera-blend-real-test tests/camera_blend_real_test.cpp)
  target_link_libraries(camera-blend-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-blend-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-blend-real-data COMMAND camera-blend-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib"
    "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CMAKE_SOURCE_DIR}/tests/data/camera")
  set_tests_properties(camera-blend-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(camera-vehicle-real-test tests/camera_vehicle_real_test.cpp)
  target_link_libraries(camera-vehicle-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-vehicle-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-vehicle-real-data COMMAND camera-vehicle-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib"
    "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CMAKE_SOURCE_DIR}/tests/data/camera")
  set_tests_properties(camera-vehicle-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(camera-sweep-real-test tests/camera_sweep_real_test.cpp)
  target_link_libraries(camera-sweep-real-test PRIVATE chase_runtime chase_assets)
  target_compile_options(camera-sweep-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME camera-sweep-real-data COMMAND camera-sweep-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CMAKE_SOURCE_DIR}/tests/data/camera")
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
  add_executable(make-test-save tests/make_test_save.cpp)
  target_link_libraries(make-test-save PRIVATE chase_save_progress chase_save_tables)
  target_compile_options(make-test-save PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME test-save-build COMMAND make-test-save
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv" "${CMAKE_BINARY_DIR}/test-save/savegame.dat")
  set_tests_properties(test-save-build PROPERTIES FIXTURES_SETUP test_save)
  add_executable(slot-progress-real-test tests/slot_progress_real_test.cpp)
  target_link_libraries(slot-progress-real-test PRIVATE chase_save_progress chase_save_tables chase_runtime)
  target_compile_options(slot-progress-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slot-progress-real-data COMMAND slot-progress-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(slot-progress-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(save-writer-test tests/save_writer_test.cpp)
  target_link_libraries(save-writer-test PRIVATE chase_save_progress)
  target_compile_options(save-writer-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME save-writer COMMAND save-writer-test)
  add_executable(save-hud-counter-test tests/save_hud_counter_test.cpp)
  target_link_libraries(save-hud-counter-test PRIVATE chase_save_progress chase_runtime)
  target_compile_options(save-hud-counter-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME save-hud-counter COMMAND save-hud-counter-test)
  add_executable(save-writer-real-test tests/save_writer_real_test.cpp)
  target_link_libraries(save-writer-real-test PRIVATE chase_save_progress chase_save_tables)
  target_compile_options(save-writer-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME save-writer-real-data COMMAND save-writer-real-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(save-writer-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(new-game-test tests/new_game_test.cpp)
  target_link_libraries(new-game-test PRIVATE chase_save_progress chase_save_tables)
  target_compile_options(new-game-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME new-game-slot COMMAND new-game-test)
  add_test(NAME new-game-slot-real-data COMMAND new-game-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(new-game-slot-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(level-state-test tests/level_state_test.cpp)
  target_link_libraries(level-state-test PRIVATE chase_save_progress chase_save_tables)
  target_compile_options(level-state-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME level-state COMMAND level-state-test)
  add_test(NAME level-state-real-data COMMAND level-state-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(level-state-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(pregen-derive-test tests/pregen_derive_test.cpp)
  target_link_libraries(pregen-derive-test PRIVATE chase_runtime chase_assets)
  target_compile_options(pregen-derive-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pregen-derive COMMAND pregen-derive-test)
  add_test(NAME pregen-derive-real-data COMMAND pregen-derive-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(pregen-derive-real-data PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(pregen-traffic-test tests/pregen_traffic_test.cpp)
  target_link_libraries(pregen-traffic-test PRIVATE chase_assets)
  target_compile_options(pregen-traffic-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pregen-traffic COMMAND pregen-traffic-test)
  add_test(NAME pregen-traffic-real-data COMMAND pregen-traffic-test
    "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
  set_tests_properties(pregen-traffic-real-data PROPERTIES SKIP_RETURN_CODE 77)
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
  target_link_libraries(studs-real-test PRIVATE chase_runtime)
  target_compile_options(studs-real-test PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME studs-real-data COMMAND studs-real-test "${CMAKE_SOURCE_DIR}/tests/data")
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
    add_test(NAME hudshot-option-press COMMAND chase-hudshot --script "pause; press X")
    set_tests_properties(hudshot-option-press PROPERTIES PASS_REGULAR_EXPRESSION "chase-hudshot: --script: unknown button: 'X'")
    add_test(NAME hudshot-option-touch COMMAND chase-hudshot --script "pause; touch 320 10 1")
    set_tests_properties(hudshot-option-touch PROPERTIES PASS_REGULAR_EXPRESSION "chase-hudshot: --script: out of range, allowed 0 to 319: '320'")
    add_test(NAME hudshot-option-brickcount COMMAND chase-hudshot --script "brickcount x")
    set_tests_properties(hudshot-option-brickcount PROPERTIES PASS_REGULAR_EXPRESSION "chase-hudshot: --script: not a whole number: 'x'")
    add_test(NAME hudshot-option-brick COMMAND chase-hudshot --script "brick 1")
    set_tests_properties(hudshot-option-brick PROPERTIES PASS_REGULAR_EXPRESSION "chase-hudshot: --script: expected brick: 'brick 1'")
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
  if(TARGET chase-layoutdump)
    add_test(NAME layoutdump-option-unknown COMMAND chase-layoutdump --bogus)
    set_tests_properties(layoutdump-option-unknown PROPERTIES PASS_REGULAR_EXPRESSION "chase-layoutdump: --bogus: unknown option: '--bogus'")
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
      add_test(NAME game-save-continue COMMAND chase-game
        --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
        --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --save "${CMAKE_BINARY_DIR}/test-save/savegame.dat" --slot 0 --frames 1 --screenshot game-continue.bmp)
      set_tests_properties(game-save-continue PROPERTIES FIXTURES_REQUIRED test_save ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
        PASS_REGULAR_EXPRESSION "continue: slot 0 starts level 1 \\(lvl_00_policestation\\).*player at -2\\.1713 1\\.4203 17\\.2821")
      add_test(NAME game-save-hud-studs COMMAND chase-game
        --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
        --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --save "${CMAKE_BINARY_DIR}/test-save/savegame.dat" --slot 0
        --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --script "0 0 2")
      set_tests_properties(game-save-hud-studs PROPERTIES FIXTURES_REQUIRED test_save ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
        PASS_REGULAR_EXPRESSION "save: level entry 2, studs 1130, super bricks 52.*hud: 4 hearts, stud display 1130, bar state 1")
      add_test(NAME game-save-continue-empty-slot COMMAND chase-game
        --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
        --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --save "${CMAKE_BINARY_DIR}/test-save/savegame.dat" --slot 1 --frames 1)
      set_tests_properties(game-save-continue-empty-slot PROPERTIES FIXTURES_REQUIRED test_save ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
        PASS_REGULAR_EXPRESSION "the save slot is empty")
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
    # save state in front of the donut van parts build-it of mission 1-1, X at update 179) enters 23 updates later (202) and 1-1_Van_Waypoint_Enter starts its script 33 updates after the X (212): 83 and 93 here.
    # A short drive (60 updates of A), then the dev step "at" moves the seated van next to 1-1_Waypoint_05 (820, 18.5, 1047: in front of the waypoint, near the
    # oracle save state driving the donut van to the shop in mission 1-1 at 813.9 17.5 1046.8) instead of the 360 unit drive; the van reaches the waypoint at update 304 and 1-1_VanEscorted calls
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
    # The dog of cutscene 1_2_c is the level prop 1-2_Dog (CSCharacter "dog", builder 0x2d1ddc, LEVOBJ-100): the cutscene poses the prop, no model of its own.
    add_test(NAME game-cutscene-dog-prop COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "916.702 64.431 663.724" --facing 0 --call-script 1-2_Pringles_Cage_Locked
      --script "0 0 5;0 0 1 A;0 0 185")
    set_tests_properties(game-cutscene-dog-prop PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "prop 1-2_dog: its own animation is not shown while a cutscene holds it.*cutscene 1_2_c started at update 169.*cutscene 1_2_c: 0 level figures drawn beside their cast copies, 1 models of their own drawn, 1 level props posed")
    # Super build bricks (docs/re/super-bricks.md, STUD-16): FP_Superbrick_01 (948.327 95.031 1124.175, level data) is enabled at the start; the
    # player at the brick picks it up in update 0 and the destroy adds SuperBuildValue 1500 and plays UI sound 14 31 updates later.
    add_test(NAME game-super-brick-freeplay COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --script "at 948.327 95.031 1124.175 0;0 0 40")
    set_tests_properties(game-super-brick-freeplay PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "super bricks: total 1500, 0 collecting, 1 collected, 7 left.*sounds: 31:14")
    # Saves (docs/re/save-format-state.md, SAVE-15, SAVE-11, STUD-17): the same run as a new game writes the save request of the brick's destroy
    # to the default save file (XDG_DATA_HOME is the test's own directory, set for every test below); a second run continues from that file
    # and starts with the 1500 super bricks and the saved level state (StoryProgress 1 as in the oracle's gameplay-start slot, SAVE-14).
    add_test(NAME game-save-new-game-clean COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/test-xdg/game-save-new-game")
    set_tests_properties(game-save-new-game-clean PROPERTIES FIXTURES_SETUP save_new_game_clean)
    add_test(NAME game-save-new-game COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --script "at 948.327 95.031 1124.175 0;0 0 40")
    set_tests_properties(game-save-new-game PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      FIXTURES_REQUIRED save_new_game_clean FIXTURES_SETUP save_new_game
      PASS_REGULAR_EXPRESSION "save: new game in slot 0, save requests write [^\n]*/test-xdg/game-save-new-game/chase/savegame\\.dat.*save: level entry 2, studs 0, super bricks 0.*save: request 1 written to")
    add_test(NAME game-save-reload COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --save "${CMAKE_BINARY_DIR}/test-xdg/game-save-new-game/chase/savegame.dat" --slot 0
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --script "0 0 2")
    set_tests_properties(game-save-reload PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      FIXTURES_REQUIRED save_new_game
      PASS_REGULAR_EXPRESSION "save: loaded in slot 0[^\n]*\nsave: level entry 2, studs [0-9]+, super bricks 1500\n.*save: slot 0, level 2, saved state of the level \\(StoryProgress 1, [0-9]+ variables\\), unlocked levels 0x4,.*super bricks: total 1500,")
    # The roof brick of mission 1-3: 1-3_Rainbow_02 (904.929 30.697 1133.099, level data) is hidden until 1-3_ArrivedAtStation has shown it (update
    # 1073 of the port's run). The player stands 2.53 from it on the roof (the position of the save state on the roof in front of the super brick of mission 1-3) and is moved
    # to 1.05 from it at update 1100; the brick adds 200. About 10 s (slow).
    add_test(NAME game-super-brick-roof COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "905.671 30.719 1130.679" --facing -30 --call-script 1-3_ArrivedAtStation
      --script "0 0 1100;0 0 1;at 905.4 30.72 1132.2 0;0 0 40")
    set_tests_properties(game-super-brick-roof PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "super bricks: total 200, 0 collecting, 1 collected, 7 left")
    # LEVOBJ-107: the teardown 0x2d135c puts the entity node back to the matrix saved at the setup 0x2d1524, so the dog of 1_2_c stands at
    # 1-2_Dog_Position_End (730.328 31.599 645.549, level data) after the cutscene, not at its last pose. The run is about 8 s (slow).
    add_test(NAME game-cutscene-dog-end COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "916.702 64.431 663.724" --facing 0 --call-script 1-2_Pringles_Cage_Locked
      --script "0 0 5;0 0 1 A;0 0 760")
    set_tests_properties(game-cutscene-dog-end PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "cutscene 1_2_c ended: level prop at 730.328 31.5987 645.549")
    # The police car hits the parked FP_Vehicle_03 (docs/re/vehicles-physics-collision.md, the vehicle pair contacts): the dev step "at" turns the seated
    # car to the worker's heading 10 m behind it, where the oracle save state with the police car placed behind a parked car starts; A held from update 1833, the car
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
    # PED-11: a walking pedestrian changes room by the object rule of 0x2e6ce4; in the port's run from the level start (with the mesh node tier 0 spawn window) slot 5 leaves Room8 for
    # Room7 at update 281 (`chase-game ... --no-start-cutscenes --script "0 0 300"`).
    add_test(NAME game-pedestrian-rooms COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --script "0 0 300")
    set_tests_properties(game-pedestrian-rooms PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "pedestrian slot 5: room 8 -> 7 at update 281")
    add_test(NAME game-vehicle-hit COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_VanBreakdown --call-script 1-1_Get_Car --start "886.0289 17.8243 1118.6516"
      --script "0 0 1500;0 0 3 X;0 0 330;at 936.39 17.85 1123.94 134.7;0 0 40 A;0 0 3 X;0 0 30")
    set_tests_properties(game-vehicle-hit PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "fp_vehicle_03: Bullet body, mass scale 1.*entered at update 1523.*fp_vehicle_03: moved by a contact at update 1856.*left at update 1896")
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
    # The destroyed shelter starts its ExplosionParticleDef and bursts its bricks (level data; docs/re/debris.md): nine with super bricks, as 0x2b3c48 sets
    # 0x3743f2 at the level load and 0x2cdca4 gives the wobbly kind 0x77 the value 6 (llvm-objdump, docs/re/debris.md, entry of round 47); the contact at
    # 11.3 m/s before it has the value 1 (0x179e3c..0x179e64).
    add_test(NAME game-vehicle-smash COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_VanBreakdown --call-script 1-1_Get_Car --start "886.0289 17.8243 1118.6516"
      --script "0 0 1500;0 0 3 X;0 0 330;at 854.43 17.1 1087.36 134.7;0 0 60 A;0 0 3 X;0 0 30")
    set_tests_properties(game-vehicle-smash PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "vehicle 1-1_Police_Car: 1 super bricks in the brick burst\n.*prop Bus_Shelter_01: smashed by 1-1_police_car at update 1886\nprop Bus_Shelter_01: explosion particle 10, 9 bricks, 6 super bricks of value 6")
    # The super bricks of the contact and of the smashed Bus_Shelter_01 come to rest, the magnet of the seated player (type row 6 +0x64 = 0) pulls each for
    # its life 1.0 (30 updates at dt 0.033), the collected record flies to the HUD for 1.0 (31 updates), then 0x29c090 adds its value 1 to the count and
    # 0x28a128 10 studs: the cycle of the oracle request B-1 (the save state in a police van in front of a lamp post, stepped: state 1 -> 3 one update after the landing, 30 updates in state 3,
    # 31 in state 4, +1 and +10 studs in the update of the free; docs/re/super-bricks.md, entry of the debris pickup). Update numbers are the port's.
    add_test(NAME game-debris-super-pickup COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-1_VanBreakdown --call-script 1-1_Get_Car --start "886.0289 17.8243 1118.6516"
      --script "0 0 1500;0 0 3 X;0 0 330;at 854.43 17.1 1087.36 134.7;0 0 60 A;0 0 100")
    set_tests_properties(game-debris-super-pickup PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "prop Bus_Shelter_01: smashed by 1-1_police_car.*\n.*super debris brick banked: value 1, count 1 at update.*\n.*super debris brick banked: value 1, count 7 at update.*\n.*studs: counter 70,.*\n.*super bricks: total 7,")
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
    # (oracle save state on the second detective waypoint of mission 1-2) enters detective mode, the X spot 1-2_Dtv_DogPath_XSpot01 ahead within 2.5 ends it, A on the X starts
    # the use state; its event (43 updates), the reveal time (32), RevealDelay (30) and the flight (18) lead to 1-2_SO_Dspot_02_X, the oracle's
    # intervals of the detective save states of mission 1-2. Teleports stand in for the walks.
    add_test(NAME game-detective-1-2 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "723.675 29.768 630.775" --facing 291.7 --call-script 1-2_SO_IntroCamOff
      --script "0 0 160;at 720.021 37.035 654.390 289.6;0 0 1 A;0 0 40;at 713.06 37.06 654.62 289.6;0 0 25;at 711.6 37.06 655.1 289.6;0 0 1 A;0 0 130")
    set_tests_properties(game-detective-1-2 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "detective: player state 1 at update 160.*1-2_dtv_dogspot_02 state 1 at update 161.*player state 0 at update 197.*1-2_dtv_dogspot_02 state 2 at update 202.*player state 2 at update 202.*player state 3 at update 226.*1-2_dtv_dogpath_xspot01 state 1 at update 270.*1-2_dtv_dogpath_xspot01 state 2 at update 302.*1-2_trapdoor_03 state 2 at update 333.*script 1-2_SO_Dspot_02_X started at update 351")
    # Mission 1-3, the key (SCRIPT-361): 1-3_ConstructionYard (the step before) enables 1-3_Dtv_BrickSpot_01; A on the spot, A on the X spot
    # 1-3_Dtv_BrickSpot_XSpot_02 (teleports stand in for the footprint walk) reveals the key through the dirt mound 1-3_TrapDoor_02, and the key's carry
    # arrow appears. A where the save state in front of the key stands (1.71 from it, outside its zone) does nothing, as in the original; A where the oracle
    # picked it up (RPC 20261025-recorded-C-1, recorded from a save state, 2026-10-25) picks it up and the carry arrow moves to the keyhole.
    add_test(NAME game-key-pickup-1-3 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --save-out "${CMAKE_BINARY_DIR}/game-key-pickup-1-3.dat"
      --start "946.4 16.8 1140.0" --facing 0 --call-script 1-3_ConstructionYard
      --script "0 0 1200;at 946.408 16.8 1145.364 0;0 0 1 A;0 0 60;at 987.88 16.56 1143.4 90;0 0 25;at 989.28 16.56 1143.4 90;0 0 1 A;0 0 200;at 987.808 16.644 1143.306 246.7;0 0 5;0 0 1 A;0 0 40;at 986.756 16.624 1142.715 246.7;0 0 5;0 0 1 A;0 0 30")
    set_tests_properties(game-key-pickup-1-3 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "carry arrow over 1-3_garage_key added at update [0-9]+\n.*script 1-3_TrapDoor_02_Script started at update [0-9]+.*script teleport to 987.808[^\n]*\n.*script teleport to 986.756[^\n]*\n.*carry 1-3_garage_key: picked up at update [0-9]+\ncarry arrow over 1-3_garage_key_base added at update [0-9]+\ncarry arrow over 1-3_garage_key removed at update [0-9]+\n")
    # Mission 1-3, the key switch (SCRIPT-362, 20261025-garage-A): 1-3_Garage_Key_Base_Script (what placing the key fires) disables the key and the keyhole
    # and enables 1-3_Garage_Key_Switch (props/prop_switchkey_vertical, kind 58). A in its UseBound (0x17f6f4 message 3) starts the use; the switch base
    # (0x21435c, 0x22c6d0) goes to state 1, then with TriggerBeforeAnim to 2, fires 1-3_Garage_Key_used through the reference step and stays in state 3.
    # The script's WaitForCurrentAnim ends only once the garage door's "open" clip has played. A at update 5 (u0); the switch record reaches state 1 at u32 and
    # 3 at u33, the player enters state 20 at u0, state 76 at u8 and state 1 at u81, as recorded from save states at the key switch of mission 1-3 after
    # placing the key by RPC 20261026-mission13-A-1 (2026-10-26, New 3DS, OpenGL, dt 0.033 0.033; the traps of 0x26b800, 0x2ccc74 and 0x1abbb0, the
    # state reads show the same one update later) and, with the same counts, RPC 20261025-garage-A-1 (dt 0.0333333). The end position is the use point
    # (985.39, 1141.25) of the node char_minifig_use (0x207be4) of prop_keyinhole, the lerp target of A-1; the scripted runs step at dt 0.033. The start (chosen
    # by the port) lies in the UseBound turned by the switch's yaw -45.45 (world centre 985.31 1141.33; 20261027-mission13-B, the lever zone test 0x221570).
    add_test(NAME game-garage-key-1-3 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --save-out "${CMAKE_BINARY_DIR}/game-garage-key-1-3.dat"
      --start "985.6 16.57 1141.5" --facing 180 --call-script 1-3_Garage_Key_Base_Script
      --script "0 0 5;0 0 1 A;0 0 170")
    set_tests_properties(game-garage-key-1-3 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "use marker: 1-3_garage_key_switch shown at update [0-9]+ \\(usegeneric\\)\n.*switch 1-3_garage_key_switch: used at update 5\n.*switch 1-3_garage_key_switch: player state 20 at update 5\n.*switch 1-3_garage_key_switch: player state 76 at update 13\n.*switch 1-3_garage_key_switch: state 1 at update 37\n.*switch 1-3_garage_key_switch: state 3 at update 38\n.*script 1-3_Garage_Key_used started at update [0-9]+\n.*switch 1-3_garage_key_switch: player state 1 at update 86\n.*script 1-3_Garage_Key_used ended at update [0-9]+\n.*player ended at 985.39 16.57 1141.25")
    # The same on the real path of game-key-pickup-1-3: after the pickup, A inside the keyhole's UseBound places the key (its TriggerObject starts
    # 1-3_Garage_Key_Base_Script), A again uses the key switch at the same spot and 1-3_Garage_Key_used opens the garage door.
    add_test(NAME game-key-use-1-3 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --save-out "${CMAKE_BINARY_DIR}/game-key-use-1-3.dat"
      --start "946.4 16.8 1140.0" --facing 0 --call-script 1-3_ConstructionYard
      --script "0 0 1200;at 946.408 16.8 1145.364 0;0 0 1 A;0 0 60;at 987.88 16.56 1143.4 90;0 0 25;at 989.28 16.56 1143.4 90;0 0 1 A;0 0 200;at 986.756 16.624 1142.715 246.7;0 0 5;0 0 1 A;0 0 30;at 986.45 16.57 1141.6 180;0 0 5;0 0 1 A;0 0 40;0 0 1 A;0 0 170")
    set_tests_properties(game-key-use-1-3 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "carry 1-3_garage_key: picked up at update [0-9]+\n.*carry 1-3_garage_key: placed into 1-3_garage_key_base at update [0-9]+\n.*script 1-3_Garage_Key_Base_Script started at update [0-9]+\n.*switch 1-3_garage_key_switch: used at update [0-9]+\n.*switch 1-3_garage_key_switch: state 3 at update [0-9]+\n.*script 1-3_Garage_Key_used started at update [0-9]+\n.*script 1-3_Garage_Key_used ended at update [0-9]+")
    # Mission 1-3 parts crate (SCRIPT-363, 20261025-garage-B): two hits (Y) break 1-3_Lift_Crate_01, its TriggerObject starts 1-3_Got_parts, which enables the
    # carry object 1-3_Generator_Parts_Carry; A picks it up, A inside the use zone of 1-3_BasePlate (teleport next to it) places it, the target's TriggerObject
    # starts 1-3_Hidden_Exhaust_Carry_Target_Script and enables the PlacedObject build-it 1-3_Pull_Switch_Bits, which holding A builds (its brick sounds, then
    # SFX_Generator). Positions are level data (crate and base plate placements); the port's own timings (recorded 20261025-garage-B).
    add_test(NAME game-parts-crate-1-3 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "993.9 16.56 1135.5" --facing 180
      --script "0 0 5;0 0 1 Y;0 0 15;0 0 1 Y;0 0 60;0 0 1 A;0 0 60;at 952.6 16.77 1132.6 90;0 0 5;0 0 1 A;0 0 80;0 0 150 A;0 0 60")
    set_tests_properties(game-parts-crate-1-3 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "1-3_lift_crate_01: destroyed at update [0-9]+\n.*script 1-3_Got_parts started at update [0-9]+.*carry arrow over 1-3_generator_parts_carry added at update [0-9]+.*carry 1-3_generator_parts_carry: picked up at update [0-9]+\ncarry arrow over 1-3_baseplate added at update [0-9]+.*carry 1-3_generator_parts_carry: placed into 1-3_baseplate at update [0-9]+.*script 1-3_Hidden_Exhaust_Carry_Target_Script started at update [0-9]+.*buildit 1-3_pull_switch_bits: stud drop mode.*script SFX_Generator started at update [0-9]+")
    # Mission 1-3 generator scene (SCRIPT-364, 20261026-mission13-B): the real path of game-parts-crate-1-3 up to the build of 1-3_Pull_Switch_Bits, whose
    # final prop is the pull switch 1-3_Generator_Pull_Switch; A inside its class UseBound (centre (0, 1, 1.4), half (1, 1.5, 1) turned by the yaw -75.2 of
    # the switch at (949.252, 16.772, 1129.45), level data), at the oracle's press position 947.956 16.79 1129.206 heading 136.5 (RPC 20261026-mission13-A-1), uses it, its TriggerObject starts 1-3_Generator_Script, which triggers the crane platform, waits for it,
    # stops the girder's rocking, starts particle definition 264 at 1-3_Crane_Girder (PlayParticlesGO), makes the girder guy jump down and ends with
    # QuickSave. The port's own update numbers (recorded 20261026-mission13-B); the platform wait of 103 updates is pinned in prop-rocking-real-data.
    # The crane platform's node goes down to 1-3_Crane_Platform_Target in 102 node moves and ends in state 5 (oracle RPC 20261025-garage-C-1: y 16.6251
    # to 5.9968 from u91 to u192), carrying the girder and the girder guy (20261026-crane-A; the descent is pinned in platform-node-move-real-data).
    # The pull takes the player's USE LERP and state 76 and the switch's 1.6 s use animation (20261026-mission13-A), so the run is 60 updates longer; the
    # order of the pull's lines (state 76, switch state 1, player state 1, switch state 3) is the oracle's, its counts are pinned in the switch test.
    # The level's startup script attaches the girder guy to the girder (AttachObject 0x1bf68c), so he sways and descends with its node.
    # The script triggers the lawn mower 1-3_Lawnmower_01 (a Looped type 3 path platform, ConstantSpeed, speed 0.007): it goes round
    # 1-3_Lawnmower_Path_01 and its first lap ends next to the path's first point (958.46, 16.80, 1061.82) after 145 node moves (the port's own
    # count with its dt, 20261027-mission13-A; the path pose is pinned in platform-path and platform-node-move-real-data).
    add_test(NAME game-generator-1-3 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --save-out "${CMAKE_BINARY_DIR}/game-generator-1-3.dat"
      --start "993.9 16.56 1135.5" --facing 180
      --script "0 0 5;0 0 1 Y;0 0 15;0 0 1 Y;0 0 60;0 0 1 A;0 0 60;at 952.6 16.77 1132.6 90;0 0 5;0 0 1 A;0 0 80;0 0 150 A;0 0 60;at 947.956 16.79 1129.206 136.5;0 0 5;0 0 1 A;0 0 760")
    set_tests_properties(game-generator-1-3 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "character 1-3_ai_girder_guy: attached to 1-3_crane_girder at update 0\n.*prop 1-3_crane_girder: attached to 1-3_crane_platform_dummy at update 0\n.*prop 1-3_crow: attached to 1-3_crane_girder at update 0\n.*carry 1-3_generator_parts_carry: placed into 1-3_baseplate at update [0-9]+.*buildit 1-3_pull_switch_bits: stud drop mode.*switch 1-3_generator_pull_switch: used at update [0-9]+\n.*switch 1-3_generator_pull_switch: player state 76 at update [0-9]+\n.*switch 1-3_generator_pull_switch: state 1 at update [0-9]+\n.*switch 1-3_generator_pull_switch: player state 1 at update [0-9]+\n.*switch 1-3_generator_pull_switch: state 3 at update [0-9]+\n.*script 1-3_Generator_Script started at update [0-9]+\n.*script 1-3_Crow_out started at update [0-9]+\n.*platform 1-3_crane_platform_dummy: travel ended in state 5 at 948.259 5.9968 1147.01 after 102 node moves at update [0-9]+\n.*particles 264 at 1-3_crane_girder started at update [0-9]+\n.*prop 1-3_crane_girder: rocking off at update [0-9]+\n.*platform 1-3_lawnmower_01: lap ended at 958\\.[0-9]+ 16\\.8037 1061\\.[0-9]+ after 145 node moves at update [0-9]+\n.*save: request [0-9]+ written.*\n.*script 1-3_Generator_Script ended at update [0-9]+")
    # Mission 1-3 push cart (SCRIPT-381, 20261026-crane-B): from the start of 1-3_Start_MowerMan_rescue, which enables 1-3_Cart_crate, five hits
    # break the crate, its TriggerObject enables 1-3_Push_Cart_bits (leprops/leprop_buildit, kind 21), A builds it into the push cart 1-3_Push_cart
    # (kind 71), whose back handle (handle02) takes the player walking into the cart on the track behind it (0x2aff3c, states 84 and 85);
    # the push moves the cart to 1-3_Push_cart_to and its TriggerObject starts 1-3_Push_Done. The stick 112 -100 points along the cart's z
    # axis with the camera yaw the rescue scene leaves (the port's own run, 20261026-crane-B); the update numbers are the port's own.
    add_test(NAME game-push-cart-1-3 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --save-out "${CMAKE_BINARY_DIR}/game-push-cart-1-3.dat"
      --start "1022.0 32.5 1026.4" --facing -20 --call-script 1-3_Start_MowerMan_rescue
      --script "0 0 700;at 1024.5 32.3 1026.6 50;0 0 5;0 0 1 Y;0 0 15;0 0 1 Y;0 0 15;0 0 1 Y;0 0 15;0 0 1 Y;0 0 15;0 0 1 Y;0 0 15;0 0 1 Y;0 0 60;0 0 200 A;0 0 30;at 1022.28 33.6 1026.6 -20;0 0 5;112 -100 110;0 0 10")
    set_tests_properties(game-push-cart-1-3 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "prop 1-3_cart_crate: destroyed at update [0-9]+\n.*buildit 1-3_push_cart_bits: stud drop mode.*script 1-3_DisplayPushHint started at update [0-9]+\n.*pushable 1-3_push_cart: grabbed at 1-3_push_cart.handle02, player state 84 at update [0-9]+\n.*pushable 1-3_push_cart: player state 85 at update [0-9]+\n.*pushable 1-3_push_cart: state 1 at update [0-9]+\n.*pushable 1-3_push_cart: state 2 at update [0-9]+\n.*script 1-3_Push_Done started at update [0-9]+")
    # Mission 1-3 ledge lift (SCRIPT-372, SCRIPT-378, 20261027-mission13-B): from the plate of 1-3_Ledge_Platform where the user's save state stands (966.185 17.463
    # 1010.260, facing x; recorded from a save state on the lift trigger of mission 1-3), with 1-3_Ledge_Platform_Switch_Bits_Script called for the built switch, a
    # short step makes the platform the player's ground entity, A uses the pull switch inside its turned UseBound, 1-3_Ledge_Platform_Switch_Bottom_Script raises the
    # platform with SetPlatformSpeed 0.013 to 1-3_Ledge_Platform_Top_Target and the player rides to y 30.318. RPC 20261027-mission13-B-1 (20261027-mission13-servant,
    # 2026-10-28, New 3DS, OpenGL, recorded from a save state on the lift trigger of mission 1-3): from the A press (u0) the node moves in u59..u136 (78 moves) and the
    # player rises with it in the same update, 0.2394 above the node, to 30.3185. The port presses at update 32 and moves in 92..169: one update later from the
    # script's start (SCRIPT-389); its player stands 0.2389 above the node (the start height 17.463 of the save state's read, rounded).
    add_test(NAME game-ledge-lift-1-3 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --save-out "${CMAKE_BINARY_DIR}/game-ledge-lift-1-3.dat"
      --start "966.185 17.463 1010.260" --facing 90 --call-script 1-3_Ledge_Platform_Switch_Bits_Script
      --script "0 0 25;0 60 2;0 0 5;0 0 1 A;0 0 160")
    set_tests_properties(game-ledge-lift-1-3 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "player: ground entity 1-3_ledge_platform at update [0-9]+\n.*switch 1-3_ledge_platform_switch: used at update 32\n.*script 1-3_Ledge_Platform_Switch_Bottom_Script started at update [0-9]+\n.*script 1-3_Lift_Up_Attribute started at update [0-9]+\n.*platform 1-3_ledge_platform: travel ended in state 5 at 966.705 30.0791 1010.82 after 78 node moves at update 169\n.*height 30.318\n.*heights:.* 17.463 17.6284 17.7939 17.9593 ")
    # Mission 1-3 fence vault (TRAV-64, 20261030-qolstores-F): from the original's place in front of the fence that has to be climbed in the lawn mower
    # rescue (1021.043 32.849 1019.872, forward 0.887 0 0.461; recorded from a save state, read by the lead 2026-10-27), B takes the press for
    # 1-3_Parkour_fence_climb: USE LERP (state 20, 0.1 s as the player faces the fence) to the point 1.55 in front of the fence line, state 326 with the
    # clip vault_fence scaled to the fence top, state 5 beyond the fence. Update numbers and positions are the port's own (docs/re/player-traversal-fence-vault.md).
    add_test(NAME game-fence-vault-1-3 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --save-out "${CMAKE_BINARY_DIR}/game-fence-vault-1-3.dat"
      --start "1021.043 32.849 1019.872" --facing 62.5
      --script "0 0 20;0 0 1 B;0 0 90")
    set_tests_properties(game-fence-vault-1-3 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "fence vault: 1-3_parkour_fence_climb takes the B press at update 20\nfence vault: player state 20 at update 20\n.*fence vault: player state 326 at update 25\n.*fence vault: fall at 1023.65 33.7081 1020.71 at update 82\n.*player ended at 1023.65 32.72[0-9]* 1020.71")
    # Mission 1-3 end (SCRIPT-380, 20261026-crane-B): 1-3_Push_Done (called: the push itself is game-push-cart-1-3) enables 1-3_Crane_bits
    # (leprops/leprop_buildit), A in its zone builds it, 1-3_Crane_Build_it_done switches the crane switch on, A on 1-3_Crane_switch (at the user's position
    # on the crane trigger, 1004.431 30.709 1080.533 facing 352.6, recorded from a save state on the crane trigger of mission 1-3 by the lead's reader on
    # 2026-10-27; inside the UseBound centre (0, 1, 1.4) turned by the switch's yaw 157.466 from (1003.68, 30.783, 1082.09), level data) fires
    # 1-3_Start_Rescue_Lawnmower_man, which ends the mission and spawns 1-4_MissionStart. The bridge jump (AISuperJump to 1-3_ChaseBridgeLand) ends at
    # x 987.288 z 1077.79 and the player lands at y 16.80: recorded from a save state on the crane trigger (20261027-mission13-C-2, servant, 2026-10-28,
    # trace with the fixed step: flight ends at (987.288, 25.711, 1077.788), landing at y 16.808).
    add_test(NAME game-crane-1-3 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --save-out "${CMAKE_BINARY_DIR}/game-crane-1-3.dat"
      --start "1002.55 30.8 1079.5" --facing 180 --call-script 1-3_Push_Done
      --script "0 0 130;0 0 120 A;0 0 100;at 1004.431 30.709 1080.533 352.6;0 0 5;0 0 1 A;0 0 850")
    set_tests_properties(game-crane-1-3 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "script 1-3_Push_Done ended at update [0-9]+\n.*buildit 1-3_crane_bits: stud drop mode.*script 1-3_Crane_Build_it_done started at update [0-9]+\n.*switch 1-3_crane_switch: used at update [0-9]+\n.*switch 1-3_crane_switch: state 3 at update [0-9]+\n.*script 1-3_Start_Rescue_Lawnmower_man started at update [0-9]+\n.*player AI move: started at update [0-9]+ from [0-9. ]+\n.*player AI move: ended at update [0-9]+ at 987.288 [0-9.]+ 1077.79, [0-9]+ updates\n.*player AI move: 60 updates after the end the player is at 987.288 16.80[0-9]* 1077.79\n.*script 1-4_MissionStart started at update [0-9]+")
    # Mission 1-2, the dog cage (SCRIPT-111): 1-2_Pringles_Cage_Locked enables the use spot 1-2_Dog_Cage_Use_Spot, A on the user's trigger position
    # (oracle save state at the dog cage trigger of mission 1-2) fires 1-2_Cage_Key_Target_Script through the spot's TriggerObject. The use marker (SCRIPT-292) is shown
    # while the spot is usable (seen on the ground in the oracle in that state) and goes after the use, when 0x236624 switched the zone off.
    add_test(NAME game-use-spot-1-2 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "916.702 64.431 663.724" --facing 0 --call-script 1-2_Pringles_Cage_Locked
      --script "0 0 5;0 0 1 A;0 0 60")
    set_tests_properties(game-use-spot-1-2 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "use marker: 1-2_dog_cage_use_spot shown at update 0.*use spot: 1-2_dog_cage_use_spot triggers 1-2_cage_key_target_script at update [0-9]+.*script 1-2_Cage_Key_Target_Script started at update [0-9]+.*use marker: 1-2_dog_cage_use_spot gone at update [0-9]+")
    # The use marker of the detective spot 1-2_Dtv_DogSpot_02 (SCRIPT-299, kind 182): the police disc once the intro script enabled the spot, from the
    # position of the save state in front of a dog detective spot of mission 1-2 (719.896 37.033 656.910, facing 198.4); the A press on the spot (position of
    # game-detective-1-2) at update 140 puts the spot in detective mode (state 1, message 3 answers 0) and the marker goes.
    add_test(NAME game-use-marker-detective-1-2 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "719.896 37.033 656.910" --facing 198.4 --call-script 1-2_SO_IntroCamOff
      --script "0 0 140;at 720.021 37.035 654.390 289.6;0 0 1 A;0 0 60")
    set_tests_properties(game-use-marker-detective-1-2 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "use marker: 1-2_dtv_dogspot_02 shown at update [0-9]+ \\(usepolice\\).*detective: 1-2_dtv_dogspot_02 state 1 at update [0-9]+.*use marker: 1-2_dtv_dogspot_02 gone at update [0-9]+")
    # PlayCharAnim(Player1, dog_cage_open_minifig) of 1-2_Cage_Key_Target_Script: the runner runs before the player's update, so state 0x50 is
    # entered in the update of the native, the first run of the thread after its start; the hand-back to state 1 follows 89 updates later at
    # dt 0.033 (oracle the save state at the dog cage trigger of mission 1-2: PlayCharAnim and the enter 0x25fbec in update 9, the enter 0x1abbb0 in update 98,
    # tools/oracle/script-anim-order-ctrace.txt). The update numbers are the port's.
    add_test(NAME game-player-script-animation COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "916.702 64.431 663.724" --facing 0 --call-script 1-2_Pringles_Cage_Locked
      --script "0 0 5;0 0 1 A;0 0 100")
    set_tests_properties(game-player-script-animation PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "script 1-2_Cage_Key_Target_Script started at update 5\nplayer state 80 entered at update 6\n.*player state 1 entered at update 95\n")
    # Mission 1-3, the helipad (LEVOBJ-117): the save state on the helipad that needs 300 super bricks stands at 901.454 30.903 1139.463 on 1-3_Helipad, which
    # 1-3_Fade_Blockers enables; the plate's prompt asks for 300 super bricks (RPC reads of the plate's class data, docs/re/super-buildit.md). Without
    # super bricks the A press opens no dialog. The update numbers are the port's.
    add_test(NAME game-super-buildit-prompt COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "901.454 30.903 1139.463" --facing 27 --call-script 1-3_Fade_Blockers
      --script "0 0 5;0 0 1 A;0 0 5")
    set_tests_properties(game-super-buildit-prompt PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "super build-it 1-3_helipad.sbplate: prompt shown at update [0-9]+ \\(needs 300, has 0\\).*\nhud super bricks: shown count 0, number shown\n"
      FAIL_REGULAR_EXPRESSION "super build-it [^\n]*dialog opened")
    # Mission 1-3, the helipad built: FP_Superbrick_01 (+1500, as game-super-brick-freeplay) is picked up, then the player stands where the user's
    # save state on the helipad with enough super bricks stands; A opens "Are you sure?", A (OK) plays superbuildit_helipad and the update after the movie builds:
    # 1500 - 300 = 1200 (the oracle went from 302 to 2), and the holder's TriggerObject starts 1-3_Helipad_Built. Then the fast travel pad of the
    # built helipad (LEVOBJ-118): A where the save state on the fast travel pad of the built helipad stands (906.561 31.445 1138.943, the pad). The oracle (RPC request
    # r40-B-1) held state 0x180 for 39 to 45 vblanks (about 22 updates) and started the Helicopter's idle 124 to 126 vblanks after 0x181 (about 63
    # updates). The update numbers are the port's.
    add_test(NAME game-super-buildit-build COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --call-script 1-3_Fade_Blockers
      --script "at 948.327 95.031 1124.175 0;0 0 40;at 901.203 30.899 1141.268 50;0 0 5;0 0 1 A;0 0 5;0 0 1 A;0 0 330;at 906.561 31.445 1138.943 135;0 0 5;0 0 1 A;0 0 90")
    set_tests_properties(game-super-buildit-build PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "prompt shown at update [0-9]+ \\(needs 300, has 1500\\).*dialog opened.*movie superbuildit_helipad at update [0-9]+.*built at update [0-9]+, super bricks 1200, shown count 1200.*script 1-3_Helipad_Built started.*fast travel 1-3_helipad.sbfasttravel: state 0x180 entered at update 388\n.*fast travel 1-3_helipad.sbfasttravel: state 0x181 entered at update 410\n.*helicopter landed, map ready at update 473\n.*hud super bricks: shown count 1200, number hidden\n")
    # The map's exit (LEVOBJ-120), same route as game-super-buildit-build, then a touch at the centre of `area_exit` (159, 65: its node box (95, 1, 128, 128) read in the oracle, request 20261028-uiport-A-1)
    # through the map buttons (UI-1) once the map is ready (update 473): takeoff and player state 1 in the touch's update, the Helicopter hidden 97 updates later (oracle
    # request 20261025-mission13-A-1: state 1 and takeoff in the same row, 191 vblanks to the vanishing). The update numbers are the port's.
    add_test(NAME game-fast-travel-exit COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --call-script 1-3_Fade_Blockers
      --script "at 948.327 95.031 1124.175 0;0 0 40;at 901.203 30.899 1141.268 50;0 0 5;0 0 1 A;0 0 5;0 0 1 A;0 0 330;at 906.561 31.445 1138.943 135;0 0 5;0 0 1 A;0 0 95;touch 159 65 3;0 0 130")
    set_tests_properties(game-fast-travel-exit PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "map ready at update 473\n.*map buttons: fast travel map closed at update 483\nfast travel: exit touched at update 483\n.*helicopter takeoff at update 483\n.*player state 1 entered at update 483\n.*helicopter hidden at update 580\n")
    # The built helipad stops the player (COLL-2): the route of game-super-buildit-build, then the player stands on the street 1.38 units outside the
    # built prop's edge (local x 9 of the holder 1-3_Helipad, yaw 45, prop half extent 7.622 from the oracle grid read of 20261027-graphics-G-1) and
    # pushes into it with the stick to the right (the camera turned by the route). Without the prop's collision the player walks on to
    # (904.99, 1142.22), 6.4 units further (the build before 20261027-graphics-G); with it he stops at local x 8.34 (replica output).
    add_test(NAME game-helipad-collision COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --call-script 1-3_Fade_Blockers
      --script "at 948.327 95.031 1124.175 0;0 0 40;at 901.203 30.899 1141.268 50;0 0 5;0 0 1 A;0 0 5;0 0 1 A;0 0 330;at 908.682 31.0 1136.821 0;0 0 30;150 0 30")
    set_tests_properties(game-helipad-collision PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "built at update [0-9]+, super bricks 1200.*player ended at 908\\.68[0-9]* 30\\.69[0-9]* 1137\\.7[0-9]*\n")
    # Mission 1-2, the dog released (SCRIPT-293): 1-2_Cage_Key_Target_Script fades the screen to black (FadeScreen 0), the
    # cutscene mode of 1_2_c fades it in again (DCamCutScenePlay handler 0x26fa18, mode enter 0x2fadb8; gdb trace from the user's
    # save state at the dog cage trigger of mission 1-2, docs/re/cutscenes.md), and after the cutscene 1-2_MissionEnd follows with the game picture.
    # The player starts on the use spot (oracle position of the save state at the dog cage trigger of mission 1-2); the update numbers are the port's.
    add_test(NAME game-cage-release-1-2 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "916.702 64.431 663.724" --facing 0 --call-script 1-2_Pringles_Cage_Locked
      --script "0 0 5;0 0 1 A;0 0 760")
    set_tests_properties(game-cage-release-1-2 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "cutscene 1_2_c started at update [0-9]+.*cutscene 1_2_c ended at update [0-9]+.*script 1-2_MissionEnd started at update [0-9]+.*screen brightness 1\n")
    # Mission 1-2, the alley (DETECT-2): the Doorlock script 1-2_Spawn_Doorlock (the waypoint's TriggerScript) passes WaitForCurrentCharAnim on
    # 1-2_Doorlock_Holmes because PlayCharAnim leaves the idle state (state 0x50, docs/re/abilities-detective.md), ends, and then A on the first
    # detective point (oracle save state on the detective point of mission 1-2, the teleport stands in for the walk) switches the detective tool on and enables
    # 1-2_Dtv_DogSpot_01; the script's PositionObject moves Holmes to 1-2_Doorlock_Teleport (SCRIPT-287). The update numbers are the port's, not oracle values.
    add_test(NAME game-doorlock-detective-1-2 COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "738.819 29.389 631.241" --facing 270 --call-script 1-2_Spawn_Doorlock
      --script "0 0 1100;at 723.675 29.768 630.775 291.7;0 0 2;0 0 1 A;0 0 40")
    set_tests_properties(game-doorlock-detective-1-2 PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "PositionObject: 1-2_doorlock_holmes to 1-2_doorlock_teleport at update [0-9]+.*script 1-2_Spawn_Doorlock ended at update [0-9]+.*detective: player state 1 at update [0-9]+.*1-2_dtv_dogspot_01 state 1 at update [0-9]+")
    # 1-2_MissionStart from its first call (update 0): the police car appears with the DestroyProp burst at update 153 and PlayCharAnim ends the
    # script at update 305. Original: first call at update 26, burst at 179, PlayCharAnim at 331 (docs/re/scripts-missions.md 2026-10-14T21:39:21, ctrace of 0x2342b4 from the save state in front of the mission 1-2 start trigger).
    add_test(NAME game-mission-1-2-start-timing COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script 1-2_MissionStart --script "0 0 320")
    set_tests_properties(game-mission-1-2-start-timing PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      PASS_REGULAR_EXPRESSION "vehicle 1-4_police_car: Bullet body, mass scale 1 at update 153\n.*script 1-2_MissionStart ended at update 305")
    # The new-game start of the level (--no-start-cutscenes absent): 1-1_MissionStart plays the intro movie 01_GameIntro, then the in-engine cutscene 1_1_a
    # (docs/re/scripts-missions.md, 1-1_MissionStart order), then ends; this is also the order of MOVIE-23.
    add_test(NAME game-mission-1-1-start COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
      --script "0 0 2600")
    set_tests_properties(game-mission-1-1-start PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 900
      PASS_REGULAR_EXPRESSION "movie 01_GameIntro started at update 0.*movie 01_GameIntro ended at update 2066.*cutscene 1_1_a started at update 2066.*cutscene 1_1_a ended at update 2525.*script 1-1_MissionStart ended at update 2556")
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
    add_test(NAME game-same-level-entry COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --set-variable Lobby_Dcam_Intro=1 --set-variable Lobby2Basement_Lift_Leave=1
      --level levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl --no-start-cutscenes --call-script LIFT_basement
      --start "20.572 1.43 14.83" --facing 0 --script "0 0 20")
    set_tests_properties(game-same-level-entry PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "same-level entry requested by Lobby2Basement at update 0.*same-level entry Lobby2Basement to Basement_Dest_Lobby_P1 at update 1.*player ended at -299\\.37 5\\.7328 6\\.0847")
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
    # Zipline of mission 1-2: the player stands where the oracle save state under the zipline of mission 1-2 has him, jumps (B) and grabs 1-8_Zip_Line05 in the
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
    # The police zipline clips hang Ability_Handcuffs on weaponRight (hand item track of zipline_grab / zipline_slide / zipline_dismount, TRAV-56): shown
    # in the update after the grab (the track's rise at 0.0256 of the grab clip) and hidden in the update 5 after the dismount's start (its fall at 0.1668).
    # Provenance: this script as above, 20261027-graphics-A, 2026-10-27; the clip data in hand-item-real-data.
    add_test(NAME game-script-zipline-handcuffs COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "870.226 66.001 630.961" --facing 102.8 --script "0 0 2;0 0 1 B;0 0 140")
    set_tests_properties(game-script-zipline-handcuffs PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "zipline: grab 1-8_zip_line05 at update 8\n.*hand item ability_handcuffs shown at update 9\n.*hand item ability_handcuffs hidden at update 115\n.*zipline: fall at update 140")
    # The use press at the high anchor is the A press (0x2011b0 -> 0x1fef10 with pressed 1; 0x1b6664 needs it, TRAV-12): standing 0.19 before
    # Mission 1-2: Chase where the save state in front of the table of mission 1-2 has him (oracle position and facing ctrl+8 = -936), 2.06 m south of
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
      PASS_REGULAR_EXPRESSION "health 4 at update 7.*donuttable_14: stud drop mode 0 count 2.*health 3 at update 23.*health 2 at update 39.*health 1 at update 55.*donuttable_14: destroyed at update 71.*script DonutTable_14_Script started at update 71")
    # B on the built trampoline (--call-script runs the script of the finished build): the landing launches at the lowest take-off speed 0.3 every 13
    # updates (oracle save state at the built trampoline of mission 1-2: take-offs 13 updates apart, speed 0.3 without B held).
    add_test(NAME game-script-bounce COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script Trampoline_03_Buildits_Script --start "720.2 32.62 637.47" --facing 0 --script "0 0 3;0 0 2 B;0 0 60")
    set_tests_properties(game-script-bounce PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "bounce: trampoline_03 launches at 0.3 at update 22.*at update 35.*at update 48.*at update 61")
    # B held on the built trampoline: the speed grows by 0.1 per bounce, the take-off at 0.9 (update 143) is the one whose fall update grabs
    # 1-2_FlagPole_01 20 updates later (oracle save state at the built trampoline of mission 1-2, trace t2: take-off at 0.9 in row 139, state 88 in row 159; TRAV-36).
    add_test(NAME game-script-bounce-grab COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --call-script Trampoline_03_Buildits_Script --start "720.2 32.62 637.47" --facing 0 --script "0 0 3;0 0 165 B;0 0 5")
    set_tests_properties(game-script-bounce-grab PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "launches at 0.8 at update 110.*launches at 0.9 at update 143.*climb bar: grab 1-2_flagpole_01 from the fall at update 163")
    # The flag poles crossed from a fall grab of 1-2_FlagPole_01 facing -z as after the bounce (start just below the pole instead of the 170-update
    # bounce): stick left at the idle end flips (oracle f2: stick left flips from -z), B held during the flip jumps at Flip Done, then 95, 96, 95 and 98 to
    # 1-2_FlagPole_Landing_Point and the fall (oracle t1 / f3); t1 lands at 720.49316 37.03597 664.30481 (the port's floor there is 37.0407).
    add_test(NAME game-script-acrobat-poles COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "720.2 42.3 637.47" --facing 180 --script "-120 0 50;0 0 110 B;0 0 3")
    set_tests_properties(game-script-acrobat-poles PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "grab 1-2_flagpole_01 from the fall at update 1\n.*acrobat pole: state 93 at update 5\n.*state 94 at update 39\n.*state 95 at update 60\n.*state 96 at update 85\n.*state 95 at update 110\n.*state 98 at update 135\n.*acrobat pole: fall at update 159\n.*player ended at 720\\.(49|50)[0-9]* 37\\.0[34][0-9]* 664\\.(29|30)")
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
    # Climbing bricks of mission 1-2: the player stands under 1-2_Brick_Grab_08 where the oracle run `drop` from the save state climbing the bricks of mission 1-2 landed,
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
    # normal jump and the rail 1-2_ClimbBar_02 is grabbed 7 updates later, as in the oracle (the save state climbing the bricks of mission 1-2: B in update 37, state 88 in 44).
    add_test(NAME game-script-brick-rail COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "908.8964 64.4281 673.1982" --facing 121.998 --script "0 0 2;0 0 1 B;0 0 40;0 120 2;0 120 1 B;0 120 35;0 120 1 B;0 120 10")
    set_tests_properties(game-script-brick-rail PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "brick grab: jump at update 81\n.*climb bar: grab 1-2_climbbar_02 at update 88")
    # Drain pipe of mission 1-2: the player stands where the oracle save state at the foot of the drain pipe of mission 1-2 has him (0.77 m from 1-5_Drain_Pipe05, facing
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
    # Crane ladder of mission 1-3: the player stands where the oracle save state in front of the crane ladder of mission 1-3 has him (in the use zone of 1-4_Ladder01, facing
    # 54.5 degrees), A mounts in the press update as in the oracle run lad-up, the stick up climbs 0.05 x 30 x dt per update (dt 0.033 here)
    # to the top and the clamber (ladder_dismount, 32 updates at dt 0.033) puts him on the roof at y 30.7.
    add_test(NAME game-script-ladder COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "894.07965 23.62261 1134.047" --facing 54.5 --script "0 0 2;0 0 1 A;0 0 12;0 120 140;0 0 10")
    set_tests_properties(game-script-ladder PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "ladder: use 1-4_ladder01 at update 2\n.*ladder: clamber at update 121\n.*ladder: clambered off at update 153\n.*player ended at 89[45]\.[0-9]+ 30\.[67][0-9]* 113[56]\.[0-9]+\n.*sounds: 26:112 39:112 53:112 65:112 79:112 92:112 105:112 118:112 129:288 141:178 142:410\n")
    # The sounds line pins the clip sound rule against the shipped clips (not an oracle recording, TRAV-61): the pulses of ladder_up (112, every
    # half cycle of 13 updates), ladder_dismount 288 at its pulse, the footstep 178 and the effort sound 410 (slot 7) of its sound tracks.
    # The same place, the stick towards the ladder (the stick value stands for the camera yaw of this runtime): the walk-in 0x206668 mounts
    # when the player's box touches the ladder (TRAV-S7) in its use zone.
    add_test(NAME game-script-ladder-walk-in COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "894.07965 23.62261 1134.047" --facing 54.5 --script "0 0 2;60 104 20;0 0 3")
    set_tests_properties(game-script-ladder-walk-in PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      PASS_REGULAR_EXPRESSION "ladder: walk-in 1-4_ladder01 at update 3\n")
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
    # Pause screen on the real path: Start in gameplay opens it in the next update, a stick push right turns the round menu to Options (the
    # stick feeds the D-pad, 0x2289b8), A on Options opens the Options page (UI-4), the stick turns it to Music and a push down lowers the
    # music volume of the new game's slot from 8 to 7 (0x31af84); B then saves the options (UI-S10) and page 0 comes back; Start closes the
    # screen after TransitionOff_Round (16 updates of 0.033 s).
    # Every pause update adds one play tick as the pause mode's update 0x300378 does (0x300444, SAVE-19).
    # game-options-reload continues from that file: the loading screen (0x303a40) takes the volumes of the slot at options +2.
    add_test(NAME game-pause-menu-clean COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/test-xdg/game-pause-menu")
    set_tests_properties(game-pause-menu-clean PROPERTIES FIXTURES_SETUP pause_menu_clean)
    add_test(NAME game-pause-menu COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "862.5931 16.1988 1060.5875" --script "0 0 5;0 0 1 S;0 0 40;145 0 1;0 0 20;0 0 1 A;0 0 50;145 0 1;0 0 20;0 -145 1;0 0 5;0 0 1 B;0 0 40;0 0 1 S;0 0 20")
    set_tests_properties(game-pause-menu PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
      FIXTURES_REQUIRED pause_menu_clean FIXTURES_SETUP pause_menu_options
      PASS_REGULAR_EXPRESSION "pause menu opened at update 6.*pause menu: options saved to [^\n]*, slot 0 sound volume 10, music volume 7, invert camera off at update 145.*pause menu closed at update 202, play ticks 202")
    add_test(NAME game-options-reload COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --save "${CMAKE_BINARY_DIR}/test-xdg/game-pause-menu/chase/savegame.dat" --slot 0
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --script "0 0 2")
    set_tests_properties(game-options-reload PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 300
      FIXTURES_REQUIRED pause_menu_options
      PASS_REGULAR_EXPRESSION "save: loaded in slot 0[^\n]*\n.*loading screen: sound volume 10, music volume 7\n")
    # The award viewer of the police station (UI-12): A on the use spot Use_AwardViewer (its position in the level data, room3) starts
    # Use_AwardViewerScript, whose AwardViewer native opens the viewer in the next update with the new-game counts (sums of TablesSuperCounters and
    # TablesLevels); Start does nothing while it is open (its mode's update 0x2f26c8 has no Start test, so no pause screen opens), Right turns to the
    # first level, B closes it after TransitionOff_Round (0.5 s, update 78 to 94).
    add_test(NAME game-award-viewer COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg" --save-out "${CMAKE_BINARY_DIR}/game-award-viewer-save.dat"
      --level levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl --no-start-cutscenes --set-variable Lobby_Dcam_Intro=1
      --start "-291.73 11.1294 -16.5349" --script "0 0 5;0 0 1 A;0 0 40;0 0 1 S;0 0 10;145 0 1;0 0 20;0 0 1 B;0 0 30")
    set_tests_properties(game-award-viewer PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" LABELS "needs-game-data;slow"
      PASS_REGULAR_EXPRESSION "use spot: use_awardviewer triggers use_awardviewerscript at update 5.*script Use_AwardViewerScript started at update 5.*award viewer opened at update 7, bronze 0/260, silver 0/46, gold 0/7.*award viewer closed at update 94"
      FAIL_REGULAR_EXPRESSION "pause menu opened")
    add_test(NAME game-script-hud COMMAND chase-game
      --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
      --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
      --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
      --start "862.5931 16.1988 1060.5875" --window 400x480 --script "0 0 70" --screenshot game-script-hud.bmp)
    set_tests_properties(game-script-hud PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
      PASS_REGULAR_EXPRESSION "studs: counter 10, 0 collecting, 1 collected.*hud: 4 hearts, stud display 10, bar state 2.*saved game-script-hud.bmp")
    # Room streaming in the level: starting in Room7 (position and view of the oracle save state at the police station entrance after mission 1-1 +1 update, 9 rooms
    # loaded), walking east to west enters Room8, Room6 unloads in that update and Room16 finishes loading 9 updates later
    # (the update numbers are the port's own walk, the rule is pinned by room-streaming-real-data). Room16 finishes loading with its
    # nearest box point 120.0 from the camera (oracle the save state at the police station entrance after mission 1-1, same walk: 120.3, docs/re/rendering-fog.md RENDER-31).
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
add_executable(stud-callers-test tests/stud_callers_test.cpp)
target_link_libraries(stud-callers-test PRIVATE chase_runtime chase_assets)
target_compile_options(stud-callers-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME stud-callers COMMAND stud-callers-test)
add_test(NAME stud-callers-real-data COMMAND stud-callers-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(stud-callers-real-data PROPERTIES SKIP_RETURN_CODE 77)

add_executable(stud-drop-classes-test tests/stud_drop_classes_test.cpp)
target_link_libraries(stud-drop-classes-test PRIVATE chase_runtime chase_assets)
target_compile_options(stud-drop-classes-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME stud-drop-classes COMMAND stud-drop-classes-test)
add_test(NAME stud-drop-classes-real-data COMMAND stud-drop-classes-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(stud-drop-classes-real-data PROPERTIES SKIP_RETURN_CODE 77)
add_executable(prop-hit-sequence-test tests/prop_hit_sequence_test.cpp)
target_link_libraries(prop-hit-sequence-test PRIVATE chase_runtime chase_assets)
target_compile_options(prop-hit-sequence-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME prop-hit-sequence COMMAND prop-hit-sequence-test)
add_test(NAME prop-hit-sequence-real-data COMMAND prop-hit-sequence-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(prop-hit-sequence-real-data PROPERTIES SKIP_RETURN_CODE 77)
add_executable(super-brick-test tests/super_brick_test.cpp)
target_link_libraries(super-brick-test PRIVATE chase_runtime chase_assets)
target_compile_options(super-brick-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME super-brick COMMAND super-brick-test)
add_test(NAME super-brick-real-data COMMAND super-brick-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(super-brick-real-data PROPERTIES SKIP_RETURN_CODE 77)
add_executable(super-brick-display-test tests/super_brick_display_test.cpp)
target_link_libraries(super-brick-display-test PRIVATE chase_frontend chase_runtime chase_assets)
target_compile_options(super-brick-display-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME super-brick-display COMMAND super-brick-display-test)
add_test(NAME super-brick-display-real-data COMMAND super-brick-display-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(super-brick-display-real-data PROPERTIES SKIP_RETURN_CODE 77)
add_executable(debris-pickup-test tests/debris_pickup_test.cpp)
target_link_libraries(debris-pickup-test PRIVATE chase_runtime chase_assets)
target_compile_options(debris-pickup-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME debris-pickup COMMAND debris-pickup-test)
add_executable(prop-bouncer-test tests/prop_bouncer_test.cpp)
target_link_libraries(prop-bouncer-test PRIVATE chase_runtime chase_assets)
target_compile_options(prop-bouncer-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME prop-bouncer COMMAND prop-bouncer-test)
add_test(NAME prop-bouncer-real-data COMMAND prop-bouncer-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(prop-bouncer-real-data PROPERTIES SKIP_RETURN_CODE 77)
add_executable(prop-wall-side-test tests/prop_wall_side_test.cpp)
target_link_libraries(prop-wall-side-test PRIVATE chase_runtime chase_assets)
target_compile_options(prop-wall-side-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME prop-wall-side COMMAND prop-wall-side-test)
add_test(NAME prop-wall-side-real-data COMMAND prop-wall-side-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(prop-wall-side-real-data PROPERTIES SKIP_RETURN_CODE 77)

add_executable(camera-lock-census tools/camera_lock_census.cpp)
target_link_libraries(camera-lock-census PRIVATE chase_runtime chase_assets)
target_compile_options(camera-lock-census PRIVATE -Wall -Wextra -Wpedantic)
add_executable(player-script-animation-test tests/player_script_animation_test.cpp)
target_link_libraries(player-script-animation-test PRIVATE chase_runtime chase_assets)
target_compile_options(player-script-animation-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME player-script-animation COMMAND player-script-animation-test)
add_test(NAME player-script-animation-real-data COMMAND player-script-animation-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(player-script-animation-real-data PROPERTIES SKIP_RETURN_CODE 77)
target_sources(chase_runtime PRIVATE src/runtime/traffic_manager.cpp)
add_executable(traffic-manager-test tests/traffic_manager_test.cpp)
target_link_libraries(traffic-manager-test PRIVATE chase_runtime)
target_compile_options(traffic-manager-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME traffic-manager COMMAND traffic-manager-test)
target_sources(chase_runtime PRIVATE src/runtime/traffic_system.cpp)
add_executable(traffic-system-test tests/traffic_system_test.cpp)
target_link_libraries(traffic-system-test PRIVATE chase_runtime chase_assets)
target_compile_options(traffic-system-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME traffic-system COMMAND traffic-system-test)
add_test(NAME traffic-system-real-data COMMAND traffic-system-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(traffic-system-real-data PROPERTIES SKIP_RETURN_CODE 77)
set_property(TEST traffic-system-real-data APPEND PROPERTY LABELS needs-game-data)
if(TARGET chase-game)
  # TRAFFIC-1: the traffic manager in the port's run from the level start (`chase-game ... --no-start-cutscenes --script "0 0 300"`): the room set-up
  # spawns cars at pregen points around the player (prefill band 0 .. 76), the cars stand still, and one leaves per update after its grace counter ran
  # out beyond the removal radius 50. The update numbers and points are the port's own run (stand-in rules TRAFFIC-S1, TRAFFIC-S7).
  add_test(NAME game-traffic COMMAND chase-game
    --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
    --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
    --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
    --script "0 0 300")
  set_tests_properties(game-traffic PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
    PASS_REGULAR_EXPRESSION "traffic: 10 cars, limit 10, 649 spawn points, 57 routes, 648 floor cells, spawn data loaded\n.*traffic car Traffic05: spawned at point 419 \\(902.337 18.5559 1061.92\\), fade 255 at update 0\n.*traffic car Traffic09: removed at update 67")
  # SCRIPT-230: 1-4_RobberCaught calls EnableTrafficAndPedestrians(0) at its start (every live car leaves in one update) and SetTrafficMaxVehicles(10) later.
  add_test(NAME game-traffic-natives COMMAND chase-game
    --fib "${CHASE_ROMFS_DIR}/lego_city3ds.fib" --names "${CHASE_ROMFS_DIR}/lego_city3ds.csv"
    --config "${CMAKE_BINARY_DIR}/config/controls.cfg"
    --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes
    --call-script 1-4_RobberCaught --script "0 0 420")
  set_tests_properties(game-traffic-natives PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy"
    PASS_REGULAR_EXPRESSION "traffic car Traffic01: removed at update 99\n.*traffic car Traffic08: removed at update 99\n.*traffic: limit 10 at update 402")
  set_property(TEST game-traffic game-traffic-natives APPEND PROPERTY LABELS needs-game-data slow)
endif()

target_sources(chase_runtime PRIVATE src/runtime/ai_driver_input.cpp)
add_executable(ai-driver-input-test tests/ai_driver_input_test.cpp)
target_link_libraries(ai-driver-input-test PRIVATE chase_runtime)
target_compile_options(ai-driver-input-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ai-driver-input COMMAND ai-driver-input-test)

if(BUILD_TESTING)
  add_test(NAME test-labels COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/check_test_labels.py" "${CMAKE_BINARY_DIR}")
endif()

get_property(chase_all_tests DIRECTORY PROPERTY TESTS)
foreach(test_name ${chase_all_tests})
  set_property(TEST ${test_name} APPEND PROPERTY ENVIRONMENT "XDG_DATA_HOME=${CMAKE_BINARY_DIR}/test-xdg/${test_name}")
endforeach()

foreach(slow_test game-key-pickup-1-3 game-parts-crate-1-3 game-generator-1-3 game-ledge-lift-1-3 game-fence-vault-1-3 game-push-cart-1-3 game-crane-1-3 game-key-use-1-3 game-debris-super-pickup game-cutscene-dog-end game-super-buildit-build game-helipad-collision game-fast-travel-exit game-super-brick-roof game-doorlock-detective-1-2 game-cage-release-1-2 game-mission-1-1 game-mission-1-1-start game-vehicle-drive game-vehicle-room game-pedestrian-rooms game-vehicle-hit game-vehicle-smash game-vehicle-break game-director-hold game-director-release game-van-escort game-pause-menu waypoint-marker-shot audio-real-data reflection-real-data)
  if(TEST ${slow_test})
    set_property(TEST ${slow_test} APPEND PROPERTY LABELS slow)
  endif()
endforeach()

foreach(game_data_test
  frontend-real-data draw-order-real-data script-text-pages-real-data hud-real-data hud-parts-real-data audio-real-data voice-params-real-data pedestrian-fade-real-data pedestrian-system-real-data cutscene-cast
  node-draw-matrix-real-data script-text-real-data waypoint-global-real-data waypoints-real-data hand-item-real-data detective-real-data
  use-spot-real-data use-marker-real-data script-starters-real-data carry-real-data scanner-real-data scanner-overlay-data zipline-real-data
  nested-props-real-data entity-ids-real-data zipline-pole-real-data zipline-rope-real-data brick-grab-real-data climb-bar-real-data acrobat-pole-real-data pole-real-data
  ladder-real-data fence-vault-real-data carry-animation-real-data lever-use-real-data nested-object-collision-real-data super-buildit-real-data layout-text-real-data layout-scale-real-data fast-travel-pad-real-data map-buttons-real-data pause-menu-real-data buildit-real-data
  prop-animation-real-data animal-props-real-data cat-rescue-real-data prop-parts-real-data vehicle-boarding-real-data vehicle-drive-real-data
  floor-graph-real-data vehicle-collision-real-data vehicle-box-contact-real-data vehicle-sphere-sweep-real-data vehicle-prop-smash-real-data
  destroy-effects-real-data character-death-real-data player-attack-real-data attack-sounds-real-data vehicle-world-real-data vehicle-rest-real-data
  vehicle-wheels-real-data vehicle-hud-real-data vehicle-real-data objectives-real-data object-kinds progress-readme priorities-current
  camera-records-real-data locomotion-real-data streaming-real-data room-streaming-real-data collision-real-data fog-real-data lighting-real-data reflection-real-data
  sky-real-data shadow-real-data level-start-real-data dcam-real-data npc-idle-real-data pedestrian-limit-real-data ai-real-data nav-path-real-data
  script-triggers-real-data script-dialog-real-data natives-world-real-data natives-party-real-data natives-objects-real-data
  natives-cutscene-real-data natives-animation-real-data natives-progress-real-data script-call-targets-real-data scene-change-real-data
  same-level-entry-real-data door-real-data fall-real-data jump-real-data slide-real-data camera-real-data camera-blend-real-data
  camera-vehicle-real-data camera-sweep-real-data save-real-data test-save-build slot-progress-real-data save-writer-real-data
  new-game-slot-real-data level-state-real-data pregen-derive-real-data pregen-traffic-real-data assets-real-data particles-real-data
  viewer-real-smoke viewer-cutscene-smoke debug-overlay-off debug-overlay-on debug-overlay-area tip-text-shot tip-text-panel waypoint-marker-shot
  waypoint-marker-position police-entrance-shot police-entrance-no-disc game-real-smoke game-save-continue game-save-hud-studs
  game-save-continue-empty-slot front-game-smoke front-overlay-off front-overlay-on front-overlay-area front-game-handoff frontshot-real-smoke
  game-script-run game-script-walk game-script-camera game-script-ledge game-vehicle-drive game-van-escort game-cutscene-glass-hidden
  game-cutscene-dog-prop game-super-brick-freeplay game-save-new-game-clean game-save-new-game game-save-reload game-super-brick-roof
  game-cutscene-dog-end game-vehicle-room game-pedestrian-rooms game-vehicle-hit game-vehicle-break game-vehicle-smash game-debris-super-pickup
  game-mission-1-1 game-detective-1-2 game-key-pickup-1-3 game-parts-crate-1-3 game-generator-1-3 game-ledge-lift-1-3 game-fence-vault-1-3 game-push-cart-1-3 game-crane-1-3 game-garage-key-1-3 game-key-use-1-3 game-use-spot-1-2 game-use-marker-detective-1-2 game-player-script-animation game-super-buildit-prompt
  game-super-buildit-build game-helipad-collision game-fast-travel-exit game-cage-release-1-2 game-doorlock-detective-1-2 game-mission-1-2-start-timing game-mission-1-1-start game-script-dialog
  game-same-level-entry game-scene-change-exit game-scene-change-entry game-director-hold game-director-release game-script-zipline game-script-zipline-handcuffs
  game-script-attack-table game-script-wobbly-table game-script-bounce game-script-bounce-grab game-script-acrobat-poles game-script-zipline-use-a
  game-script-zipline-use-b game-script-brick-grab game-script-brick-rail game-script-pole game-script-ladder game-script-ladder-walk-in
  game-script-jump game-pause-menu-clean game-pause-menu game-options-reload game-script-hud game-room-streaming game-stream-room-in soundtest-real-smoke soundtest-id-smoke fmv-real-smoke
  prop-wobbly-real-data stud-callers-real-data stud-drop-classes-real-data prop-hit-sequence-real-data super-brick-real-data
  super-brick-display-real-data prop-bouncer-real-data prop-wall-side-real-data player-script-animation-real-data)
  if(TEST ${game_data_test})
    set_property(TEST ${game_data_test} APPEND PROPERTY LABELS needs-game-data)
  endif()
endforeach()

foreach(reference_trigger_data_test reference-trigger-real-data)
  if(TEST ${reference_trigger_data_test})
    set_property(TEST ${reference_trigger_data_test} APPEND PROPERTY LABELS needs-game-data)
  endif()
endforeach()

target_sources(chase_runtime PRIVATE src/runtime/traffic_route.cpp)
add_executable(traffic-route-test tests/traffic_route_test.cpp)
target_link_libraries(traffic-route-test PRIVATE chase_runtime chase_assets)
target_compile_options(traffic-route-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME traffic-route COMMAND traffic-route-test)
add_test(NAME traffic-route-real-data COMMAND traffic-route-test
  "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(traffic-route-real-data PROPERTIES SKIP_RETURN_CODE 77 LABELS needs-game-data)

add_executable(prop-rocking-test tests/prop_rocking_test.cpp)
target_link_libraries(prop-rocking-test PRIVATE chase_runtime)
target_compile_options(prop-rocking-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME prop-rocking COMMAND prop-rocking-test)
add_executable(node-attach-test tests/node_attach_test.cpp)
target_link_libraries(node-attach-test PRIVATE chase_runtime)
target_compile_options(node-attach-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME node-attach COMMAND node-attach-test)
add_executable(attach-children-test tests/attach_children_test.cpp)
target_link_libraries(attach-children-test PRIVATE chase_runtime)
target_compile_options(attach-children-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME attach-children COMMAND attach-children-test)
add_executable(prop-rocking-real-test tests/prop_rocking_real_test.cpp)
target_link_libraries(prop-rocking-real-test PRIVATE chase_runtime chase_assets)
target_compile_options(prop-rocking-real-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME prop-rocking-real-data COMMAND prop-rocking-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(prop-rocking-real-data PROPERTIES SKIP_RETURN_CODE 77 LABELS needs-game-data)
add_executable(platform-node-move-test tests/platform_node_move_test.cpp)
target_link_libraries(platform-node-move-test PRIVATE chase_runtime)
target_compile_options(platform-node-move-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME platform-node-move COMMAND platform-node-move-test)
add_executable(platform-riding-test tests/platform_riding_test.cpp)
target_link_libraries(platform-riding-test PRIVATE chase_runtime)
target_compile_options(platform-riding-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME platform-riding COMMAND platform-riding-test)
add_executable(platform-riding-real-test tests/platform_riding_real_test.cpp)
target_link_libraries(platform-riding-real-test PRIVATE chase_runtime chase_assets)
target_compile_options(platform-riding-real-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME platform-riding-real-data COMMAND platform-riding-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(platform-riding-real-data PROPERTIES SKIP_RETURN_CODE 77 LABELS needs-game-data)
add_executable(platform-path-test tests/platform_path_test.cpp)
target_link_libraries(platform-path-test PRIVATE chase_runtime)
target_compile_options(platform-path-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME platform-path COMMAND platform-path-test)
add_executable(platform-node-move-real-test tests/platform_node_move_real_test.cpp)
target_link_libraries(platform-node-move-real-test PRIVATE chase_runtime chase_assets)
target_compile_options(platform-node-move-real-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME platform-node-move-real-data COMMAND platform-node-move-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(platform-node-move-real-data PROPERTIES SKIP_RETURN_CODE 77 LABELS needs-game-data)

add_executable(script-particles-test tests/script_particles_test.cpp)
target_link_libraries(script-particles-test PRIVATE chase_runtime)
target_compile_options(script-particles-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME script-particles COMMAND script-particles-test)

add_executable(pushable-test tests/pushable_test.cpp)
target_link_libraries(pushable-test PRIVATE chase_runtime)
target_compile_options(pushable-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME pushable COMMAND pushable-test)
add_executable(mission13-end-test tests/mission13_end_test.cpp)
target_link_libraries(mission13-end-test PRIVATE chase_runtime)
target_compile_options(mission13-end-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME mission13-end COMMAND mission13-end-test)
add_executable(pushable-real-test tests/pushable_real_test.cpp)
target_link_libraries(pushable-real-test PRIVATE chase_runtime chase_assets)
target_compile_options(pushable-real-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME pushable-real-data COMMAND pushable-real-test "${CHASE_ROMFS_DIR}/lego_city3ds.fib" "${CHASE_ROMFS_DIR}/lego_city3ds.csv")
set_tests_properties(pushable-real-data PROPERTIES SKIP_RETURN_CODE 77 LABELS needs-game-data)
