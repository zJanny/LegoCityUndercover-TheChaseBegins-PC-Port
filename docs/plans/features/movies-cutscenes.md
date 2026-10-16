# Movies and in-engine cutscenes

## 1. Scope

Mobiclip `.moflex` movies with subtitles (FMV player and the shared `chase_movie` decoder) and the in-engine `csdata` cutscenes (tracks, cameras, actors, soundtrack). Where movies are shown in the game flow is [front-end.md](front-end.md).

## 2. Status

IN_PROGRESS, updated 2026-10-15T07:56:00+02:00. Movies play in `chase-fmv` and in the front end; in-engine cutscenes play in `chase-viewer --cutscene` and in `chase-game` (DCamCutScene natives, `1_1_b` compared with the oracle); `PlayCutscene` movies play in the game, the level start plays the intro movie and `1_1_a`, and cutscenes fade to and from black.

## 3. Verified behaviour

- 49 `.moflex` movies (400x240, 30 fps, IMA-ADPCM 22050 Hz stereo; the intro movie is 59.94 fps) decode with system FFmpeg; subtitles `cutscenes/<video>_<lang>.srt` (204 files, 2921 cues) parse strictly.
- 47 of 49 movies alternate left/right eye frames (header layout byte 0); `chase_movie` keeps the first picture of each pair, as the oracle shows; `frontend-real-data` pins 191/2044 pictures.
- Cutscene tracks evaluate as cubic Bezier curves in normalised time; cameras, actor poses and joint orientations match the references; cutscenes play at 30 frames per second (data evidence).

## 4. Stand-ins

- MOVIE-S1 The cutscene Bezier curve shape is a data fit, not traced in code.
- MOVIE-S2 The 30 frames per second cutscene rate is inferred from the soundtrack lengths, not from the timer code.
- MOVIE-S3 The camera matrix flip for GL is inferred; its consumer 0x241540 was not traced.

