# Script system: missions, the reference-trigger step

## 1. Scope

The reference-trigger step 0x23211c that the build-it, the switches and other classes call to fire their `TriggerObject` references: its records and lists, the registering kinds, its callers and receivers; the switch base class (kinds 38 and 58 and the other switch kinds) that calls it. Split from [script-system-missions.md](script-system-missions.md).

## 2. Status

IN_PROGRESS, updated 2026-10-26T16:14:15+01:00. The step is ported (all registering kinds in one list, activate and deactivate, owner events, messages 10 / 0xff / 0xfe, nav node requests) and the build-it runs on it. The switch base (loader, stable update, state change, base handler) runs for kinds 38 and 58 with the lever's use zone and handler, the use animation's timing, the zone bits of ToggleUseable and the player's use state; the mission 1-3 key switch opens the garage door. The other switch kinds and the other callers are open (SCRIPT-365, SCRIPT-359).

## 3. Verified behaviour

- The build-it fires `TriggerObject` through the step (oracle ctrace): [buildit.md §2026-10-15T13:07](../../re/buildit.md#ts-20261015-130720).
- The switch base and the lever follow the code read (CTest `switch`); the key switch of mission 1-3 fires `1-3_Garage_Key_used` in `chase-game` with the oracle's update counts (CTests `game-garage-key-1-3`, `game-key-use-1-3`), [switches.md §2026-10-25T16:34:17](../../re/switches.md#ts-20261025-163417). evidence: [switches.md §2026-10-25T16:34:17](../../re/switches.md#ts-20261025-163417).

## 4. Stand-ins

- SCRIPT-S220 Closed 2026-10-25: the reference list holds the records of every registering kind (`reference_trigger_level.cpp`), not only the `TriggerObject` references of build-its. evidence: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).
- SCRIPT-S289 `applyReferenceActivation` returns the nav node requests of the step (`0x22c470` sets bit 15 of the nav node's halfword +0x34) and no caller applies them; no record of the eight levels has a NavNode other than 255. needs: SCRIPT-360. evidence: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).
- SCRIPT-S290 The reference list registers only the placed objects; the objects nested in class files are not registered (7 `HoseSwitch` children of `vehicles/vehicle_fire_douser` in lvl_07, kind 58); an object created in the default scene only (create flag bit 1, honoured by `entity_ids.cpp`) is registered in every scene, and the records are added in object order, not in the order the object loader creates them. needs: SCRIPT-359. evidence: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).
- SCRIPT-S292 The A press on a lever counts only under the gate the detective and use-spot glue use (on the ground, not carrying, not in a build-it zone, no script or detective control, no cutscene), a gate chosen by the port instead of the press path 0x1fee94. evidence: [switches.md §2026-10-25T16:34:17](../../re/switches.md#ts-20261025-163417).
- SCRIPT-S293 The switch's own use animation (+0x18) is timed but not drawn; the `light_on` / `light_off` nodes, the sound pitch from `FrequencyRange`, the fade of `DisableAfterUse` (no shipped switch sets it), the `Icon` child and `levertwitch` are not applied; disabled switches get no update and a lever without a class `UseBound` gets no zone (0x23e130 builds one from the node bounds; gates chosen by the port). needs: SCRIPT-370. evidence: [switches.md §2026-10-25T16:34:17](../../re/switches.md#ts-20261025-163417).

- SCRIPT-S299 Closed 2026-10-26: `SwitchSet::update` advances the switches' animation clock before their update; with it a 24-frame use animation at 15 per second ends state 1 after 49 updates of 0.033, as the oracle's pull switch (state 1 in update 9, state 3 in update 58; the other order gives 48). evidence: [switches.md §2026-10-26T16:12:38](../../re/switches.md#ts-20261026-161238).

## 5. Open items

- [ ] SCRIPT-359 — Callers of the step not wired: destroy, door state change, platform, topple, super build-it (`super_buildit.*`) and message 8 to a build-it. blocks: SCRIPT-S290, SCRIPT-410. evidence: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).
- [ ] SCRIPT-360 — Receivers of the step: the switches (kinds 38, 58: 0xff, 0xfe, 8, 54; message 10 ignored), doors and the detective answer them; the 0xff / 0xfe handlers of the other classes are not ported and the NavNode flag with the level-start pass is not applied. blocks: SCRIPT-S289. evidence: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).
- [ ] SCRIPT-365 — The other switch kinds (100 energy switches, 125, 138 fires, 139 switchfloor, 141, 185 ability props and break-and-enter, 186 colour swappers, 201, 222 freeze targets, 227, 232) do not run: their update, handler and extra state are not read, so their `TriggerObject` never fires. evidence: [switches.md §2026-10-25T16:34:17](../../re/switches.md#ts-20261025-163417).
- [ ] SCRIPT-370 — Lever details still open: the class type `*0x37674c` that gates the use animation and `levertwitch`, message 7 (0x2320f8), message 41 to the controlled objects, state 77 (`HoldMe`), USE WALK (state 21), 0x2520e8 for LeverType 5 and the parent case of 0x239678 (kind 183) in the ability check. blocks: SCRIPT-S293. evidence: [switches.md §2026-10-26T13:57:52](../../re/switches.md#ts-20261026-135752).

## 6. Findings summary

- Reference-trigger step 0x23211c, its records and lists, events, messages, registering kinds and callers: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).
- Switch base record, states, stable update, state change, base handler, lever class and the mission 1-3 key switch: [switches.md §2026-10-25T16:34:17](../../re/switches.md#ts-20261025-163417).
- The player's lever use, 0x245f84, 0x219a8c, `DisableAfterUse`, zone bit 6, the lever's use marker: [switches.md §2026-10-26T13:57:52](../../re/switches.md#ts-20261026-135752).

## 7. Code and tests

- `src/runtime/reference_trigger.{hpp,cpp}` (the step), `reference_trigger_level.*` (the list of a level and the kind table), `reference_trigger_apply.*` (effects on the runner), callers `applyBuilditActions` and the AI spawner glue `src/runtime/game_ai_spawner_glue.cpp`; tests `reference-trigger` (made-up values) and `reference-trigger-real-data` (level records, the Downtown fire and engine joins, the build-it on lvl_01).
- `src/runtime/switch_base.{hpp,cpp}` (loader, post-link, 0x21435c, 0x22c6d0, 0x23aac0), `src/runtime/switch_set.{hpp,cpp}` (kinds 38 and 58, the lever handler and zone, the step inside the state change, the use animation's slots, 0x236624, the marker query), `src/runtime/lever_use.{hpp,cpp}` (the player's use); glue `src/game_switch_glue.cpp` (log lines `switch NAME: used`, `switch NAME: state N`), `src/game_switch_use_glue.cpp`; tests `switch`, `lever-use`, `lever-use-real-data`, `game-garage-key-1-3`, `game-key-use-1-3`.
