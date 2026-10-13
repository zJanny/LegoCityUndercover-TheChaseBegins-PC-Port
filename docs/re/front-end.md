# Front end: boot to the first level

Findings from the oracle (Azahar, fresh save, Old 3DS, software renderer, GDB stub screenshots) and objdump/REA reading of `game.arm.elf`. Related: [ui-layouts.md](ui-layouts.md), [fmv.md](fmv.md), [text-localisation-fonts.md](text-localisation-fonts.md), [save-format.md](save-format.md), [arm-executable.md](arm-executable.md) (0x1e7bac "any button" test).

## 2026-10-13T17:54:08+02:00 — Screen flow and the boot sequencer (partial)

**Mechanism.** Mode object 0x51429c (vtable 0x363738, update 0x302080) is the boot sequencer. Its table at 0x372eb4 has 20-byte rows `{mode object, fade in, fade out, hold, int}` (floats set by the initialiser 0x348cd0). `[0x372e6c+0x14]` = current row, `+0x18` = next index. Each step calls the transition 0x25d26c(mode, 6, ...). Mode objects and their vtables are built by 0x348cd0. Vtable slots: +8 enter, +0x1c update, +0x20 leave.

| Row | Mode object (vtable) | enter / update | Screen | int |
|---|---|---|---|---|
| 0 | 0x5142c8 (0x364398) | 0x312edc / 0x313018 | black; asynchronous load (0x2400a0). Sets the flag `[0x372e6c]` = options last-slot byte < 0 ("first run") | 0 |
| 1 | 0x514348 (0x363768) | 0x302144 / 0x302450 | logos: top `sprites/splashscreens/splash2top`, bottom `splash2bottom_uk` (bottom tries `_uk` first, then no suffix) | 2 |
| 2 | 0x51431c (0x3640f0) | 0x30d838 / 0x30d9f4 | movie `CutScenes/splash%u.moflex` = splash5 (TT Fusion), top screen 400x240 | 5 |
| 3 | 0x514378 (0x363cd0) | 0x30740c / 0x3076d8 | language list (skipped when the flag is 0) | 5 |
| 4 | 0x5143cc (0x3643c8) | 0x31315c / 0x313368 | prompt state machine, byte `[0x372e6c+7]`: observed 1 = StreetPass question, 7 = HOME notice | 6 |
| 5 | 0x514418 (0x364740) | 0x319ad8 / 0x319cb8 | autosave notice | 5 |
| end | 0x514f8c (0x361ba4) | via 0x16c0d0(1, 2) | slot list | |

Rows 1..5 have fade in 0.5, fade out 0.5, hold 4.0 (row 1) or 1.0. Row 3 is skipped in 0x302080 when the first-run byte is 0.

**Triggers** (code, with oracle checks):
- Logos: done when screen time > hold + fade in (4.5 s); 0x1e7bac returns 0 for int 2, so no button skips it. Oracle: 66 updates at dt 0.07 (4.6 s), then a hard cut to black (no visible fade out), 3 black updates, movie.
- Movie: ends at the end of the video, or on a press of A/B/X/Y/Start when the first-run byte is 0 (0x1e7bac). Oracle: 181 updates for a 6.36 s clip, i.e. frame-locked to updates, not to dt. Then 3 black updates before the next screen.
- Language: Up/Down move the row (sound 18), A (callback 0x2bc8e4 sets `[0x372e6c+1]`) stores the language (rows -> 1,2,3,4,5,7,6,9,8) in options +1, applies it (0x1f440c), plays sound 20 and fades out.
- Prompts, autosave: A or B as shown on the screen. StreetPass question: A = Yes, B = No (only No was tried).
- Slot list: A on a slot; "New Game" then shows "Saving Game..." (systemtext 0xcbafe6a8), a loading screen (`UI_LS_Generic1`, subtitle of the video), then `01_gameintro.moflex` with its subtitles on the bottom screen in the dialog bubble over `ui_lcu_bg`, then the "Doughnut Disaster" mission card, then gameplay.

