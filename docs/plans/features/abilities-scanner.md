# Abilities: sound scanner

## 1. Scope

The sound scanner of mission 1-2 and later missions: the `prop_scannerzoneaudio` object class, entering the scanner view, zooming and locking on a target, and the `ObjectTriggered` events that start `Scanner_Enter_Script`, `Scanner_Zoomed_Script`, `Scanner_Locked_Script` and `Scanned_Pringles` (part of SCRIPT-111 in [script-system-missions.md](script-system-missions.md)). The detective tool is in [abilities-detective.md](abilities-detective.md).

## 2. Status

IN_PROGRESS, updated 2026-10-18T11:27:21+02:00. The audio zone (type 201, ScannerMode 1), the scannable objects, the player states 0x156 / 0x157 / 0x158, aim, zoom, lock, scan, exit and all zone and target triggers are ported in `chase-game`; from the roof trigger of mission 1-2 the four scanner scripts start and `EnableScanner 0` closes the scanner (test `scanner-real-data`). The overlay (frame, zoom meter, headphone icon, reticle, scan meter) is drawn from the original layouts; the cyan wash, the dog shown only in the scanner and the bottom-screen scanner UI are not (SCAN-8, SCAN-9). Real-time transition counts are ported.

## 3. Verified behaviour

- Class values of `1-2_ScannerZoneAudio` and `1-2_Dog_Prop` from the level data; settings block and audio mode values read in the oracle (m12-8).
- Real-time timing (oracle B-1): state 0x157 49 updates after the press, `EnterScanTriggerObject` 20 later, toggle and camera 56 after the entry, lock 20, scan 39, script exit 57 updates; `ZoomedInTriggerObject` every update at full zoom.

## 4. Stand-ins

