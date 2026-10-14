# Script system: trigger pass and trigger events

## 1. Scope

The trigger pass 0x1746c0 with the bound checks (BoundEntered, BoundLeft, ObjectBound*, on-foot and vehicle variants), the runtime trigger and bound records, EnableTrigger / EnableBound lookups, the trigger events fired by AI and vehicle code, and the trigger start rules. General items, the runner and the scene start are in [script-system.md](script-system.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T16:48:13+02:00. The trigger pass checks every bound trigger type (script_triggers.*); vehicle types wait for SCRIPT-43. Player bound triggers and `TriggerObject` start scripts; the `AIMove` trigger fires when the stand-in move ends.

## 3. Verified behaviour

- Bound trigger checks (BoundEntered, BoundLeft, ObjectBound*, on-foot variant) follow the disassembly; the police entrance walk matches the oracle (`script-triggers-real-data`): [script-triggers.md §2026-10-14T15:53](../../re/script-triggers.md#ts-20261014-155345).

## 4. Stand-ins

- SCRIPT-S8 Triggers start enabled except the ones SCRIPT-S15 switches (the record builder was not read), and the object flag 0x100000 that forces an ObjectTriggered start is taken as clear. The trigger pass does not run while the startup is stepped before the first frame.
- SCRIPT-S15 A trigger whose object name is set and names no object of the level starts disabled: a data fit (lvl_01 triggers 79-82 and 141, lvl_00 none), the record builder was not found. needs: SCRIPT-51.
- SCRIPT-S28 The position of an ObjectBound* trigger's object is its placement (objects never move in `chase-game`); characters, vehicles and platforms are where they were placed. needs: SCRIPT-77. evidence: [script-triggers.md §2026-10-14T15:53](../../re/script-triggers.md#ts-20261014-155345).
- SCRIPT-S29 The player is never in a vehicle: PlayerOnFootBoundEntered runs, PlayerVehicleBoundEntered never does. needs: SCRIPT-76. evidence: [script-triggers.md §2026-10-14T15:53](../../re/script-triggers.md#ts-20261014-155345).
- SCRIPT-S30 Object flag bits 0x20 (object bound checks) and 0x100000 (forced start) are taken as clear; an unloaded room's owner entity counts as present. needs: SCRIPT-78. evidence: [script-triggers.md §2026-10-14T15:53](../../re/script-triggers.md#ts-20261014-155345).

## 5. Open items

- [ ] SCRIPT-75 — Trigger events fired by AI code: AIMove fires at the end of an AIFollowSimplePath (argument 255, 0x2cbf10, original rules) and when a straight AIMove ends (stand-in arrival, SCRIPT-S31; sites 0x2c3bd8 / 0x2c630c); CharacterArrested 3, CharacterArrestable 2, CharacterAvoidArrest 1, PursuitEnded 1 in lvl_01 and CharacterCulled 0x2aadc8 are not. needs: SCRIPT-42, SCRIPT-79. blocks: SCRIPT-1. evidence: [script-triggers.md §2026-10-14T15:53](../../re/script-triggers.md#ts-20261014-155345).
- [ ] SCRIPT-76 — Vehicle trigger events: PlayerVehicleEntered 1, VehicleChaseEnded 1, PlayerVehicleBoundEntered 5 (the pass runs them only while 0x24255c(player) is set; the port is always on foot). needs: SCRIPT-43, VEH-4. blocks: SCRIPT-1. evidence: [script-triggers.md §2026-10-14T15:53](../../re/script-triggers.md#ts-20261014-155345).
- [ ] SCRIPT-77 — ObjectBound checks of moving objects (characters, vehicles, platforms: 1-3_Ledge_Platform, race vehicles, Tony Knuckles, Dogtruck) use the placement position. needs: SCRIPT-42, SCRIPT-43, SCRIPT-44, VEH-1. blocks: SCRIPT-S28. evidence: [script-triggers.md §2026-10-14T15:53](../../re/script-triggers.md#ts-20261014-155345).
- [ ] SCRIPT-78 — Triggers of class files and of the master scene (0x23cff4 also searches it) are not in the pass; the owner entity of an unloaded room's trigger and the flag bits 0x20 of the object and 0x100000 of +0xc are unread. needs: SCRIPT-51, TRANS-3. evidence: [script-triggers.md §2026-10-14T15:53](../../re/script-triggers.md#ts-20261014-155345).
- [ ] SCRIPT-51 — The builder of the runtime trigger records (enable byte, skip byte, owner object) was not found; the rule behind SCRIPT-S15 is unconfirmed, and bound-event owners (`BoundEntered` with an object name) are not understood. blocks: SCRIPT-S15. evidence: [scene-start.md §2026-10-14T14:27](../../re/scene-start.md#ts-20261014-142739).

## 6. Findings summary

- Check functions of every bound trigger type, oracle runs of BoundLeft (why a teleport fails): [script-triggers.md §2026-10-14T15:53](../../re/script-triggers.md#ts-20261014-155345).
- Trigger and bound records, EnableTrigger / EnableBound lookups, the bound test 0x22f650, the room switch +0xb, oracle comparison, why the police entrance did not fire: [scripts-runtime.md §2026-10-14T10:06](../../re/scripts-runtime.md#ts-20261014-100640).

## 7. Code and tests

- `applyTriggerCheck` / `classifyTriggerEvent` (`src/runtime/script_triggers.*`, called by `ScriptRunner::checkTriggers`; the object position source is set in `src/game_level.cpp`); CTest `script-triggers`, `script-triggers-real-data`.