**Timing in the oracle.** Update count in the boot: black 34 updates (load-bound), logos 66, movie 181, language reached at update 286. Free-run wall time and vblanks mislead (host slower than 3DS: clock 66 ticks/vblank, dt capped at 0.07); use stepped `memtrace` (0x372e6c:0x30 shows row and index per frame) and the game clock 1/3000 s.

**Language screen drawing (0x3073e4..0x3076d8).** Frontendfont, letter spacing -2 (set by 0x235cb0(-2)), row i centred at x = 160, line middle y = 20 + 24 i, text keys at 0x3660fc (gametext). Unselected colour (0.174, 0.407, 0.6), selected (0.29 k, 0.678 k, k) with k = (sin(0.2 n) + 2) / 3 (n unverified: update counter assumed). Matches the oracle bottom screen to 5 pixels (max 3).

**Layout pages** (savegameflow): `page_alerttext.bxaml` with storyboard TransitionOn2 at its end, background `ui_lcu_bg` plus the 60 % black `quest_info_panel_a13` overlay, text in Text_Info_NoHeader (frontendfont, spacing 0, line pitch 27 for two lines, 23 for three, leading above the line, wrap limit 296 px), buttons from `ui_frontend/images/screen_selectbutton|backbutton.bpng`. Keys: StreetPass gametext 0x4617a2ca, HOME gametext 0x65213108, autosave gametext 0xa5e2f63f, Yes 0x2c187527, No 0x4e1381, OK 0x4f13bc, Continue systemtext 0xe3f70996, Select gametext 0x715c59fc, Select Slot systemtext 0xe2117397, New Game systemtext 0xd7476f00. `page_selectslot.bxaml` (TransitionOn end, ListItem1 with storyboard ButtonHighlight, hidden Text_Time/Percent/Location, header spacing -1) over `ui_lcu_bg`, plus `ui_frontend/shortcuts.bxaml` drawn at (-80, -16) (480x272 canvas on the 320x240 screen) showing only SelectButton_3DS. The prototype renders agree with the oracle (mean abs diff 0.6 for the StreetPass page, 0.8 for the slot list; the HOME and autosave pages differ only by the wrap width, fixed afterwards). Autosave notice spinner: `ui_saveindicator` about 32 px at (159, 211), rotating (page_saveicon storyboard Activity); not drawn yet.

**Slot list top screen.** `ui_3dstopscreenlogobg_01..05_uk` stacked in that order, each 512x256 centred at (-56, -8) on the 400x240 screen, matches the oracle (mean abs 0.26). The other texture `ui_3dstopscreenlogo_uk` and `logobg_uk` are not part of it. Whether the layers move (stereo parallax or drift) is not known.

**Tool note.** `tools/azahar_oracle.py` could not start Xvfb when the stale `/tmp/.X*-lock` files filled :90..:199; fixed on main (286b81d).

**Unknowns**
- StreetPass Yes path and sub-states 2..6 of row 4; the flow for a returning player (no language screen, flag 0): which prompts show.
- Row 0 duration on real hardware (34 updates measured); fade-in shape and length (about 3 updates visible on the language list); the transition code 0x25d26c / 0x22b6d4.
- Language index to localisation suffix and logo bottom variants for non-UK languages.
- Whether Up/Down on the circle pad also move the language row; slot list behaviour (move, cancel, existing saves, delete).
- Pulse counter of the selected language row; spacing/offset rule per element (font size field 12/14/10 vs 0).
- The loading screen, intro movie subtitles layout and mission card, and the code that drives them.

## 2026-10-13T20:46:07+02:00 — Prompt state machine, save indicator, boot pages, intro screens; native front end (chase_frontend)

Sources: REA (Ghidra 12.1.2, `procedure_pseudo_code`) on `artifacts/game/game.arm.elf`; Azahar oracle (OpenGL + RPC screenshots, port 24690, home `artifacts/oracle/fe-home`, states front-lang / front-streetpass / front-selectslot, cold boot capture); native renders compared with the oracle screenshots (mean abs difference over a 400x480 composite).

