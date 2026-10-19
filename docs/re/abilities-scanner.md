# Sound scanner

The sound scanner of mission 1-2 and later missions: the zone class `props/prop_scannerzoneaudio` (object type 201), the scannable objects (`props/prop_dogwaggles` and others with `ShowOnAudioScan`), the player states of the scanner, the scanner view (aim, zoom, lock-on, scan) and the ObjectTriggered events they fire. Port: `src/runtime/scanner.{hpp,cpp}`, glue `src/game_scanner.cpp`.

## Current state

- Chase holds the scanner device: the clips `scanner_out` and `scanner_in` of `class_generic` carry the hand item track of the magnifying glass, naming `Ability_Scanner` (model `weap_scannerdevice`) and `weaponRight`; the port draws it through the glass's mechanism. [§2026-10-19T18:21:29](#ts-20261019-182129)
- Type 201 descriptor 0x1863c8: create 0x19f6d0, parameter reader 0x192008, message handler 0x1aea24 (message 3 = use), no update. The zone's trigger objects sit at component +0x5c..+0x74; `ScannerMode` at +0x4c. [§2026-10-18T11:27:21](#ts-20261018-112721)
- Enter: A press in `UseBound` -> message 3 -> 0x2249e0(1, zone) -> player event 201 -> state 0x156 "Scanner Enter" (clip `scanner_out`) and the alignment 0x2248c4; when the clip ends state 0x157 "Scanner Active" (0x1bf2d0) starts the spinner transition; `EnterScanTriggerObject` fires 20 updates later (0x2bee98; real-time count, see the last bullet). [§2026-10-18T11:27:21](#ts-20261018-112721)
- View: camera update 0x26f0b4 (settings block 0x1e06ac: zoom 43..20 at 35 per second with L/R, turn 60 degrees per second, pitch -25..40); `ZoomedInTriggerObject` fires in every update at full zoom (oracle: from frame 49 on). Lock 0x2ef4c8 (1.5 per second, `LockedOnTriggerObject`), scan 0x2ef310 (A held, 0.75 per second, `TriggerScanned` of the target once). [§2026-10-18T11:27:21](#ts-20261018-112721)
- Exit: B (slot 7) or `EnableScanner 0` (0x1cd9dc) -> event 201 -> state 0x158 (0x1bf478) -> transition with 0x2befc0 (`ExitScanTriggerObject`, clip `scanner_in`) and 0x1a2824 (`CompleteExitTriggerObject`, flags cleared). [§2026-10-18T11:27:21](#ts-20261018-112721)
- Emulator: in Azahar the camera yaw word 0x376894 reads NaN once the scanner runs, the scannable records never get a screen position, and the game stops responding after the exit (both runs). This is the likely cause of the white view; lock, scan and exit timing could not be measured there. [§2026-10-18T11:27:21](#ts-20261018-112721)
- Real time (oracle with the motion source): state 0x157 49 updates after the press, enter callback +20, end callback and camera +56, lock 20, scan 39, script exit 57; no hang, yaw finite. [§20261019-084345](#ts-20261019-084345)
- Overlay: layouts of `sprites/blend/DRC_Scanner` at half scale centred on the screen, zoom meter time = zoom progress x 3.0 s, reticle storyboards `Ret_Focus` / `Ret_LockingOn` by lock state, scan meter by scan progress; the reticle position follows 0x2ef4c8. [§20261019-085330](#ts-20261019-085330)
- Markers: every scannable on screen gets a `SoundCentre` pip and one of 15 `SoundPulse` ring triplets (the large green rings); the map screen shows the audio and exit buttons; the exit by B ends 57 updates after the press. [§2026-10-19T10:01:16](#ts-20261019-100116)
- Overlay leftovers: `TargetAquired_On`, `ScanButtonPrompt_*` and `WAV*` are never started; the info boxes follow the state machine 0x2d2e14 on scan state 6; the frame tint depends on ability bit 28; the sonar formula of 0x2e9030 is transcribed. [§2026-10-19T14:13:26](#ts-20261019-141326)
- Cyan wash: the audio mode's enter 0x2ef664 sets 0x51641e (render override on), 0x3772cd = 0 (sky included) and 0x3760fd (no fog); the frame function 0x2e4858 then draws the world and sky with the material override 0x530834 = 0x95 and the draw list with 0xd5 (additive draws skipped): mode 3 constant 0xffab8126, no texture, the bump stages give luminance(lighting) x that colour; pass 5 turns on the glow 0x2bf830 (0.6, 1.4). There is no green dog: the green shape is the 2D spinner, and the dog is behind a wall. [§2026-10-19T14:06:22](#ts-20261019-140622)

<a id="ts-20261018-112721"></a>

## 2026-10-18T11:27:21+02:00 — The sound scanner: classes, player states, view, triggers (REA, objdump, Azahar GDB traces of m12-8)

Corrected by [abilities-scanner.md §2026-10-19T08:43:45](#ts-20261019-084345): the enter / exit counts 3 / 4 / 6 are stepped-frame counts; real time is 49 / 20 / 56 / 57 updates, and the exit does not hang with the motion source.

### Method

REA (Ghidra 12.1.2) pseudocode of 0x192008, 0x19f6d0, 0x1aea24, 0x2249e0, 0x1bf2d0, 0x2bee98, 0x2befc0, 0x1a2824, 0x2cb03c, 0x2248c4, 0x187f78, 0x26f0b4, 0x1b7540, 0x26eee0, 0x20f410, 0x1ec470, 0x285c8c, 0x29125c, 0x2ef100, 0x2ef310, 0x2ef4c8, 0x2ef144, 0x2ef664, 0x2ef694, 0x1ebe18, 0x1ebe88, 0x1eb0e0, 0x2d2c78, 0x1cd9dc, 0x322ba8; llvm-objdump (`--mattr=+vfp2`) of 0x1863c8, 0x1e06ac, 0x2ef814, 0x322b7c, 0x325248, 0x1bf478, the state records near 0x34e7dc and the vtable 0x362968. Note: for 0x2d2ab8 and 0x2d2cf8 REA returned the bodies of 0x2ef310 and 0x2ef4c8 (the audio overrides; the vtable 0x362968 +0x34 = 0x2ef100 calls both). Oracle: state `m12-8-roof-sound-scanner`, New 3DS, OpenGL, no RPC, port 24713, runs s1..s5 (`ctrace`, `memtrace`, `read`; stepped frames have dt 0.07, so frame counts of dt-driven steps differ from the 1/30 port).

### Classes and objects of lvl_01

- `1-2_ScannerZoneAudio` (`props/prop_scannerzoneaudio`, ObjectType 201, ScannerMode 1, at 869.48 65.953 645.766, yaw 88.804): `EnterScanTriggerObject` = `1-2_Scanner_Enter_Script`, `ZoomedInTriggerObject` = `1-2_Scanner_Zoomed_Script`, `LockedOnTriggerObject` = `1-2_Scanner_Locked_Script`. Class bounds `UseBound` centre (0, 1, 0.0253) half (1, 1.5, 1), `VisibleBound` half (8, 1.473, 8). `1-2_Scanner_Spot_Waypoint_01` (`props/map_useobjectwaypoint`, ObjectType 203) points at the zone with `UseMarker`.
- `1-2_Dog_Prop` (`props/prop_dogwaggles`, ObjectType 0, mesh `char_dog`): `TriggerScanned` = `1-2_Scanned_Pringles`, `ShowOnObjectScan` 1; the class gives `ShowOnAudioScan` 1, `ScannerPickup` 1, `InfoIcon` 13.
- Four other type 201 zones in lvl_01 are photo zones (`prop_scannerzonephoto`, ScannerMode 4 in the class) and the satellite / physical classes also carry type 201.

### Zone class (type 201)

- Descriptor at 0x1863c8 (`mov r0,#201; bl 0x246240`): create 0x19f6d0 (component 0x8c bytes at +0x7c; `ScannerMode` at +0x4c, `VisibleBound` at +0x50, use zone registered by 0x23e130 at +0x40), parameter reader 0x192008 (+0x54 `Scanner_PFX`, +0x58 `LinkedObject`, +0x5c `PreEnterScanTriggerObject`, +0x60 `EnterScanTriggerObject`, +0x64 `ExitScanTriggerObject`, +0x68 `CompleteTriggerObject`, +0x6c `CompleteExitTriggerObject`, +0x70 `LockedOnTriggerObject`, +0x74 `ZoomedInTriggerObject`, +0x78 `SkyboxOverride`, +0x7c `TeleportTo`, +0x80 `SatellitePower`, +0x86 `ATTR_SFX_USE`, +0x88 `ATTR_SFX_MOVE`), destroy 0x22f0a0, message handler 0x1aea24.
- Message 3 (use): needs the registry flag +0x44 bit 4 (ToggleUseable); without the pressed flag it answers 1 (hint). Pressed, for ScannerMode 0/1/3 it checks for vehicles (type 0xb7) within 3.0 between player and zone (0x22ecf0; a hit sends message 0x155 to the player) and otherwise calls 0x2249e0(1, zone, 0); ScannerMode 2 calls 0x2249e0(2, zone, 0). Message 0x57 starts / stops `ATTR_SFX_MOVE` while the camera moves.

### Enter (oracle s1, s3, s4: A pressed at frame 2)

- 0x2249e0 (globals 0x375eb4..0x375eeb: +2 `eb6` toggle allowed, +0x34 `ee8` active, +0x35 `ee9` in transition, +0x36 `eea` mode index, +0x30 zone): when not active it stores the kind and zone; kind 1 needs player state 1 or 0x73 and sends player event 201 (0x21e1d4). Then 0x2cb03c: alignment flag 0x375e10 = 1, ctrl+0x24c = 0, facing target = zone heading, `PreEnterScanTriggerObject` fired; `eb6` = 0.
- Frame 2: event 201 handler (record near 0x330724) pushes state 0x156; its enter 0x322ba8 plays role 0x208 (`scanner_out`, 18 frames at 15 fps) with blend 0.1. 0x2248c4 each update: facing turns by up to 0x800, position lerps to the zone by ctrl+0x24c, which grows by 1.5 dt (cleared at frame 14 at dt 0.07).
- Frame 20 (18 updates of 0.07 = 1.26 s >= 1.2 s): handler 0x322b7c pushes state 0x157, enter 0x325248 = `b 0x1bf2d0`: `ee8` = `ee9` = 1, sound 0x19, transition 0x20f410(kind, 0x2bee98, 0x1a27e0).
- Frame 23: 0x2bee98 fires `EnterScanTriggerObject` (0x244f40(r0 = 1-2_Scanner_Enter_Script, r1 = zone)), sets the mode index from ScannerMode, `ee9` = 0, calls the mode enter (+8, 0x2ef664 -> 0x1ebe88: view at the player position + 1.7 x 0.85 + `CameraYOffset`, read as 869.48 67.398 645.766 at mode +0xd10). Frame 24: end callback 0x1a27e0, `eb6` = 1. First camera update 0x26f0b4 at frame 26 (0x187f78 gates it on the byte 0x3760c5 == 3).

### View, zoom, lock, scan

- Mode table 0x526ad8 (read): modes 0 and 1 share the object 0x5216ac (vtable 0x362968; +0xd58 = 1 for audio), settings pointer +0x14 = 0x522440 with the values of 0x1e06ac: yaw limit 0, pitch -25 / 40, smoothing 0.1, zoom turn scale 0.66, zoom start 43, min 20, max 43, turn 60 per second, zoom 35 per second, threshold 1e-5. 0x2ef814 writes for audio: +0x34 = 0.75, +0x38 = 4.0, +0xcd0 = +0xcd4 = 80, +0xcd8 = 50, +0xcdc..+0xce4 = 30, +0xcf0 = 1.5 (read the same in memory).
- 0x26f0b4: zoom += 35 (L - R) dt clamped to 20..43 (zoom loop sounds 0x2e then 0x2d); full zoom fires `ZoomedInTriggerObject` (r1 = 0) in every update (oracle s3: R held from frame 40, sounds at 40 and 45, trigger from frame 49 in every frame). Aim: pitch += -stickY 60 dt, yaw += stickX 60 dt (degrees, D-pad override), times 1 - 0.66 (1 - zoom fraction); the yaw accumulator 0x375ee0 grew 0.0257 rad in a 0.033 update with raw stick 120 (s2).
- 0x1ec470 (+0x20): angle limit +0xce8 = 50 - 20 (1 - zoom fraction), range +0xcec = 30 (100 for an object with a shown model, 0x29125c), screen records (0x28 bytes from +0x4c, 14 in m12-8), target search 0x285c8c: smallest squared screen distance with (radius + 80)^2 >= it.
- Lock 0x2ef4c8 (state byte +0x3c): 0 -> 1 on a target, progress +0x28 += 1.5 dt, at 1 state 2, `LockedOnTriggerObject` (zone type 201) and sound 0x2b; losing the target gives 3 / 4 (sound 0x2c) and then 0.
- Scan 0x2ef310: A (slot 6) held on the locked target gives state 5; progress +0x30 += 0.75 dt, above 1 sounds 0xee stop / 0xef, state 6, the target's `TriggerScanned` (component +8) fired once with the target as instigator (flag bit 1). Y (slot 9) or losing the target cancels (0x1eb0e0: state 7 / 8, decay 4.0 per second, 0x2d2c78 back to 0).

### Exit

- B (slot 7 press, 0x187f78) or `EnableScanner` with the other state (0x1cd9dc -> 0x2249e0(1, 0)) while `eb6`: event 201 -> handler near 0x32f724 pushes state 0x158, enter 0x1bf478: `eb6` = 0, `ee9` = 1, transition 0x20f410(1, 0x2befc0, 0x1a2824), sounds 0x27 off and 0x1a on (s3, s4 frame 34; state 1 is pushed in the next update). 0x2befc0: `ExitScanTriggerObject`, player facing from the camera yaw 0x376894, clip 0x207 `scanner_in`. 0x1a2824: `eb6` = 1, `CompleteExitTriggerObject` when component +0x84 bit 1, zone and `ee8` cleared.

### Emulator observations

- After the enter, `player` printed camera yaw `nan` (0x376894) and record 0 (the dog, distance 53.34) kept screen x / y = FLT_MAX and squared distance infinity while flagged visible; so no target was found and lock / scan never happened in Azahar. The source of the NaN was not traced (no RPC read; candidates: the camera matrix of 0x26f0b4 or the yaw written from it).
- In s3 and s4 the stub stopped answering a few frames after the B press (exit callback 0x2befc0 writes the player facing from the NaN yaw); the emulator had to be stopped.

Unknowns:
- The UI spinner timing of the transition (states of 0x26eee0) in real time; the counts 3 / 4 / 6 above are stepped-frame counts.
- Order of 0x187f78 and the use handler in one update; the camera pitch sign of 0x1b7540 (the clamp's base angles were not transcribed); the projection 0x22e1cc and radius 0x25b578; the list builder of the scannable lists 0x526aec; `CompleteTriggerObject`, `SkyboxOverride`, `TeleportTo`, the alien / reward branch 0x2ef144 and the shown-object handling (message 0xfb).
- Exit timing in the original (not measured because of the hang).

<a id="ts-20261019-084345"></a>

## 2026-10-19T08:43:45+02:00 — Scanner timing in real time with the motion source (oracle run B-1, state m12-8-roof-sound-scanner)

### Method

One real-time run by the emulator servant (Azahar, OpenGL, level motion source of [oracle-setup.md §2026-10-18T22:03:32](oracle-setup.md#ts-20261018-220332)), RPC sampling of 0x375eb4 (0x38 bytes), the mode object 0x5216ac +0x20..0x40 and the camera yaw 0x376894 about 900 times per second (103,810 rows); presses by RPC input (A, R, D-pad, A held, B). Raw data: `artifacts/oracle/r27-servant-out/b1/b1_sample.csv` and `b1_events.csv` (not committed). The game clock (0x5165c8, 3000 ticks per second) advances by 100 per update, so ticks / 100 counts updates of 1/30 s (the emulator ran at about 0.94 of real time; ticks are game time).

### Measured

- Enter: the press clears the toggle byte +2 (0x375eb6) in the same update (3 vblanks, 100 ticks); state 0x157 is entered (+0x34 and +0x35 = 1) 4894 ticks = 49 updates later; the mode index (+0x36 = 1, the enter callback 0x2bee98) 1978 ticks = 20 updates after that; the toggle byte back to 1 (end callback 0x1a27e0) and the first valid camera yaw (1.7402 -> 1.5499) 3619 ticks = 36 updates after the callback, so 56 updates after the entry of 0x157. The stepped counts 3 / 4 / 6 of [Corrects §2026-10-18T11:27:21](#ts-20261018-112721) are therefore wrong for real time; `scanner_out` (1.2 s) does not explain the 49 updates (1.63 s).
- Lock: state 0 -> 1 -> 2 (progress 1.0) in 2006 ticks (20 updates, 1.5 per second as read in 0x2ef4c8). Scan: A held at 3626277, state 5 one update later, state 6 (progress 1.0) after 3911 ticks (39 updates, 0.75 per second). Both agree with the port.
- Script exit: after the scan (state 6) the scanned object's script closes the scanner; the exit transition started 47 updates after state 6 (matches `Wait 1.5` of 1-2_Scanned_Pringles); +0x35 stayed 1 and +0x34, +0x35, +0x36, the zone pointer and the byte at +1 cleared and the toggle byte returned to 1 together 5706 ticks = 57 updates after it started (the callbacks inside were not separately visible in the sampled bytes).
- Yaw stayed finite in all rows and the game answered input after the exit: the old hang after the exit does not occur with the motion source.
- The B press (3 s later) found the scanner already closed (the script exit came first), so a B exit was not measured.
- Screenshots (not committed): the view shows a cyan tint over the 3D view, a white thin frame with corner pieces, a vertical zoom meter with plus / minus on the left, a headphone hexagon top right, and a ring reticle with green rings and a centre dot when the dog is locked; the lower screen shows the hint box with an "i" badge (`Hold A to lock on to the audio source.`) before the view, and the map with a headphone and a return-arrow button during the view.

### Port

`kScannerTakeOutUpdates` 49, `kScannerEnterCallbackUpdates` 20, `kScannerEnterEndUpdates` and `kScannerCameraStartUpdates` 56, `kScannerExitEndUpdates` 57; the exit callback count 20 is a stand-in (SCAN-S1). The take-out count no longer comes from the `scanner_out` clip length.

Unknowns:
- Why state 0x157 comes 49 updates after the press (clip length 1.2 s, alignment 0.67 s); the animation clocks of the spinner transition (20 + 36 updates) and the exit callback time; the exit by B.

<a id="ts-20261019-085330"></a>

## 2026-10-19T08:53:30+02:00 — The audio scanner overlay: layouts and storyboard rules (REA 0x16785c region, 0x2e9030, 0x1d99a8, 0x263400, 0x2ef4c8, 0x1ebd14, 0x245b40; oracle B-1 screenshots)

Corrected by [abilities-scanner.md §2026-10-19T10:01:16](#ts-20261019-100116): the large green ring at lock state 2 is the SoundPulse markers, not a reticle storyboard state.
Corrected by [abilities-scanner.md §2026-10-19T14:13:26](#ts-20261019-141326): 0x1d99a8 tints the Zoom_Meter LayoutRoot, not iconAudioMode, and the tint depends on ability bit 28 of the player character.

### Method

REA (Ghidra 12.1.2) pseudocode of the UI setup (the function around 0x1be9e4 / 0x1befcc, name FUN_0016785c), 0x2e9c1c, 0x2e9eec, 0x2e9030, 0x2ef4c8, 0x1ebd14, 0x245b40, 0x227228, 0x233948, 0x2119d4, 0x263400, 0x20f540, 0x234c04 and the assembly of 0x1d99a8; the layouts with the C++ decoder (`parseLayout`); the screenshots of [§2026-10-19T08:43:45](#ts-20261019-084345). The vtable-like table at 0x34cf94 and the audio / physical / photo reticle objects (0x52451c, 0x5244e8, 0x526834) were read only as far as their setup.

### Layouts

- Setup loads `Sprites/blend/DRC_Scanner/DRC_scanner` (1024 x 512 units; children `Common` = border, mirrored border, `Zoom_Meter`, `Signal_Meter`, `ModeButtons` (opacity 0), `iconAudioMode`; `ModeLayers` with `Physical` and `Audio` (`Reticule_Audio` at 262, 128, 500 x 256, and the six `SonarSegment`s); `info_top`, `info_bot`), the layouts `ScanMeter` (three instances, storyboards `Scanning`, `Off`), `SoundCentre` (50 instances, storyboard `Pulsing`) and `SoundPulse` (45 instances, `Pulse`) and the photo layouts. Every scene's `LayoutRoot` gets the scale 0.5 / 0.5 from 0x20f540 (`0x20f540(0.5, 0.5, scene, 1)`) and 0x234c04 centres it on the 400 x 240 screen. The screenshot confirms it: the zoom meter icons are 93 pixels apart for 187 layout units, and the headphone icon and frame corners land where the half-scale layout (512 x 256, offset -56, -8) puts them.
- Zoom meter (0x1d99a8): the storyboard `Zoom` of `zoom_meter.bxaml` (3.0 s) is set to time = progress x length (0x23caa4) with progress = 1 - (zoom - min) / (max - min) (0x263400; settings +0x18 = 20, +0x1c = 43), so zoom 43 is time 0 (arrow at the minus end) and zoom 20 time 3.0. The same function tints `border`, `border_Copy` and `iconAudioMode` with white, or with a stored colour (+0x18 of 0x1d9ae4) when a flag of the player's component passes 0x23a87c (not read); the three are white in the pictures.
- Reticle (0x2e9030, parameters dt and the reticle UI; its +4 pointer is the mode record +0x14, so the byte +0x28 is the lock state +0x3c, +0x14 the lock progress, +0x1c the scan progress, +4 / +8 the reticle position): the root of `Reticule_Audio` sits at the layout position plus (reticle position - screen centre); the mode position +0x18 / +0x1c comes from 0x2ef4c8 (state 1: linear blend from +0x20 to the target's screen position by the lock progress, 0x2119d4; state 2: on the target; states 5 to 8: on the scanned object or the screen centre; states 0, 3, 4 write nothing).
- Storyboards by state (0x1ebd14 starts the new storyboard and resets the previous one to time 0): states 0 to 4: `Ret_LockingOn` restarted every update (time 0), `Ret_Focus` set to progress x 0.26 with the lock progress, the scan meter restarted; states 5 and 7: `Ret_LockingOn` at scan progress x 2.26 and the `Scanning` storyboard of ScanMeter (3.0 s) at scan progress x length; state 6: `Ret_LockingOn` at its length, ScanMeter storyboard `Off` once when the state changed; state 8: `Ret_Focus` at scan progress x 0.26. Storyboard lengths from the data: Zoom 3.0, Ret_Focus 0.26, Ret_LockingOn 2.26, Scanning 3.0.
- The pieces of `ScanMeter` are 28 images with static rotations (`rot`), `ScaleRoot` scale 0.28: 350 x 0.28 x 0.5 = 49 pixels across, the size of the white arc around the centre of the mid-scan screenshot.
- Zoom timing (0x26f0b4) and the camera yaw are in [§2026-10-18T11:27:21](#ts-20261018-112721).

### Port

`src/runtime/scanner_reticle.{hpp,cpp}` (state to storyboard fractions and reticle position, test `scanner-reticle`), `src/frontend/scanner_overlay.{hpp,cpp}` (draws `drc_scanner.bxaml`, `reticule_audio.bxaml` and `scanmeter.bxaml` with the half-scale layout; `LayoutScene::rotations` makes the layout drawer apply element rotation and track property 5 and lets scale tracks replace the element scale; test `scanner-overlay-data`), glue `LevelSession::drawScannerOverlay` in `src/game_scanner.cpp`. While the view is shown the overlay replaces the hearts HUD on the top screen (the oracle shows no hearts in the view, B-1 screenshot `04_full_zoom`). In state 6 the port draws the scan meter at the end of `Scanning` instead of the `Off` storyboard (SCAN-S6). A chase-game run from the roof trigger shows the frame, zoom meter and headphone icon like the oracle picture.

Unknowns:
- The cyan wash over the 3D view and the green dog (message 0xfb) are not in these layouts (not found). The lock state 2 screenshot shows a large green ring (`ReticuleFocus`) that the rule above (`Ret_LockingOn` at time 0, where `ReticuleFocus` has opacity 0) does not produce; what 0x245b40 / 0x233948 do to a restarted storyboard is not settled.
- What starts `Reticule_Spinner`, `ScanButtonPrompt_*`, `WAV*` and `TargetAquired_On`, the sonar segment intensities (0x2e9030 computes them from the direction of every sound source) and the `SoundCentre` / `SoundPulse` markers; the bottom-screen scanner UI (map with the headphone and return buttons, hint box); the scan meter position beyond "centred on the reticle".

<a id="ts-20261019-100116"></a>

## 2026-10-19T10:01:16+02:00 — Scanner sound markers, map-screen buttons, exit by B, where the wash is not (REA, oracle B-1 and B-2 of round 28)

Corrected by [abilities-scanner.md §2026-10-19T14:06:22](#ts-20261019-140622): the wash is the material override 0x530834 of the audio mode, and there is no green dog (the green shape is the 2D spinner).

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

<a id="ts-20261019-140622"></a>

## 2026-10-19T14:06:22+02:00 — The cyan wash: the material override 0x530834 of the audio mode (REA, llvm-objdump, oracle m12-8)

### Method

REA (Ghidra 12.1.2) pseudocode of 0x2bee98, 0x2ef664, 0x2ef694, 0x2befc0, 0x285c68, 0x29123c, 0x1ee98c, 0x2e4858, 0x128dc8, 0x12e460, 0x1292fc, 0x12df94, 0x180fac, 0x2128c8, 0x228d20, 0x17fc38, 0x2bf830, 0x2b9cf8; llvm-objdump (`--mattr=+vfp2`) of 0x129560 (reads of 0x530834), 0x188778 (draw list item) and the vtable 0x362968. Oracle: state `m12-8-roof-sound-scanner`, OpenGL, `--rpc --rpc-input`, port 45645: `read` of the bytes below before the A press, in the view (A, 5 s later) and after the exit by B; screenshots in the agent's scratch folder (not committed).

### Mechanism

- The audio mode object 0x5216ac (vtable 0x362968: +8 enter 0x2ef664, +0x14 exit 0x2ef694) is entered by the enter callback 0x2bee98 and left by the exit callback 0x2befc0. The enter calls 0x285c68(1): 0x51641e = 1, 0x3772cd = 0; and 0x1ee98c(1): the no-fog byte 0x3760fd = 1 (fog off for every material, rule of 0x128dc8 in [rendering-fog.md](rendering-fog.md)); it also sets 0x52a1cd = 1 (reader 0x1718b8 not followed). The exit clears 0x52a1cd, 0x3760fd and, through 0x29123c, 0x51641e and 0x3772cd.
- The frame function 0x2e4858 (one call per pass) reads 0x51641e. Pass 3 (0x17fc38, then the sky 0x180fac): override block 0x530834 = flags 0x95, 0x530835 (mode) = 3, 0x530837 (texture key) = 0, 0x530840 bit 0 cleared, 0x53083c = 0xffab8126; reset to 0 after the pass. Pass 4 (the sorted draw list 0x228d20(0) -> 0x2128c8): flags 0xd5, reset after. Pass 5 calls the glow post-process 0x2bf830 with (0.6, 1.4) instead of (0x516410, 0.7). Pass 1 skips a camera block (0x52a430 = 0).
- The override block (read by 0x128dc8 for the material key, by 0x129560 for the shader choice and the reflection flag 0x37796d, and by 0x12e460 for the stages): flag bit 0 replaces material byte 1 (mode) with +1 and gives mode 3 the constant +8 instead of white; bit 1 byte 2 (lighting) with +2; bit 2 byte 3 (texture) with +3; bit 3 byte 5 (alpha) with +4; bit 4 the vertex colour flag (key bit 19) with bit 0 of +0xc; bit 5 the scale of the last stage with bit 1 of +0xc; bit 7 forces key bit 22 (the bump stages); bit 6 makes the draw list item 0x188778 skip draws whose material byte 9 (blend destination index) is 1.
- With 0x95 every material becomes: stage 0 the constant (0x26, 0x81, 0xab); the lighting stage of its own lighting key, with modulate turned into replace (so it outputs the fragment lighting, the vertex colour plus lighting, or the constant); then the bump stages of 0x12e460: multiply-add with 0x7f7f7f (0.498 x + 0.498) and dot3 with 0x8ecba6, which is the luminance 0.300 r + 0.590 g + 0.114 b, and a modulate by the combiner buffer, which holds stage 0 (update mask bit of stage 0); the last stage keeps the scale of byte 0xb bit 5 (x2 for doubled materials). Textures are not sampled (texture key 0), but the alpha still comes from the texture slot of the alpha key, so cut-outs (palm leaves) stay. Result: luminance(lighting) x (0.149, 0.506, 0.671), times 2 on doubled materials, clamped: a fully lit doubled surface gives (76, 255, 255), the colour of most sky and wall pixels of the oracle picture.
- 0x180fac (the sky) saves 0x530834 and clears it while 0x3772cd is set; the audio mode clears 0x3772cd, so the sky is washed too. Another mode's enter (0x304698, 0x285c68(0)) sets 0x3772cd = 1 and keeps the normal sky.
- The glow post-process 0x2bf830: copies the screen into a 256 x 128 target (0x52a6b8), blurs it three times between 0x52a6d8 and 0x52a6f8 with the table modes 0xc / 0xd, and adds it to the screen with the table mode 0x12 (read: stage 0 multiply-add of vertex alpha x texture alpha + vertex colour, stages 1 and 2 modulate by the texture, so 2 x (a + c) x T^2 with the x2 flag set when the two arguments add up to more than 1), vertex colour (0.3, 0.3, 0.3, 0.7), blend factors one / one (destination index 0x516414 = 1). In normal play of lvl_01 the first argument 0x516410 is 0, so the glow is off; in the scanner it is on. This explains why the oracle colours are more saturated than the plain formula (red about 0.24 of green instead of 0.295).

### Oracle (m12-8, port 45645)

- In the view: 0x51641e = 1, 0x3760fc / 0x3760fd = 1 / 1, 0x52a1cd = 1, 0x3772cd = 0, 0x530834 = 0 (reset between the passes), 0x530835 = 3, 0x53083c = 0xffab8126, 0x530840 = 0, 0x375ee8..ea = 1, 0, 1. 0x516410 = 0.0, 0x516414 = 1. The table entry of mode 0x12 at 0x596260 holds three stages (source 0x30, operand 0x22, combiner 8; source 0x000f003f, combiner 1; the same again).
- After the exit by B the mode exit had run (0x51641e = 0, 0x3760fd = 0, 0x52a1cd = 0) while 0x375ee8 and +1 were still 1 and the screen showed the full-screen hexagon panel of the transition; 5 s later the scanner was idle and the street view normal.

### Correction of the "green dog"

[Corrects §2026-10-19T10:01:16 (unknown about the wash and the green dog)](#ts-20261019-100116). The SCAN-8 wording "draws the dog green (message 0xfb, `ShowOnObjectScan`)" is not supported by any picture: the green zigzag of the B-1 pictures `02` and `03` (and of this run) is a 2D overlay shape that turns between the pictures (the reticle spinner), and in the locked pictures the dog sits behind a wall, marked only by the centre pip and the rings. In the draw path the dog takes the same override as every other mesh of the draw list. Message 0xfb is `EnableObject` ([scripts.md](scripts.md)), and 0x2b9cf8 (the entity colour record) depends on camera states 0x2297b0 / 0x236104, not on the scanner.

### Port

`assets::MaterialOverride` and `materialState(part, override)` (src/assets/material.cpp), `runtime::scannerRender` (src/runtime/scanner_render.*, values above), `Scene::setMaterialOverride` (src/render/scene.cpp: the overridden stages run through the existing texenv shader `scene.frag.glsl`; flag 6 skips additive draws; the texture units follow the overridden state), glue in `LevelSession::draw`: no fog, the world override for the sky, the draw-list override for the rest while the scanner view is shown. Test `scanner-render`. A `chase-game` picture at the m12-8 spot (the zone forced usable for the picture) shows the same hue as the oracle, darker and less saturated because the glow pass is missing (RENDER-68).

Unknowns:
- Which port draws belong to pass 3 (0x17fc38, 0x95, additive kept) and which to the draw list of pass 4 (0xd5, additive skipped): the port applies the draw-list rule to every non-sky draw (SCAN-S9).
- The wash ends at the mode exit (exit callback 0x2befc0); the port ends it with the view (the exit callback count is SCAN-S1).
- The reader of 0x52a1cd, the blur table modes 0xc / 0xd and the downsample material of 0x2bf830, and the camera block that pass 1 skips.

<a id="ts-20261019-141326"></a>

## 2026-10-19T14:13:26+02:00 — Scanner overlay leftovers: unused storyboards, info boxes, frame tint, sonar segments (REA)

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

<a id="ts-20261019-182129"></a>

## 2026-10-19T18:21:29+02:00 — The scanner device in Chase's hand (hand item track of the scanner clips, parseAnimation scan, chase-game run; no new REA)

Reads the mechanism of [abilities-detective.md §2026-10-18T19:30:00](abilities-detective.md#ts-20261018-193000) (track hash 0xba1e5b53, attach 0x2ca88c, show 0x242cf0 / hide 0x2428c0 in the handler 0x185668); no new function was read.

- `models/char_minifig/class_generic/scanner_out_minifig.bfnanm` (19 frames) and `scanner_in_minifig.bfnanm` (10 frames), the clips of the player roles 0x208 and 0x207, carry the track. Both name the object `Ability_Scanner` and the bone `weaponRight` (`runtime::handItem`).
- Track values (step sampling as for the glass): `scanner_out` 0 up to 0.3 of the clip, 1 from 0.4 to the end (shown); `scanner_in` 0 at the start, 1 from 0.1 to 0.6, 0 from 0.7 (hidden). So the device appears during the take-out, stays through the whole view and disappears near the end of `scanner_in`.
- The object `Ability_Scanner` of `levels/players.blvl` has the meshname `Weap_ScannerDevice` (model `models/weap_scannerdevice/weap_scannerdevice.bfnmdl`), a sibling of the glass among the ability items of that file.
- Port: `game_hand_items::HandItems` (`src/game_hand_items.cpp`, the former glass class) keeps one record per item (object, scene instance, shown flag, bone) and applies each event of the one track detector to the item it names; test `hand-item-real-data` checks the clips, track values and the object. A `chase-game` run on the roof of mission 1-2 (scanner enabled by a local test edit that ignored the zone's disabled state, not committed) shows the device in Chase's right hand during `scanner_out`.

Unknowns:
- The pose of the device (the bone's bind matrix, as for the glass, DETECT-S8) was not compared with an oracle picture; whether the device is drawn in the scanner view (the first-person camera) was not checked; the previous-value handling at the start of `scanner_in`, whose first sample is 0 while the item is shown.
