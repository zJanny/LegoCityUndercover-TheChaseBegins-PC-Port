# Sound scanner

The sound scanner of mission 1-2 and later missions: the zone class `props/prop_scannerzoneaudio` (object type 201), the scannable objects (`props/prop_dogwaggles` and others with `ShowOnAudioScan`), the player states of the scanner, the scanner view (aim, zoom, lock-on, scan) and the ObjectTriggered events they fire. Port: `src/runtime/scanner.{hpp,cpp}`, glue `src/game_scanner.cpp`.

## Current state

- Type 201 descriptor 0x1863c8: create 0x19f6d0, parameter reader 0x192008, message handler 0x1aea24 (message 3 = use), no update. The zone's trigger objects sit at component +0x5c..+0x74; `ScannerMode` at +0x4c. [§2026-10-18T11:27:21](#ts-20261018-112721)
- Enter: A press in `UseBound` -> message 3 -> 0x2249e0(1, zone) -> player event 201 -> state 0x156 "Scanner Enter" (clip `scanner_out`) and the alignment 0x2248c4; when the clip ends state 0x157 "Scanner Active" (0x1bf2d0) starts the spinner transition; `EnterScanTriggerObject` fires 3 updates later (0x2bee98). Measured in the oracle. [§2026-10-18T11:27:21](#ts-20261018-112721)
- View: camera update 0x26f0b4 (settings block 0x1e06ac: zoom 43..20 at 35 per second with L/R, turn 60 degrees per second, pitch -25..40); `ZoomedInTriggerObject` fires in every update at full zoom (oracle: from frame 49 on). Lock 0x2ef4c8 (1.5 per second, `LockedOnTriggerObject`), scan 0x2ef310 (A held, 0.75 per second, `TriggerScanned` of the target once). [§2026-10-18T11:27:21](#ts-20261018-112721)
- Exit: B (slot 7) or `EnableScanner 0` (0x1cd9dc) -> event 201 -> state 0x158 (0x1bf478) -> transition with 0x2befc0 (`ExitScanTriggerObject`, clip `scanner_in`) and 0x1a2824 (`CompleteExitTriggerObject`, flags cleared). [§2026-10-18T11:27:21](#ts-20261018-112721)
- Emulator: in Azahar the camera yaw word 0x376894 reads NaN once the scanner runs, the scannable records never get a screen position, and the game stops responding after the exit (both runs). This is the likely cause of the white view; lock, scan and exit timing could not be measured there. [§2026-10-18T11:27:21](#ts-20261018-112721)

<a id="ts-20261018-112721"></a>

## 2026-10-18T11:27:21+02:00 — The sound scanner: classes, player states, view, triggers (REA, objdump, Azahar GDB traces of m12-8)

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