**Corrections of earlier entries**
- Row 0 (0x5142c8, enter 0x312edc, update 0x313018) is not a black screen. A cold-boot capture shows the bottom screen with the `page_alerttext` layout on black (the TransitionOn storyboard, which hides the `image` element), the save indicator spinning, header "Preparing Game Card..." (systemtext 0x12e3d762), then header "Preparing save data..." (0x84828812 or 0xc1117bc4, same text) with body "Do not turn off the Nintendo 3DS system." (0x2c824a40). The top screen is black. 0x312edc starts the asynchronous load `0x2400a0(0x17ed04, 0x2c195c, row+0x50)`; 0x313018 leaves the row when the byte at row+0x50 is set (transition 0x22b6d4 with 0.5) and then reads the first-run flag (REA ev_b95d58f3…, ev_ef81d49a…). The earlier "black 34 updates" came from a state that was saved after this page.
- "The movie is frame-locked to updates" (earlier entry) holds for splash5 only because it runs at 30 frames per second. `01_gameintro.moflex` has 4088 video frames at 59.94 frames per second (ffprobe, 68.2 s), so a movie must run against a clock, not one frame per update. The native player does that (`SteppedPlayer::advance(dt)`).
- Logos: 66 updates at dt 0.07 is reproduced when the test `screen time > hold + fade in` (4.5 s, 0x1e7bac) is made before the time advances (65 updates after the first). The unit test pins this.

**Prompt mode 0x5143cc (vtable 0x3643c8; enter 0x31315c; update 0x313368; REA ev_2e8cc455…, ev_6ac06587…)**
- State byte S = 0x372e73. Flags: 0x372e6e (+2), 0x372e6f (+3 "skip the question"), 0x372e70 (+4 = result of 0x329338), 0x372e71 (+5 = result of 0x3293b8), 0x372e72 (+6).
- Enter: +2 = 0, S = 0, +3 = 1, +4 = 1, +5 = 0. If 0x329338() is false: +4 = 0, +3 = 0. Else if 0x3293b8() is true: +5 = 1, +3 = 0. Else r = 0x20aa64(...); +3 = 0 when r < 0 and r == -0x377ef411. If +3 == 0: S = 1 (question) else 0x1e59f8() and S = 6. The page of S is built by the switch that ends both functions.
- 0x329338 reads a 4-byte value (service call at 0x12b050, 0xd0000) and compares it with a threshold at 0x36f02c (a StreetPass slot count, ev_b897e432…); 0x3293b8 reads 0xc0 bytes (0xc0000) and returns bit 5 of the first word (ev_f3b72e12… shows the enclosing function; parental restriction by its text at S=4).
- Pages (text object 0x375964 = gametext, 0x375968 = systemtext): 1 question 0x4617a2ca with Yes 0x2c187527 (button flag 0) and No 0x4e1381 (flag 1); 2 gametext 0x62ab3f48 + Continue (systemtext 0xe3f70996); 3 gametext 0x62ab3f49 + Continue + Back (systemtext 0x8be4916); 4 gametext 0x64301deb + Continue; 5 gametext 0x71ba2807 + Continue; 6 gametext 0xe10321ee + Continue; 7 gametext 0x65213108 + Continue.
- Update (runs only while 0x240030() is 0, i.e. no page is waiting): S=1: result byte (0x20d848 = byte at [0x376d14 + 0x2d], the flag of the chosen button, ev_37c2c3ca…) != 0 (No) -> +3 = 0, S = 7. Else (Yes): +4 set: +5 clear: 0x1e59f8(), then +6 clear: +3 = 1, S = 6, else S = 5; +5 set: S = 4. +4 clear: S = 2. S=2 -> 3. S=3: Back (result != 0) -> 2, Continue -> 7. S=4, 5 -> 7. S=6: +2 = 1, S = 7. S=7: transition 0x22b6d4(next mode, 0, 1).
- Oracle (front-streetpass, A then A): "Even when the game is shut down, StreetPass communication will continue automatically." (S=6), then the HOME notice; B on the HOME notice does nothing. The oracle therefore has +4 set, +5 clear, +6 clear, and +3 = 0 at the start. Native: `StreetPassEnvironment` (stand-in, those values).
- 0x20d85c(text, flag) appends a button (8-byte entries at 0x376d14 + 0xc, count at +0x2c, ev_120c3d85…); 0x20aa94(1, text) sets the body text.

