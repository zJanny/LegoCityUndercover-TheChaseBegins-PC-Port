# Level objects: fast travel pad

## 1. Scope

The fast travel pad (kind 227) that the super build-it of the helipad of mission 1-3 creates: its use zone, the player's console states 0x180..0x183, the Helicopter that lands and takes off, the exit and the arrival. The super build-it itself is in [level-objects-buildit.md](level-objects-buildit.md); the map buttons and the map screen are UI-1 in [ui-menus.md](ui-menus.md).

## 2. Status

IN_PROGRESS, updated 2026-10-25T11:24:05+01:00. On the built pad A puts the player at the console (states 0x180 and 0x181, clips `typing_stood`) and the Helicopter lands (`land`, then `idle`), with the oracle's timing. A mouse click (the touch screen) on the exit button's place, once the Helicopter idles, lets the Helicopter take off (`takeoff` once, 96 updates) and returns the player to state 1 in the same update; the Helicopter is hidden when the clip ended, and the pad can be used again. The arrival state 0x183 (Helicopter idles for a second, then takes off; the player waves; state 1 at the end) runs after a scene change whose object has a `FastTravel` reference, but no map exists to start such a change yet. The map page itself (level icons, the question, the buttons' pictures) is not ported (LEVOBJ-120, UI-1).

## 3. Verified behaviour

- Fast travel pad `1-3_Helipad.SBFastTravel` (kind 227): zone `UseBound`, A requests state 0x180; the approach lasts 22 updates and `idle` starts 63 updates after 0x181 at dt 1/30, the oracle's 39 to 45 and 124 to 126 vblanks; the pad stands at the oracle's player position with heading 24576 (CTests `fast-travel-pad`, `fast-travel-pad-real-data`, `game-super-buildit-build`): [fast-travel.md §2026-10-20T00:22:29](../../re/fast-travel.md#ts-20261020-002229).
- Exit: the map's exit button takes effect once 0x28b930 answers ready; `takeoff` starts once on the Helicopter, player state 1 is entered in the same update, the Helicopter is hidden 96 updates later (oracle: 191 vblanks after the first row with state 1, `takeoff` already running in that row; clip 9600 ticks = 48 frames); state 0x182 is not used (CTests `fast-travel-pad`, `game-fast-travel-exit`): [fast-travel.md §2026-10-25T11:21:51](../../re/fast-travel.md#ts-20261025-112151).
- Arrival 0x183: `idle` loops, `takeoff` on the 30th update (float timer greater than 1.0), state 1 on the 127th; the reference `FastTravel` of `FastTravel_01_CherryTreeHills` names the holder `1-3_Helipad` (CTests `fast-travel-pad`, `fast-travel-pad-real-data`): [fast-travel.md §2026-10-25T11:21:51](../../re/fast-travel.md#ts-20261025-112151).

## 4. Stand-ins

- LEVOBJ-S96 The pad's use query runs when the port's "player free" flag allows it, and a requested player state (0x180, 0x181, 0x183) is entered in the next update. needs: LEVOBJ-122. evidence: [fast-travel.md §2026-10-20T00:22:29](../../re/fast-travel.md#ts-20261020-002229).
- LEVOBJ-S97 The exit button's click area is the centre (159, 65) and radius 32 of its layout rectangle (`area_exit`), fired by a left click that starts on the bottom screen; the original's resolved position was not compared and its map buttons screen is not ported. needs: UI-1. evidence: [fast-travel.md §2026-10-25T11:21:51](../../re/fast-travel.md#ts-20261025-112151).
- LEVOBJ-S98 The order of the map update, the pad update and the player's state step is not read: the exit is taken in the pad update of the touch, state 1 and `takeoff` start in it, and `--script` gets a `touch X Y COUNT` step to test it. evidence: [fast-travel.md §2026-10-25T11:21:51](../../re/fast-travel.md#ts-20261025-112151).

## 5. Open items

- [ ] LEVOBJ-120 — The rest of the fast travel map: the page switch of the map buttons screen, the six level icons and their unlock bits, the "go there?" question and the Yes path (message to the `FastTravel_0n_*` object). needs: UI-1. blocks: LEVOBJ-123. evidence: [fast-travel.md §2026-10-25T11:21:51](../../re/fast-travel.md#ts-20261025-112151).
- [ ] LEVOBJ-121 — The rest of the pad: messages 0xff and 0xfe to the child `pfx` (particles, needs PART-3), the camera of the fast travel states (`CameraBound`, `CameraBoundArrive` record byte), the pad list of the map, `LinkedObject`, `VisibleBound` and the second slot set of the state objects. evidence: [fast-travel.md §2026-10-25T11:21:51](../../re/fast-travel.md#ts-20261025-112151).
- [ ] LEVOBJ-122 — The gate of the use registry walk and the update in which a requested player state is entered are not read. blocks: LEVOBJ-S96, LEVOBJ-126. evidence: [fast-travel.md §2026-10-20T00:22:29](../../re/fast-travel.md#ts-20261020-002229).
- [ ] LEVOBJ-123 — The arrival has no way to start in `chase-game` before the map's Yes path exists, so the scene change switch carrying the `FastTravel` holder (`Options::fastTravelArrival`) is only tested by unit tests and the real data; run it once the map exists. needs: LEVOBJ-120. evidence: [fast-travel.md §2026-10-25T11:21:51](../../re/fast-travel.md#ts-20261025-112151).
- [ ] LEVOBJ-124 — State 0x182 "Fast Travel Exit": who raises the event "Fast Travel Active Exit Event" (type 201), and the table entry that closes the map from its update. evidence: [fast-travel.md §2026-10-25T11:21:51](../../re/fast-travel.md#ts-20261025-112151).
- [ ] LEVOBJ-125 — The arrival's conditions at the level start (the player not in state 0x81, the table taken to be the active scene change object) and the pad's object state after the switch: the port does not restore the helipad's built state (LEVOBJ-43), so the pad of a new session is disabled and hides its Helicopter. needs: LEVOBJ-43. evidence: [fast-travel.md §2026-10-25T11:21:51](../../re/fast-travel.md#ts-20261025-112151).
- [ ] LEVOBJ-126 — The exit click has no cutscene gate in the port, and the pad becomes usable only long after cutscene `1_3_a` ends (the running `1-3_Helipad_Built` keeps the player held); the original's gates are not read, and the tests tap during the cutscene. needs: LEVOBJ-122. evidence: [fast-travel.md §2026-10-25T11:37:18](../../re/fast-travel.md#ts-20261025-113718).

## 6. Findings summary

- Fast travel pad, states 0x180..0x183, Helicopter, map query, oracle timing: [fast-travel.md §2026-10-20T00:22:29](../../re/fast-travel.md#ts-20261020-002229).
- The map's exit (0x2d9f84, 0x1ee5f8, 0x26aa60), the arrival state 0x183, state 0x182, the `FastTravel` reference of the scene changes, oracle timing of the exit: [fast-travel.md §2026-10-25T11:21:51](../../re/fast-travel.md#ts-20261025-112151).
- The real order build, cutscene, pad, exit in the port (delay before the pad answers, no exit gate): [fast-travel.md §2026-10-25T11:37:18](../../re/fast-travel.md#ts-20261025-113718).

## 7. Code and tests

- `src/runtime/fast_travel_pad.*` (`FastTravelPads` with `requestExit`, `arrive`, `fastTravelExitTapped`, `loadFastTravelHelicopter`, `fastTravelHelicopterPose`), glue `src/game_fast_travel.*` (the click on the bottom screen, the log lines), `SceneChangeDestination::fastTravelHolderHash` in `src/runtime/scene_change.*`, the script step `touch X Y COUNT` in `src/game_options.*`; CTest `fast-travel-pad`, `fast-travel-pad-real-data`, `game-options-core`, `game-super-buildit-build` (slow), `game-fast-travel-exit` (slow).
