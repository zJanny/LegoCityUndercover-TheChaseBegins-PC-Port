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

## 2026-10-13T20:58:38+02:00 — Stereoscopic movies: left/right eye frames alternate

**Correction** of the first entry ("Mobiclip video 400x240 at 30 fps") and of docs/re/front-end.md 2026-10-13T20:46:07 ("01_gameintro … 59.94 frames per second"): 47 of the 49 movies are stereoscopic. Their video stream holds the two eye pictures as alternating frames, so `01_gameintro` is 2044 picture pairs at 29.97 pairs per second (FFmpeg reports 4088 frames at 59.94 fps). Showing every decoded frame made the picture jump left and right (reported by the user in chase-game's intro).

- Header: the first record after the sync block (0x4c32, 2 skipped bytes, u64 timestamp, u16 size; FFmpeg `libavformat/moflex.c` `moflex_read_sync`) is type 3 (video), size 13: stream index, codec 0, u16 rate denominator… as FFmpeg reads it, then 3 bytes FFmpeg skips. The last of them (file offset 28) is the layout: 0 in 47 files (30 at 60000/1001, 16 superbuildit at 60/1, `superbuildit_shuttleboosters` at 30/1 with 20 identical frames), 6 only in `splash5.moflex` (30 fps, one picture per frame). Census: `xxd -s 14 -l 15 -p` over `artifacts/romfs/romfs/cutscenes/*.moflex`.
- Pairs: PSNR of neighbouring decoded frames of 01_gameintro (frames 1200..1205) is 17.2..17.7 dB, of frames two apart 29.5..30.3 dB (also 07_intro_farm 18.3 vs 34.4, superbuildit_windmill 20.5 vs 34.6).
- Eye shown by the original on the (2D) top screen: the first frame of each pair. The oracle intro captures `artifacts/review/front-end/oracle/intro/intro02..09.png` each match an even frame best (3060, 3198, 3338, 3476, 3614, 3750, 3888, 4028; mean abs distance on 100x60 grey 6.6..8.9) with the odd neighbours at 16.6..20.8 (script `artifacts/review/fmv-stereo/match_eye.py`, not committed).
- Native: `chase::movie::Decoder` reads the layout byte and keeps only the first picture of each pair for layout 0 (`Decoder::eyePairs`). Test `frontend-real-data` (`eyePairs`): splash5 191 pictures at 1/30 s, 01_gameintro 2044 pictures at 2002/60000 s. `chase-fmv --time 51.054` now shows frame 3060 (PSNR 28.8 dB against the oracle capture intro02).

Unknowns:
- Meaning of layout values other than 0 and 6, and of the two other skipped bytes (01 01 in all files).
- Where the game's Mobiclip player reads the layout and selects the eye (not traced in the ARM code; the rule above is from the header census and the oracle).
- Whether the second eye of the pair is the right eye (only matters for a stereoscopic display, which the port does not target).
