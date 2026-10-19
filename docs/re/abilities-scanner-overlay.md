# Sound scanner: overlay

The top-screen overlay of the audio scanner view (layouts of `sprites/blend/DRC_Scanner`, reticle, markers, info boxes, frame tint, sonar segments). Part of [abilities-scanner.md](abilities-scanner.md).

## Current state

- Overlay: layouts of `sprites/blend/DRC_Scanner` at half scale centred on the screen, zoom meter time = zoom progress x 3.0 s, reticle storyboards `Ret_Focus` / `Ret_LockingOn` by lock state, scan meter by scan progress; the reticle position follows 0x2ef4c8. [§20261019-085330](#ts-20261019-085330)
- Markers: every scannable on screen gets a `SoundCentre` pip and one of 15 `SoundPulse` ring triplets (the large green rings); the map screen shows the audio and exit buttons; the exit by B ends 57 updates after the press. [§2026-10-19T10:01:16](#ts-20261019-100116)
- Info boxes: state machine 0x2d2e14, top / bottom choice 0x1f3494 (reticle y against 105 with a hysteresis of 20), icon table = sheet 0 of `tables/tablespoi.bxls` by the class value `InfoIcon`; ported. [§2026-10-20T00:18:54+02:00](#ts-20261020-001854)
- Overlay leftovers: `TargetAquired_On`, `ScanButtonPrompt_*` and `WAV*` are never started; the info boxes follow the state machine 0x2d2e14 on scan state 6; the frame tint depends on ability bit 28; the sonar formula of 0x2e9030 is transcribed. [§2026-10-19T14:13:26](#ts-20261019-141326)

<a id="ts-20261019-085330"></a>

## 2026-10-19T08:53:30+02:00 — The audio scanner overlay: layouts and storyboard rules (REA 0x16785c region, 0x2e9030, 0x1d99a8, 0x263400, 0x2ef4c8, 0x1ebd14, 0x245b40; oracle B-1 screenshots)

Corrected by [abilities-scanner.md §2026-10-19T10:01:16](#ts-20261019-100116): the large green ring at lock state 2 is the SoundPulse markers, not a reticle storyboard state.
Corrected by [abilities-scanner.md §2026-10-19T14:13:26](#ts-20261019-141326): 0x1d99a8 tints the Zoom_Meter LayoutRoot, not iconAudioMode, and the tint depends on ability bit 28 of the player character.

### Method

REA (Ghidra 12.1.2) pseudocode of the UI setup (the function around 0x1be9e4 / 0x1befcc, name FUN_0016785c), 0x2e9c1c, 0x2e9eec, 0x2e9030, 0x2ef4c8, 0x1ebd14, 0x245b40, 0x227228, 0x233948, 0x2119d4, 0x263400, 0x20f540, 0x234c04 and the assembly of 0x1d99a8; the layouts with the C++ decoder (`parseLayout`); the screenshots of [§2026-10-19T08:43:45](abilities-scanner.md#ts-20261019-084345). The vtable-like table at 0x34cf94 and the audio / physical / photo reticle objects (0x52451c, 0x5244e8, 0x526834) were read only as far as their setup.

### Layouts

- Setup loads `Sprites/blend/DRC_Scanner/DRC_scanner` (1024 x 512 units; children `Common` = border, mirrored border, `Zoom_Meter`, `Signal_Meter`, `ModeButtons` (opacity 0), `iconAudioMode`; `ModeLayers` with `Physical` and `Audio` (`Reticule_Audio` at 262, 128, 500 x 256, and the six `SonarSegment`s); `info_top`, `info_bot`), the layouts `ScanMeter` (three instances, storyboards `Scanning`, `Off`), `SoundCentre` (50 instances, storyboard `Pulsing`) and `SoundPulse` (45 instances, `Pulse`) and the photo layouts. Every scene's `LayoutRoot` gets the scale 0.5 / 0.5 from 0x20f540 (`0x20f540(0.5, 0.5, scene, 1)`) and 0x234c04 centres it on the 400 x 240 screen. The screenshot confirms it: the zoom meter icons are 93 pixels apart for 187 layout units, and the headphone icon and frame corners land where the half-scale layout (512 x 256, offset -56, -8) puts them.
- Zoom meter (0x1d99a8): the storyboard `Zoom` of `zoom_meter.bxaml` (3.0 s) is set to time = progress x length (0x23caa4) with progress = 1 - (zoom - min) / (max - min) (0x263400; settings +0x18 = 20, +0x1c = 43), so zoom 43 is time 0 (arrow at the minus end) and zoom 20 time 3.0. The same function tints `border`, `border_Copy` and `iconAudioMode` with white, or with a stored colour (+0x18 of 0x1d9ae4) when a flag of the player's component passes 0x23a87c (not read); the three are white in the pictures.
- Reticle (0x2e9030, parameters dt and the reticle UI; its +4 pointer is the mode record +0x14, so the byte +0x28 is the lock state +0x3c, +0x14 the lock progress, +0x1c the scan progress, +4 / +8 the reticle position): the root of `Reticule_Audio` sits at the layout position plus (reticle position - screen centre); the mode position +0x18 / +0x1c comes from 0x2ef4c8 (state 1: linear blend from +0x20 to the target's screen position by the lock progress, 0x2119d4; state 2: on the target; states 5 to 8: on the scanned object or the screen centre; states 0, 3, 4 write nothing).
- Storyboards by state (0x1ebd14 starts the new storyboard and resets the previous one to time 0): states 0 to 4: `Ret_LockingOn` restarted every update (time 0), `Ret_Focus` set to progress x 0.26 with the lock progress, the scan meter restarted; states 5 and 7: `Ret_LockingOn` at scan progress x 2.26 and the `Scanning` storyboard of ScanMeter (3.0 s) at scan progress x length; state 6: `Ret_LockingOn` at its length, ScanMeter storyboard `Off` once when the state changed; state 8: `Ret_Focus` at scan progress x 0.26. Storyboard lengths from the data: Zoom 3.0, Ret_Focus 0.26, Ret_LockingOn 2.26, Scanning 3.0.
- The pieces of `ScanMeter` are 28 images with static rotations (`rot`), `ScaleRoot` scale 0.28: 350 x 0.28 x 0.5 = 49 pixels across, the size of the white arc around the centre of the mid-scan screenshot.
- Zoom timing (0x26f0b4) and the camera yaw are in [§2026-10-18T11:27:21](abilities-scanner.md#ts-20261018-112721).

### Port

`src/runtime/scanner_reticle.{hpp,cpp}` (state to storyboard fractions and reticle position, test `scanner-reticle`), `src/frontend/scanner_overlay.{hpp,cpp}` (draws `drc_scanner.bxaml`, `reticule_audio.bxaml` and `scanmeter.bxaml` with the half-scale layout; `LayoutScene::rotations` makes the layout drawer apply element rotation and track property 5 and lets scale tracks replace the element scale; test `scanner-overlay-data`), glue `LevelSession::drawScannerOverlay` in `src/game_scanner.cpp`. While the view is shown the overlay replaces the hearts HUD on the top screen (the oracle shows no hearts in the view, B-1 screenshot `04_full_zoom`). In state 6 the port draws the scan meter at the end of `Scanning` instead of the `Off` storyboard (SCAN-S6). A chase-game run from the roof trigger shows the frame, zoom meter and headphone icon like the oracle picture.

Unknowns:
- The cyan wash over the 3D view and the green dog (message 0xfb) are not in these layouts (not found). The lock state 2 screenshot shows a large green ring (`ReticuleFocus`) that the rule above (`Ret_LockingOn` at time 0, where `ReticuleFocus` has opacity 0) does not produce; what 0x245b40 / 0x233948 do to a restarted storyboard is not settled.
- What starts `Reticule_Spinner`, `ScanButtonPrompt_*`, `WAV*` and `TargetAquired_On`, the sonar segment intensities (0x2e9030 computes them from the direction of every sound source) and the `SoundCentre` / `SoundPulse` markers; the bottom-screen scanner UI (map with the headphone and return buttons, hint box); the scan meter position beyond "centred on the reticle".

<a id="ts-20261019-100116"></a>

## 2026-10-19T10:01:16+02:00 — Scanner sound markers, map-screen buttons, exit by B, where the wash is not (REA, oracle B-1 and B-2 of round 28)

Corrected by [abilities-scanner.md §2026-10-19T14:06:22](abilities-scanner.md#ts-20261019-140622): the wash is the material override 0x530834 of the audio mode, and there is no green dog (the green shape is the 2D spinner).

### Method

REA (Ghidra 12.1.2) pseudocode of 0x2e9030 (re-read for its per-object part), 0x1948dc, 0x262d94 (the UI part of the scanner entry, named FUN_001bf2d0 by the decompiler), 0x2d9f84, 0x2eec90, 0x2b9cf8, 0x1ee2d0 and llvm-objdump of the callers of the scanner getters 0x246128 (active, +0x34 of 0x375ee4), 0x23601c (zone object), 0x1948cc (mode index), 0x2b9d8c (+1 of 0x375eb4), 0x2ce434, 0x2c529c, 0x245b40, 0x231cec; the layouts `soundcentre.bxaml`, `soundpulse.bxaml` and `mapbuttons.bxaml` with the layout decoder. Oracle (emulator servant, state m12-8-roof-sound-scanner): B-1 exit by B (`artifacts/oracle/r28B-out/b1/`), B-2 twelve screenshots of the locked view with ticks of 0x376b0c (`artifacts/oracle/r28B-out/b2/`, not committed). Note: 0x5165c8 is not a clock (constant); the game clock is 0x376b0c, 100 ticks per update.

### Per-object markers (the large green rings)

- The large green ring at lock state 2 is not part of the reticle storyboards. 0x2e9030 gives every record of the mode's scannable list (count at +0xc80 of the list block, 0x28 bytes per record, sorted with 0x26eec4) whose +0x24 bit 0 is set a `SoundCentre` instance (50 instances, storyboard `Pulsing`, 0.459 s, restarted when it has ended) and, for the first 15 such records, a triplet of `SoundPulse` instances (45 instances, storyboard `Pulse`, 1.1 s: ring image `AudioSurface_WhiteGrad1`, a green ring, opacity 0 -> 0.5 at 0.3 s -> 0, scale 0 -> 1.5, size 256 at layout scale 0.5).
- Per record: when it first shows, the fade timer is 0; it grows by 2 per second to 1. The layout opacity is timer x distance fade, with the distance of the record's screen position from the screen centre (200, 120): 1 up to 20 pixels, then 1 - (d - 20) / 180 clamped (0 from 200). Centre pip and rings are placed at the record's screen position minus half their size.
- Pulses: the first update starts the three ring storyboards with speed 1 + 0.2 k and a delay argument 3 k (k = 0, 1, 2) through 0x245b40(speed, 0, board, 0, delay, 0xffff, 0, 0); when all three have ended (state 6) a 0.3 s wait runs, then they start again.
- The B-2 pictures (12 updates of a locked dog) show three concentric green rings that grow and fade; the series is too short to give the delay unit. Port: `src/runtime/scanner_markers.{hpp,cpp}`, drawn by `src/frontend/scanner_overlay.cpp` before the frame, test `scanner-markers`. The reticle storyboard mapping of [Corrects §2026-10-19T08:53:30 (unknown about the ring)](#ts-20261019-085330) is otherwise confirmed: 0x2e9c1c stores `Ret_Focus` at +0x20 and `Ret_LockingOn` at +0x24.

### Bottom screen in the scanner view

- The oracle picture shows the map with the headphone button (greyed) at (0, 116) and a back-arrow button at (0, 176): the `buttonAudio` and `buttonExit` references of `mapbuttons.bxaml` (64 x 64, icons `buttonicon_audio` and `buttonicon_exit`); the characters button is gone. 0x262d94 (scanner entry) shows and hides the button layouts at 0x519e24 + n x 0x28 by mode (mode 2 for a type 201 zone) and plays their slide-in storyboards (`AudioOnDelay`, `ExitOnDelay`). Port: `MinimapView::scanner` draws both buttons at their final place, the audio button in the `disabled` look.
- After the exit the bottom screen showed the tutorial box "Press L or R to zoom in and out." (B-1 picture `b1_after.png`).

### Exit by B (oracle B-1, retry)

Byte pattern (0x375eb6, +0x34, +0x35, +0x36): idle (1,0,0,0) -> after A (0,0,0,0) -> 37 updates later (0,1,1,0) -> 20 later (0,1,0,1) -> 36 later view shown (1,1,0,1); 93 updates from the press to the view. B pressed in the view: one update later (0,1,1,1); 56 updates later all four return to idle in the same sample. The exit by B therefore ends 57 updates after the press, like the script exit (57); the exit callback inside is not visible in the sampled bytes (SCAN-S1 stays). The first try pressed B while the view was still entering and B was ignored.

### Where the cyan wash is not

- Not in the DRC layouts and not in any caller of the scanner-active getter 0x246128 that was checked (155fac, 16642c, 17cb74 (HUD hide), 1af3d0, 1b0748, 1ee848, 20ac88, 235358, 26be04, 2ace20, 2d3738, 2d9d0c, 2d9f84 (map screen), 2e5158, 303990, 309a5c): they handle gameplay state, the map screen and camera modes. 0x2b9cf8 (called by 0x279834 every frame) writes an entity colour record (bytes at +0x3c, 0x205b5c) for every entity of the list at 0x375d70 with 1 / 1 / 1 / 0 when 0x2297b0 or 0x236104 hold; this is a highlight flag, not cyan.
- The game embeds PICA shader programs (data 0x378225: `Lit`, `PostProcess`, `UAVOverlay`, `Outline`, `PlasmaShell`, `Shockwave`, `ScreenRipple`, `RingEffect`, `ShadowMap`, `Cubemap`, `TexCoords`) and a per-vertex `VColourTint` uniform; which draw path the scanner changes was not found. The picture shows a luminance-like blue-cyan mapping of the whole 3D view including the sky (sky pixel 80, 255, 255; dark pixels 16, 91, 139 in 04_full_zoom): red about 0.2 to 0.3 of green, blue about 1.4 of green, clamped.
- 0x1948dc is the scannable list builder of SCAN-S3: five lists (kinds 0xcd, 0xd3, 0xd4, 0xd5, 0xdf, 0x148 bytes apart at 0x526aec); kind 0xd3 takes objects whose class passes 0x2ce434 (type 205: `ShowOnObjectScan`, type 211: `ShowOnAudioScan`), have `ScannerPickup` and not `ScannerTempDisable` and sit in the list of the player's area (0x21cf7c); kind 0xcd (physical) takes entities of type 0x1b with bound flag 0x80 and, through 0x2ce434(0xcd), other objects with `ShowOnObjectScan`; so `ShowOnObjectScan` belongs to the physical list and `ShowOnAudioScan` to the audio list.

Unknowns:
- Where the wash and the green dog are produced (the draw path or shader state the scanner view changes).
- The unit of the pulse delay argument, the sort order 0x26eec4 (it only decides which records get one of the 15 ring triplets), the sonar segment intensities that 0x2e9030 also computes, and the start of the first `Pulsing` time of a centre pip.
- The slide-in timing of the map buttons, the map circle and arrow icon of the scanner screen, the touch handling of the back button and what 0x24581c / 0x2134c8 / 0x217d0c do exactly.

<a id="ts-20261019-141326"></a>

## 2026-10-19T14:13:26+02:00 — Scanner overlay leftovers: unused storyboards, info boxes, frame tint, sonar segments (REA)

Corrected by [abilities-scanner.md §2026-10-20T00:18:54](#ts-20261020-001854): the info box target of phase 1 is the aimed record, the icon table is sheet 0 of tablespoi.bxls, and the box choice 0x1f3494.

### Method

REA (Ghidra 12.1.2) pseudocode of the UI setup FUN_0016785c (0x1be9e4), 0x1bf148, 0x2e9b3c, 0x2e9c1c, 0x2e9eec, 0x2e9030, 0x2d2e14, 0x2ad730, 0x23a87c and 0x2dec28 (the function that contains 0x1d99a8); `strings` and literal-pool search of the ELF; the initial data at 0x375e3c / 0x375e64 / 0x375e6c read from the ELF. No oracle.

### Storyboards the code never starts

- `TargetAquired_On` is looked up by 0x2e9c1c and stored at reticle +0x28, and only freed again by 0x2e9b3c. 0x2e9030 never uses +0x28, so it is never played. `ScanButtonPrompt_*` and `WAV*` are not named anywhere in the code (no string). The port does not play any of them, which matches.

### Info boxes (0x2d2e14, called first by 0x2e9030)

- Setup: `info_top` and `info_bot` of DRC_scanner, each with `LayoutRoot`, `*_object_icon`, `*_box`, `*_i` and the storyboards `InfoOn` / `InfoOff`; the green box and "i" images `info_box_green.png` / `info_i_green.png` are loaded (0x375e78 / 0x375e7c). The audio enter 0x1bf148 hides both boxes (0x24581c(box, 0, 0, 1)).
- State byte +0x10 of the box object: 0, then 1 when the scan state (mode +0x3c) becomes 6 (was not 6). State 1: target = the scanned record (mode +0x34), show the boxes, rewind `InfoOff`, start `InfoOn` on both, set the icon through 0x2ad730, go to 2. State 2: when both `InfoOn` have ended go to 3. State 3: when the scan state leaves 6 go to 4. State 4: rewind `InfoOn`, start `InfoOff`, go to 5. State 5: when both have ended, hide the boxes and go back to 0.
- Every update with a target: box opacity 1 while the record's squared screen distance (+0x20) is below 625 (25 pixels), else 1 - (d² - 625) / 2975, clamped at 0 (0 from 60 pixels); both boxes sit at their base position plus the record's screen position (+0x14, +0x18).
- Icon (0x2ad730): the texture of the table 0x375e64 (filled at run time, zero in the ELF) at the index of the object's component byte +0x24, or index 0 when that byte is 5 and the player character lacks ability bit 28.

### Frame tint (0x2dec28, around 0x1d99a8)

- `border`, `border_Copy` and the `Zoom_Meter` `LayoutRoot` (0x52694c; not `iconAudioMode` as [Corrects §2026-10-19T08:53:30 (the third tinted element)](#ts-20261019-085330) said) get colour 0xffffffff, or the colour at 0x375e6c (zero in the ELF, written at run time) when 0x23a87c(player component +0x2cc, 0x1c) passes. 0x23a87c builds a bit mask with bit 28 and asks 0x210e38 whether the character (+0x2cc is the character index) has that ability. Chase's police outfit shows white in the oracle pictures.

### Sonar segments (0x2e9030, for SCAN-10)

- For each visible record: offset = screen position - (200, 120) (0x529e74 / 0x529e78 halved); angle = atan2(x, -y) moved into [0, 2π); segment i = trunc(angle x 0.95493) (6 / 2π); weight 0 below 50 pixels, 1 up to 300, then 1 - (d - 300) / 100 clamped. Segments i and i + 1 each get (1 - |table[k] - angle| x 0.95493) x weight, with the angle table at 0x375e3c = 0, π/3, 2π/3, π, 4π/3, 5π/3 and a 0 after it (index 6 for i = 5). Each sum is clamped at 1 and becomes the opacity of `SonarSegment_k` (0x21d384); its storyboard (name at 0x2e9e7c) restarts when it has ended. 0x2e9eec places the six segment roots at setup.

Unknowns:
- The writers of 0x375e64 (icon textures) and 0x375e6c (tint colour), the name of ability bit 28, and the reader of the zone's `Scanner_PFX` (+0x54); what 0x2c5774()+0xc is (shown while the mode index is 1); the storyboard name at 0x2e9e7c.

<a id="ts-20261020-001854"></a>

## 2026-10-20T00:18:54+02:00 — Scanner info boxes: state machine, top / bottom choice, icon table, oracle picture (llvm-objdump, oracle picture of the lead's servant)

[Corrects §2026-10-19T14:13:26 (the info box state machine and the icon table)](#ts-20261019-141326): the target of state 1 is the record the scanner aims at (mode +0x48), the one used from then on is the scanned record (mode +0x44) and the icon table is a data table.

### Method

llvm-objdump (`--mattr=+vfp2,+vfp3`) of `artifacts/game/game.arm.elf`: 0x2d2e14 (literal pool 0x2d3130 = 0.0, 0x2d3134 = 1.0, 0x2d3138 = 625.0, 0x2d313c = 2975.0), 0x1f3494 (pool 0x1f35a0 = 0.5, 0x1f35a4 = 15.0, 0x1f35ac = 20.0, 0x1f35b0 = 1000.0), 0x2ad730 and 0x29e18c. Layouts with `chase-layoutdump --tracks`. Oracle: one run of the servant (state m12-8-roof-sound-scanner, New 3DS, OpenGL), pictures `artifacts/oracle/r40-servant-out/s12c_066.png` and `s12c_sheet2.png`. No REA.

### State machine 0x2d2e14 (this +4 is the mode record +0x14, so the byte +0x28 is the scan state)

- Every call: with the scan state 6 the box target is the scanned record (+0x30, mode +0x44); with a target, opacity 1 below a squared distance of 625, else 1 - (d2 - 625) / 2975, at least 0; both box roots (0x5268fc, 0x526920) get that opacity and the position (own x + record x, own y + record y).
- Phases (byte +0x10): 0 waits for the scan state 6 with the previous state (+0x29) not 6; 1 takes the aimed record (+0x34, mode +0x48), picks the box (0x1f3494), rewinds `InfoOff`, starts `InfoOn` on both layouts, sets the icon (0x2ad730) and goes to 2; 2 waits for both `InfoOn` to end; 3 re-picks the box every call and waits for the scan state to leave 6; 4 rewinds `InfoOn`, starts `InfoOff` on both; 5 waits for both to end, hides both boxes, clears the target and goes to 0. `InfoOn` is 0.7 s and `InfoOff` 0.2 s (group opacity 0 to 1 and y offset -20 to 0 for the top box, +20 to 0 for the bottom box; the green "i" blinks).
- After the switch the function reads the pointer of 0x2c5774 + 0xc and, with the byte 0x375ee6 equal to 1, runs 0x2046c0 / 0x24d08c on it; not followed.

### Top or bottom (0x1f3494, show flag 1)

- Line = half the screen height (0x529e30 + 0x48, 240) - 15 = 105; with the scan state 6 the hysteresis is 20 (else 1000): when the last choice (+0x11) was 1 the line is 85, else 125. The value tested is the float at this +4 + 8 (mode +0x1c, the reticle y). Above the line (lower on the screen) 0x5268fc is shown and 0x526920 hidden (choice 1), else the reverse (choice 2). The oracle picture agrees: with the reticle at y about 138 the box shown sits above the target (this fixes 0x5268fc as `info_top`; the setup code that stores the objects was not decoded).

### Icon (0x2ad730) and the icon table writer 0x29e18c

- The icon is the texture at index (byte +0x24 of the record's object component) of the table at 0x375e64; index 0 when that byte is 5 and the player character lacks ability bit 28. The writer 0x29e18c loads the data table `TablesPOI.xls` (file `tables/tablespoi.bxls`), and loads one texture per row of sheet 0 (25 rows, path in column 1: `sprites/blend/drc_scanner/images/icons/ui_scanner_*.png`). Row 13 is `ui_scanner_dog`; the class value `InfoIcon` of `1-2_Dog_Prop` is 13 (the link of the byte +0x24 to `InfoIcon` is inferred from that match and the oracle picture, not read). Sheet 1 of the table is the 28 postcard pictures.

### Oracle picture

- From about vblank 96 after the A press the box fades in, fully shown (opacity of its group) by about vblank 112, and stays until the scanner exit starts (vblank 194): a hexagon with the dog icon centred above the reticle and a small green "i" to its right. Box centre (152, 78) against the reticle (150, 138); the port's formula gives the centre at base (201.5, 58) + (screen - (200, 120)) = (151.5, 76). Fade-out not measured.

### Port

`src/runtime/scanner_info.{hpp,cpp}` (state machine and choice), `drawInfo` in `src/frontend/scanner_overlay.cpp` (green images `info_box_green` / `info_i_green`, the icon from `Resources::tableText`), glue in `LevelSession::drawScannerOverlay`; tests `scanner-info`, `scanner-overlay-data`.

Unknowns:
- The units of the record's +0x14 / +0x18 (the port uses pixels from the screen centre, measured), the writer of byte +0x24 and its link to `InfoIcon`, the name of ability bit 28, the tail 0x2d3020 (0x2c5774 object), whether 0x5268fc is `info_top` by code, and the fade-out in the oracle.
