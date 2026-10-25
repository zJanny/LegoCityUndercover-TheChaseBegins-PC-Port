# Script system: missions, the reference-trigger step

## 1. Scope

The reference-trigger step 0x23211c that the build-it, the switches and other classes call to fire their `TriggerObject` references: its records and lists, the registering kinds, its callers and receivers. Split from [script-system-missions.md](script-system-missions.md).

## 2. Status

IN_PROGRESS, updated 2026-10-25T11:25:18+01:00. The step is ported (all registering kinds in one list, activate and deactivate, owner events, messages 10 / 0xff / 0xfe, nav node requests) and the build-it runs on it; the classes that call it otherwise are open (SCRIPT-358, SCRIPT-359).

## 3. Verified behaviour

- The build-it fires `TriggerObject` through the step (oracle ctrace): [buildit.md §2026-10-15T13:07](../../re/buildit.md#ts-20261015-130720).

## 4. Stand-ins

- SCRIPT-S220 Closed 2026-10-25: the reference list holds the records of every registering kind (`reference_trigger_level.cpp`), not only the `TriggerObject` references of build-its. evidence: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).
- SCRIPT-S289 `applyReferenceActivation` returns the nav node requests of the step (`0x22c470` sets bit 15 of the nav node's halfword +0x34) and no caller applies them; no record of the eight levels has a NavNode other than 255. needs: SCRIPT-360. evidence: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).
- SCRIPT-S290 The reference list registers only the placed objects; the objects nested in class files are not registered (7 `HoseSwitch` children of `vehicles/vehicle_fire_douser` in lvl_07, kind 58); an object created in the default scene only (create flag bit 1, honoured by `entity_ids.cpp`) is registered in every scene, and the records are added in object order, not in the order the object loader creates them. needs: SCRIPT-359. evidence: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).

## 5. Open items

- [ ] SCRIPT-358 — The switch base state machine (kinds 38, 58, 100, 138, 139, 185, 186, 222) calls the step; the port has none of these classes (the mission 1-3 switches, the fires and valves of the Downtown missions, the break-and-enter doors), so their `TriggerObject` joins never fire. needs: SCRIPT-360. blocks: SCRIPT-362, SCRIPT-364. evidence: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).
- [ ] SCRIPT-359 — Callers of the step not wired: destroy, door state change, platform, topple, AI spawner, super build-it (`super_buildit.*`) and message 8 to a build-it. blocks: SCRIPT-S290. evidence: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).
- [ ] SCRIPT-360 — Receivers of the step: nothing reads message 10 (join), the message 0xff / 0xfe handlers of most classes are not ported (in the port only doors and the detective answer them), and the NavNode flag with the level-start pass is not applied. blocks: SCRIPT-358, SCRIPT-S289. evidence: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).

## 6. Findings summary

- Reference-trigger step 0x23211c, its records and lists, events, messages, registering kinds and callers: [script-triggers.md §2026-10-25T11:21:25](../../re/script-triggers.md#ts-20261025-112125).

## 7. Code and tests

- `src/runtime/reference_trigger.{hpp,cpp}` (the step), `reference_trigger_level.*` (the list of a level and the kind table), `reference_trigger_apply.*` (effects on the runner), caller `applyBuilditActions`; tests `reference-trigger` (made-up values) and `reference-trigger-real-data` (level records, the Downtown fire and engine joins, the build-it on lvl_01).
