# Doors

## 1. Scope

Doors (object type 42): record, update 0x1d16b8, state change, messages, easing, hinge and slide pose, timings, sounds. Collision of a door is in [collision.md](collision.md); the scripts that ask doors to open are in [script-system.md](script-system.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T09:50:11+02:00. Doors with `AutoOpen` open and close in `chase-game`; doors that need a script request never open.

## 3. Verified behaviour

- State machine, timer and linear angle: the Police door opens in 22 updates (90 degrees / 22 per update) and closes mirrored; `door-real-data` pins the poses at timers 1, 11 and 22 for both directions to 2e-6.
- Hinged types 0 and 1 and sliding types 5 to 8 and 0x11 follow the transcribed pose rules.
- Sounds follow the state change 0x1a6b6c: opening sound when the timer equals the delay in state 2, stop of it and the open sound at open, closing sound at the start of closing, stop of it and the shut sound when closed. Positional at the door; the stops end the voice at once (door sounds have fade 0). `door-core` pins the order, `door-real-data` the ids of Police_Door_02 (605, 601; shut and open are 0). Details: [doors.md §2026-10-13T21:51](../../re/doors.md).
- A door opens by itself when `AutoOpen` is set (instance parameter, the lvl_00 lift and double doors) and the player is inside its zone, or on message 0xff (from `TriggerObject` / the platform link).

## 4. Stand-ins

- DOOR-S1 The proximity test uses the player position of `chase-game`.
- DOOR-S2 Sound id 0 is skipped (assumed to mean none).
- DOOR-S3 Types 2, 3, 4, 9 to 0x13 use the identity pose (motion not decoded); easing 4 and the custom curves, the DoorAnim path (states 2 / 4 wait for the animation object), partial opening and the bounce are transcribed but not measured.
- DOOR-S4 An open door keeps the closed-state box (see DOOR-3).
- DOOR-S5 The owner key of a door's sounds is its index in the level's door list plus 1 (the original uses the u16 at entity+0x14, meaning unknown).

## 5. Open items

- [ ] DOOR-1 — Some doors in lvl_01 (Cherry Tree Hills, for example `Police_Door_01/02`) open only when a game script asks. Scripts do not run, so these doors stay closed. No guessed trigger was added; whether the player's "use" action also opens them is unknown. needs: SCRIPT-1 ([script-system.md](script-system.md)). evidence: [doors.md §2026-10-13T21:17](../../re/doors.md), [objects.md §2026-10-13T21:17](../../re/objects.md), [scripts.md §2026-10-13T20:42](../../re/scripts.md).
- [ ] DOOR-2 — Easing 4 and the custom curves, door types 2, 3, 4, 9 to 0x13 (motion), the DoorAnim path, partial opening and the bounce are transcribed or missing without oracle measurements. evidence: [doors.md §2026-10-13T21:17](../../re/doors.md).
- [ ] DOOR-3 — An open door keeps the closed box in `chase-game`, although one oracle walk differed (the open door held the character still at (905.755, 1116.386) for 30 updates while the closed door let him slide); why is not explained. needs: COLL-4. evidence: [doors.md §2026-10-13T21:17](../../re/doors.md).
- [ ] DOOR-4 — The rest-matrix-before-pose draw order (`placement * pose * nodeMatrix`) is unverified against a picture of the original. evidence: [doors.md §2026-10-13T21:17](../../re/doors.md).
- [ ] DOOR-6 — The meaning of the player flag set by 0x1d16b8 in states 2 and 4 (`player ctrl +0x316 |= 0x10`). evidence: [doors.md §2026-10-13T21:17](../../re/doors.md).
- [ ] DOOR-7 — Fixed in code 2026-10-14 (two-node doors are drawn as two instances, each with its own pose; the cause was the renderer, not `doorPose`), but not yet checked against a picture of the original with the lift doors open. Closed collision box of an open door still applies (DOOR-3). evidence: [doors.md §2026-10-14T09:50](../../re/doors.md).

## 6. Findings summary

- Record, update 0x1d16b8, state change and messages, easing, poses, timings, sounds: [doors.md §2026-10-13T21:17](../../re/doors.md).
- Sound call sites and order: [doors.md §2026-10-13T21:51](../../re/doors.md).
- Two-leaf doors: node poses and rendering: [doors.md §2026-10-14T09:50](../../re/doors.md).
- Trigger chain (`AutoOpen`, message 0xff) and where the start state of doors comes from: [objects.md §2026-10-13T21:17](../../re/objects.md), [scripts.md §2026-10-13T20:42](../../re/scripts.md).

## 7. Code and tests

- `src/runtime/door.*`, doors wired into `chase-game`; oracle tool command `poke`.
- CTest: `door-core`, `door-real-data`. Scenarios: `tools/oracle/objcoll-door.txt`.
