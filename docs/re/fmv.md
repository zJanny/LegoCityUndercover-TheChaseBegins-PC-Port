# FMV (Mobiclip) and subtitles

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-13T11:05+02:00 — FMV player with subtitles: WIP, not merged

- Branch worktree-agent-a93143c61d56bb50d, commit 2be2b23 (WIP). Based on origin/main 6c26639, not rebased; CMakeLists.txt and assets.hpp will conflict (keep both sides).
- The FMVs are romfs/cutscenes/*.moflex (49 files): Mobiclip video 400x240 at 30 fps and IMA-ADPCM audio at 22050 Hz stereo. System FFmpeg 8 decodes them. Subtitles are fib cutscenes/<video>_<lang>.srt (17 videos × 12 languages, SubRip with the .loc escape bytes). The German videos only have <stem>_de_de.srt.
- Contents of the branch:
  - parseSubtitles (src/assets/subtitles.cpp; strict SubRip, decoded with decodeText) with a synthetic test.
  - Real-data pins: 204 files, 2921 cues, 3560 lines, 6486170 ms (Python regex census) and a decoded-text FNV-1a of 0xe37a66e1 (recorded from C++; that pin has not been run).
  - chase-fmv (SDL2 + libavformat/libavcodec/libswscale/libswresample), drawing subtitles in frontendfont. The headless screenshot of 01_gameintro at 3 s was checked visually.
- Open before merging:
  - `chase-fmv splash5.moflex` did not exit within 120 s (end-of-stream bug).
  - Live A/V sync, audio output and Escape/close are untested.
  - The fmv-real-smoke ctest was not registered (probably the configure-time EXISTS check through the artifacts symlink).
  - docs/fmv-player.md and the FFmpeg line in docs/building.md are not written.
  - Which font and position the game uses for subtitles is unknown.

## 2026-10-13T11:40+02:00 — FMV player finished and merged

- Rebased the WIP branch onto main. A missing space in `set(CHASE_ROMFS_DIR"...")` had kept fmv-real-smoke from being registered; it is fixed.
- End-of-playback bug: the clock follows the audio queue, and the audio of splash5 ends at 6.362 s. The old end test still needed now > last frame (6.333 s) + 0.1 s, so it never passed. With audio, playback now ends when decoding is finished, no frames are pending and the audio queue is empty. Without audio, it ends one frame after the last frame. splash5 now exits after 6.4 s with dummy audio and with --no-audio.
- Measured: splash5 has 191 video and 191 audio packets (6.36 s); 01_gameintro has 4088 video and 2044 audio packets (68.2 s). Packets are interleaved.
- Checked visually: Russian subtitles render in frontendfont ("Все дороги ведут в него." at 4.5 s). `--lang de` on 01_gameintro_de finds 01_gameintro_de_de.srt.
- The decoded-subtitle FNV-1a pin 0xe37a66e1 now passes in assets-real-test.
- Added docs/fmv-player.md, FFmpeg in docs/building.md, and links in README.md and docs/status.md.
- Not tested: Escape or window close with a real display, and A/V sync on real audio hardware.
- ctest PASS 9/9.