- MOVIE-S10 The length of a cutscene is max(camera frames / 30, soundtrack length), as in the viewer; the original's end is not recovered. evidence: [cutscenes.md §2026-10-14T22:49](../../re/cutscenes.md#ts-20261014-224929).
- MOVIE-S11 During a cutscene the HUD is hidden and the player and doors are held (oracle: HUD hidden on the top screen); what else stops is not known.
- MOVIE-S12 A new game from the front end shows `01_GameIntro` in the front end and skips the script's `PlayCutscene` in the level (a plain `--level` start plays it in the level); the original shows it during the load.
- MOVIE-S30 An actor that is a level character (or the player) is drawn as a copy of that character and the level's own figure is hidden while the cutscene shows; the original drives the figure itself. The copy ends at the same place the figure stood. needs: MOVIE-22. evidence: [cutscene-actors.md §2026-10-15T07:54](../../re/cutscene-actors.md#ts-20261015-075438).
- MOVIE-S20 The cutscene fade times (0.5 s before the start with flag a, 0.5 s at the end for every cutscene, 4 vblanks black, 43 vblanks fade in) are measured with `1_1_b`, not read from the code. evidence: [cutscenes.md §2026-10-14T23:52](../../re/cutscenes.md#ts-20261014-235214).
- MOVIE-S21 With the front end new game `kNewGameStart` (the oracle player position at gameplay-start) is applied before the intro cutscene; the script then moves the player to `1-1_Chase_Intro_Shot_Dummy` (891.47, 1109.06), 4.7 m from it, as in the original order; where the original player stands after the intro was not compared.

## 5. Open items

- [ ] MOVIE-1 — Meaning of `.moflex` layout values other than 0 and 6 and of the two other skipped header bytes; where the original's Mobiclip player reads the layout and selects the eye; whether the second eye of the pair is the right eye. evidence: [fmv.md §2026-10-14T13:39](../../re/fmv.md#ts-20261014-133959).
- [ ] MOVIE-3 — 113 of 397 actor animations have a different frame count from their camera (the viewer holds their last frame); the meaning of camera fields 7-11, the fov axis and the base value of the actor scale are unknown. evidence: [cutscenes.md §2026-10-13T02:30](../../re/cutscenes.md#ts-20261013-0230).
- [ ] MOVIE-4 — The extra (root motion) curve use in cutscenes; the 12 cutscene folders without soundtrack and the outliers 6_4_a, 2_11_a, 1_4_c. evidence: [cutscenes.md §2026-10-13T03:20](../../re/cutscenes.md#ts-20261013-0320).

- [ ] MOVIE-12 — Unknown: what ends a cutscene object, skipping with a button, letterboxing, soundtrack volume and sound sync, what the level objects do. Actor visibility is resolved (the actors are the level's figures, see MOVIE-S30). needs: SCRIPT-47. evidence: [cutscenes.md §2026-10-14T22:49](../../re/cutscenes.md#ts-20261014-224929).
- [ ] MOVIE-21 — Read the slot array of the cutscene object (flag, entity pointer, entity byte +0x18 = 5) once the heap read works. evidence: [cutscene-actors.md §2026-10-15T07:54](../../re/cutscene-actors.md#ts-20261015-075438); screenshots: [cutscene-actors.md §2026-10-15T10:50](../../re/cutscene-actors.md#ts-20261015-105031).
- [ ] MOVIE-31 — (User report 2026-10-16) In the cutscene after the van is brought back to the shop (`1-1_Waypoint_05`), `chase-game` shows Chase twice: the figure that MOVIE-S30 should hide (the player, likely still seated in the van) stays visible beside the cutscene copy. Compare with the oracle from state `m11-6-van-to-shop`. needs: MOVIE-22. evidence: [cutscene-actors.md §2026-10-15T07:54](../../re/cutscene-actors.md#ts-20261015-075438)
- [ ] MOVIE-22 — What entity byte +0x18 = 5 means for a driven figure and where the player and the level characters stand after the cutscene (the port restores their old place). blocks: MOVIE-S30, MOVIE-31. evidence: [cutscene-actors.md §2026-10-15T07:54](../../re/cutscene-actors.md#ts-20261015-075438).
- [ ] MOVIE-24 — During a cutscene the original's bottom screen shows a hexagon panel with a police badge; the port keeps the tip panel or the empty dialogue background; its code and textures are not found. blocks: HUD-33. evidence: [cutscene-actors.md §2026-10-15T10:50](../../re/cutscene-actors.md#ts-20261015-105031).
- [ ] MOVIE-20 — Fades from the code: the cutscene mode that 0x25d26c starts (record table 0x58613c) and its end fade, what flags b, c, d control, whether `1_1_a` fades at its end, whether the fade in includes the camera blend; the end function 0x2fac7c only fades for `7_9_A`. needs: SCRIPT-150. evidence: [cutscenes.md §2026-10-14T23:52](../../re/cutscenes.md#ts-20261014-235214).

## 6. Findings summary

- Cutscene actors are the level's own characters (`CSCharacter`, `Player1.Player01`): [cutscene-actors.md §2026-10-15T07:54](../../re/cutscene-actors.md#ts-20261015-075438).
- FMV container, audio, subtitles: [fmv.md §2026-10-13T11:40](../../re/fmv.md#ts-20261013-1140); stereoscopic pairs: [fmv.md §2026-10-13T20:58](../../re/fmv.md#ts-20261013-205838).
- Cutscene natives, slot table, oracle run of `1_1_b`: [cutscenes.md §2026-10-14T22:49](../../re/cutscenes.md#ts-20261014-224929); fades and level start: [cutscenes.md §2026-10-14T23:52](../../re/cutscenes.md#ts-20261014-235214).
- Cutscene tracks, cameras, actors, joint orientations: [cutscenes.md §2026-10-13T02:30](../../re/cutscenes.md#ts-20261013-0230); playback rate and soundtrack: [cutscenes.md §2026-10-13T03:20](../../re/cutscenes.md#ts-20261013-0320).

## 7. Code and tests

- `src/movie/` (`chase_movie`), `src/fmv.cpp` (`chase-fmv`), `src/assets/subtitles.cpp`, `src/assets/cutscene.cpp`; `chase-viewer --cutscene`; `src/runtime/natives_cutscene.*`, `src/runtime/cutscene_cast.*`, `src/game_cutscene.*`, `src/game_level_cutscene.cpp` (game side), `loadCutscene` in `chase_assets`.
- CTest: `fmv-real-smoke`, `viewer-cutscene-smoke`, `frontend-real-data`, `natives-cutscene`, `natives-cutscene-real-data`, `cutscene-cast`.