**Autosave notice (row 5, 0x514418, enter 0x319ad8, update 0x319cb8; ev_2974bcbe…, ev_01bfd1f1…)**
- Page: body gametext 0xa5e2f63f, OK (gametext 0x4f13bc). 0x24004c(0x37684c, 1) turns the save indicator on. The indicator object (byte state at 0x37684c; update 0x1a3d0c ev_c8b2c075…, switch 0x24004c ev_815bb013…): 0 off, 1 turning on (callback at +4 with (obj, 1, 0)), 2 on, 3 turning off. The update waits for the page (0x240030 == 0), turns the indicator off when the state is 2 and leaves the row once the state is 0. So the spinner turns off after OK, then the slot list follows. The off transition length is not read.
- The spinner is `sprites/ui_saveindicator/ui_saveindicator.btga` (64x64) drawn at 1:1 with its top left at (128, 180) (the oracle bounding box matches), rotation from the Activity storyboard of `page_saveicon` (-pi to pi in 2 s, opacity 1, 0.5, 0.955, 0.498, 1), TransitionOn opacity 0 to 1 in 0.6 s. The layout's own `Images\saving.png` has no texture in the archive.

**Pages drawn from page_alerttext / page_selectslot (native, `src/frontend/pages.cpp`)**
- Alert pages reproduce the oracle at mean abs 0.2 to 0.4 (HOME 0.20, StreetPass 0.38, autosave 0.38, language 0.22). Body text: frontendfont, spacing 0, wrap 296 px, line pitch 27 for one or two lines and 23 for three (fit), header (`Text_Header`) spacing 0. TransitionOn2 shows the blue panel at once; TransitionOn (boot and saving pages) hides `image`.
- Button icons are `sprites/buttons/dsbuttona.btga` and `dsbuttonb.btga` (32x32): the code strings include `Sprites/Buttons/DSButtonA.tga`; the layout's own 16x16 `buttona.bpng` does not match the oracle.
- Slot page: `ui_lcu_bg` under `page_selectslot` (TransitionOn), text spacing 0 for the rows and -1 with a 2 px shift for the header, rows left-aligned, `ButtonHighlight` at its end for the selected row and at 0 for the other, "Select (A)" from `ui_frontend/shortcuts.bxaml` at (-80, -16). Mean abs 1.4 over both screens. The top screen title layers (`ui_3dstopscreenlogobg_01..05_uk`) at (-56, -8) differ by about 2.4 (not a translation or scale of a layer: searched; probably the original's own blending of the partial layers).
- Slot list behaviour (oracle, front-selectslot): two slots "New Game" 1 and 2; Down selects 2, Up selects 1, B does nothing. A on slot 1: Select_Slot1 storyboard (0.3 s), the list disappears for a moment, then the "Saving Game..." page (header systemtext 0xcbafe6a8, body 0x2c824a40, the boot-page style on black with the spinner) with the title still on the top screen.

**Loading screen, intro movie, title card (oracle, `slot/sa5..sa11`, `intro/intro00..intro49`)**
- Loading: top screen `sprites/ui_loadingscreens/ui_ls_generic1.btga` (512x256, 400x240 shown) with a rotating 3D police badge and seven orbiting stars (code strings `Prop_Special_LoadingRing`, `Badge`, `BadgeFadeInTime`, `loadinglightamb/dir1/dir2`, `loadingcamera` and the model `models/prop_special_loadingring`); bottom: the `ui_dialog` background, a faint hexagon (`awardviewertop/images/bighexbg`), a large badge, and a panel with the tip "Tip: <text>" (gametext 0x9e318a31 plus hinttext 0x926950e8 "If your vehicle gets stuck, hold <EAST> to over rev the engine to destroy it and gain another vehicle."). `<EAST>` is the A button icon (`dsbuttona`). Which tip is chosen is not known (the same tip in all samples).
- Intro movie: top screen the movie (400x240, 59.94 fps); bottom screen `sprites/ui_dialog/ui_dialog_a` + `ui_dialog_b` (320x256 background, matches the oracle exactly, mean abs 0.00) with a dialog box at x 6..313, y 14..85 holding the subtitle (frontendfont, spacing 0, lines centred at y 33.5 and 56.5, pitch 23). The box is `ui_dialog_box` stretched with a 16 px 9-slice (visible margin about 8 px horizontally and 4 px vertically beyond the rectangle; fit). Subtitles come from `cutscenes/01_gameintro_<lang>.srt`.
- After the movie: a black interval, then the mission title card on the bottom screen (title hinttext 0x42ef3429 "Doughnut Disaster" in a box, a hexagon artwork with a police badge in the middle that is not in any single texture found) while the top screen shows the in-engine cutscene (Chase and the police chief, dialog bubble "Welcome to the force, rookie. Hurry up and get those doughnuts!"). Then the player is in Cherry Tree Hills (tutorial hints on the bottom screen).

**Native implementation** (`src/frontend/`, library `chase_frontend`, no SDL; `src/movie/`, library `chase_movie`, FFmpeg)
- `Flow` is the state machine: Boot (stand-in 3 s), Logos (4.5 s, 3 black updates after), FusionMovie (splash5, skippable only when the first-run flag is clear), Language (rows to indexes 1,2,3,4,5,7,6,9,8; sounds 18 and 20), Prompt (the state machine above), Autosave, SlotSelect, SavingGame, Loading, IntroMovie, MissionCard, Done with a hand-off (slot, new game). `FrontEnd` draws every screen into two CPU canvases. chase-game uploads them as textures and blits the two screens stacked (bottom centred), plays movie sound through its own SDL queue and the button sounds through the sound player.
- Verification: `frontend-flow` (state machine), `frontend-real-data` (texts, wrapping, exact logo pixels, render hashes of six screens, movie lengths 189..194 updates for splash5 and 2040..2052 for the intro at 30 updates per second), `front-game-handoff` (a scripted run through every screen into Cherry Tree Hills). Review pictures: `artifacts/review/front-end/` (native, oracle, compare).

**Stand-ins (all labelled in the code)**: durations of the boot pages, saving page, loading screen and title card; the emptied slot page interval (0.5 s); save indicator off time (0.3 s); fade-in of the language screen (0.25 s); StreetPass environment; all slots empty; language to table suffix; the loading tip; flat badge, hexagon and card art; the top screen under the title card is black; page sizes and positions fitted to the oracle (spinner, body pitch, subtitle box, title layers).

**Unknowns**
- What the original does for a returning player (flag 0): whether rows 4 and 5 still show, which slot rows show for existing saves (Text_Time/Text_Percent/Text_Location are hidden in the new-game case), Continue, delete and back behaviour.
- The transition between StreetPass pages (the original builds a new page after 0x240030 reports the old one done; its timing) and the transition out of every page.
- The activation call 0x1e59f8 and the system reads behind 0x329338 / 0x3293b8 / 0x20aa64.
- The async load that row 0 waits for (duration on hardware), and the loading screen's duration and tip choice.
- The 3D badge scene of the loading screen (model, lights, camera, spin and star motion) and the hexagon card art of the title card; the in-engine cutscene before the first level; when the card ends (the oracle route pressed A).
- Whether the title layers of the slot screen move or blend as measured (2.4 mean difference).
- Which level, position and heading a new game starts with beyond the oracle's player position (886.764, 16.589, 1111.166).