- SCAN-S1 — Transition counts are measured in real time (49 / 20 / 56 / 57 updates) but the animation clocks behind them are not read; the exit callback count (20) is not observed. evidence: [abilities-scanner.md §2026-10-19T08:43:45](../../re/abilities-scanner.md#ts-20261019-084345)
- SCAN-S2 — View math: aim and pitch limits applied to the aim offsets with assumed signs, quaternion slerp for 0x21a600, projection with the follow camera's scales for zoom degrees and a 400 x 240 screen (0x22e1cc / 0x25b578 not read). evidence: [abilities-scanner.md §2026-10-18T11:27:21](../../re/abilities-scanner.md#ts-20261018-112721)
- SCAN-S3 — Scannable list: every object whose class gives `ShowOnAudioScan` 1, target point = placement + model bounds centre, radius 1, all with the linked range 100 (list builder for 0x526aec not read). evidence: [abilities-scanner.md §2026-10-18T11:27:21](../../re/abilities-scanner.md#ts-20261018-112721)
- SCAN-S4 — Player clip while scanning (scanner_out held until the exit callback, then scanner_in) and the exit facing from the scanner view yaw. evidence: [abilities-scanner.md §2026-10-18T11:27:21](../../re/abilities-scanner.md#ts-20261018-112721)
- AUDIO-S33 — Scanner sounds go through the port's sound player with owner 0xfff0; the original plays them on the 2D list 0x51a3ac (0x240944, not read). evidence: [abilities-scanner.md §2026-10-18T11:27:21](../../re/abilities-scanner.md#ts-20261018-112721)

- SCAN-S5 — Overlay layout: half scale centred on the screen, layers whose state is not read hidden (`Physical`, `Sonar`, `info_top`, `info_bot`, `Signal_Meter`), the animation evaluator is the data fit with rotation (`LayoutScene::rotations`), the scan meter centred on the reticle. evidence: [abilities-scanner.md §2026-10-19T08:53:30](../../re/abilities-scanner.md#ts-20261019-085330)
- SCAN-S6 — Reticle storyboards: `Reticule_Spinner` runs from the start of the update clock, state 6 shows `Scanning` at its end (the original starts `Off` once), the restart semantics of 0x245b40 are read as "time 0". evidence: [abilities-scanner.md §2026-10-19T08:53:30](../../re/abilities-scanner.md#ts-20261019-085330)

## 5. Open items

- [ ] SCAN-2 — Other type 201 zones: photo (ScannerMode 4 in the class, sub-mode 0 values of 0x2ef814), physical (kind 2), satellite (mode 3, `SatellitePower`), cannon; `EnableScanner 1` without a zone. evidence: [abilities-scanner.md §2026-10-18T11:27:21](../../re/abilities-scanner.md#ts-20261018-112721)
- [ ] SCAN-3 — Not transcribed: vehicle veto of message 3 (message 0x155), view collision push-out of 0x1ebe88, `CompleteTriggerObject`, `SkyboxOverride`, `TeleportTo`, alien / reward branch 0x2ef144, `ATTR_SFX_USE` / `ATTR_SFX_MOVE`, listener placement of 0x187f78. evidence: [abilities-scanner.md §2026-10-18T11:27:21](../../re/abilities-scanner.md#ts-20261018-112721)
- [ ] SCAN-5 — `EnableScanner` is applied at the next scanner update (the original toggles inside the native). evidence: [abilities-scanner.md §2026-10-18T11:27:21](../../re/abilities-scanner.md#ts-20261018-112721)
- [ ] SCAN-6 — A `chase-game --script` run from the roof trigger (only the runtime test drives the sequence so far). evidence: [abilities-scanner.md §2026-10-18T11:27:21](../../re/abilities-scanner.md#ts-20261018-112721)

- [ ] SCAN-7 — Exit by B (slot 7) in real time: the exit callback count and the length when the player leaves by B (the script exit came first in B-1). evidence: [abilities-scanner.md §2026-10-19T08:43:45](../../re/abilities-scanner.md#ts-20261019-084345)

- [ ] SCAN-8 — The cyan wash over the 3D view and the dog drawn green only in the scanner (message 0xfb, `ShowOnObjectScan`): where the original does it is not found (not in the DRC_Scanner layouts). needs: SCAN-1. evidence: [abilities-scanner.md §2026-10-19T08:53:30](../../re/abilities-scanner.md#ts-20261019-085330)
- [ ] SCAN-9 — Remaining overlay parts: the large green ring at lock state 2, `Reticule_Spinner` / `ScanButtonPrompt_*` / `WAV*` / `TargetAquired_On` triggers, sonar segments, sound centres and pulses, `Scanner_PFX`, info boxes, the bottom-screen scanner UI (hint box, map with headphone and return buttons), the tint rule of 0x1d99a8. needs: SCAN-1. evidence: [abilities-scanner.md §2026-10-19T08:53:30](../../re/abilities-scanner.md#ts-20261019-085330)

## 6. Findings summary

- Overlay layouts and storyboard rules: [abilities-scanner.md §2026-10-19T08:53:30](../../re/abilities-scanner.md#ts-20261019-085330). Real-time timing: [abilities-scanner.md §2026-10-19T08:43:45](../../re/abilities-scanner.md#ts-20261019-084345). Scanner classes, states, view and triggers: [abilities-scanner.md §2026-10-18T11:27:21](../../re/abilities-scanner.md#ts-20261018-112721). Mission 1-2 gap list: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921); oracle state `m12-8`: [oracle-states.md §2026-10-18T10:13:06](../../re/oracle-states.md#ts-20261018-101306).

## 7. Code and tests

- `src/runtime/scanner.{hpp,cpp}` (zone and scannable load, state machine, view, lock, scan, events), `src/game_scanner.cpp` (LevelSession glue: pad, EnableScanner, triggers, sounds, player pose, view override), `SoundPlayer::playing` / `GameAudio::playing`.
- CTest `scanner` (made-up zone: timings, lock 20 updates, scan, zoom, exit, pitch limits) `scanner-reticle`, `scanner-overlay-data` (layout values and drawing), and `scanner-real-data` (lvl_01 from the m12-8 position: Enter, Zoomed, Locked, Scanned_Pringles, EnableScanner 0 exit).
