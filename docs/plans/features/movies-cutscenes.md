# Movies and in-engine cutscenes

## 1. Scope

Mobiclip `.moflex` movies with subtitles (FMV player and the shared `chase_movie` decoder) and the in-engine `csdata` cutscenes (tracks, cameras, actors, soundtrack). Where movies are shown in the game flow is [front-end.md](front-end.md).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. Movies play in `chase-fmv` and in the front end; cutscenes play in `chase-viewer --cutscene` only, not in the game flow.

## 3. Verified behaviour

- 49 `.moflex` movies (400x240, 30 fps, IMA-ADPCM 22050 Hz stereo; the intro movie is 59.94 fps) decode with system FFmpeg; subtitles `cutscenes/<video>_<lang>.srt` (204 files, 2921 cues) parse strictly.
- 47 of 49 movies alternate left/right eye frames (header layout byte 0); `chase_movie` keeps the first picture of each pair, as the oracle shows; `frontend-real-data` pins 191/2044 pictures.
- Cutscene tracks evaluate as cubic Bezier curves in normalised time; cameras, actor poses and joint orientations match the references; cutscenes play at 30 frames per second (data evidence).

## 4. Stand-ins

- MOVIE-S1 The cutscene Bezier curve shape is a data fit, not traced in code.
- MOVIE-S2 The 30 frames per second cutscene rate is inferred from the soundtrack lengths, not from the timer code.
- MOVIE-S3 The camera matrix flip for GL is inferred; its consumer 0x241540 was not traced.

## 5. Open items

- [ ] MOVIE-1 — Meaning of `.moflex` layout values other than 0 and 6 and of the two other skipped header bytes (01 01 in all files); where the game's Mobiclip player reads the layout and selects the eye (not traced in the ARM code); whether the second eye of the pair is the right eye (only matters for a stereoscopic display, which the port does not target). evidence: [fmv.md §2026-10-13T20:58](../../re/fmv.md#ts-20261013-205838).
- [ ] MOVIE-2 — In-engine cutscenes are not played in the game flow (the front end's top screen under the title card is black). blocks: FE-4, SCRIPT-29.
- [ ] MOVIE-3 — 113 of 397 actor animations have a different frame count from their camera (the viewer holds their last frame); the meaning of camera fields 7-11, the fov axis and the base value of the actor scale are unknown. evidence: [cutscenes.md §2026-10-13T02:30](../../re/cutscenes.md#ts-20261013-0230).
- [ ] MOVIE-4 — The extra (root motion) curve use in cutscenes; the 12 cutscene folders without soundtrack and the outliers 6_4_a, 2_11_a, 1_4_c. evidence: [cutscenes.md §2026-10-13T03:20](../../re/cutscenes.md#ts-20261013-0320).

## 6. Findings summary

- FMV container, audio, subtitles: [fmv.md §2026-10-13T11:40](../../re/fmv.md#ts-20261013-1140); stereoscopic pairs: [fmv.md §2026-10-13T20:58](../../re/fmv.md#ts-20261013-205838).
- Cutscene tracks, cameras, actors, joint orientations: [cutscenes.md §2026-10-13T02:30](../../re/cutscenes.md#ts-20261013-0230); playback rate and soundtrack: [cutscenes.md §2026-10-13T03:20](../../re/cutscenes.md#ts-20261013-0320).

## 7. Code and tests

- `src/movie/` (`chase_movie`), `src/fmv.cpp` (`chase-fmv`), `src/assets/subtitles.cpp`, `src/assets/cutscene.cpp`; `chase-viewer --cutscene`.
- CTest: `fmv-real-smoke`, `viewer-cutscene-smoke`, `frontend-real-data`.
