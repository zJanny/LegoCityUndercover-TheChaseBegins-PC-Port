# Script system: objective and text natives

## 1. Scope

The objective natives (SetActiveWayPoint, SubObjectiveGoToEnable, arrows, map and scanner natives) and the text natives (tip text, dialogue lines, mission title, intro and tally screens) of batches 30 and 31. See [script-system-natives.md](script-system-natives.md) for the other natives and [script-system.md](script-system.md) for the runner.

## 2. Status

IN_PROGRESS, updated 2026-10-19T23:16:35+02:00. SetActiveWayPoint, SubObjectiveGoToEnable and the two arrow natives follow the original: waypoints fade out on a script disable (any accepted disable outside state 3, with the 16 map slots and the vehicle waypoint global), sub-objective groups show their target icons and a fading group slot on the map, and arrows are drawn as world quads over their targets. Nine batch 31 map and scanner natives run as stand-ins; BlackoutMap raises the panel alpha of the dialogue background. The 16 text natives of batch 30 run as stand-ins: tip text, dialogue lines, headings, the character intro and the information box are drawn with the layout of the draw code.

## 3. Verified behaviour

- SetActiveWayPoint chain, sub-objective record update, group map slots and arrows from the code (0x229c98, 0x2be934, 0x2ed8d8, 0x29de50; pinned by `waypoints`, `sub-objectives`, `map-marker`), not compared with oracle pictures: [objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002).
- The waypoint marker position and the tip fade were measured on the oracle: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919), [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).

## 4. Stand-ins

- SCRIPT-S269 SetActiveWayPoint: the entity disable of 0x229c98 (only when *(0x529c18 + 0x24) is the waypoint) is never taken because that global's writer is unknown and its readers use it for characters; the vehicle waypoint global *(0x529c18 + 0x28) is kept (`WaypointSet::vehicleWaypoint`) and read by nothing. needs: SCRIPT-349. evidence: [objective-hud.md §2026-10-19T23:06:37](../../re/objective-hud.md#ts-20261019-230637).
- SCRIPT-S270 Sub-objective update and arrows: the key is always counted into the centroid (0x230ef4 not found); the arrow height uses the player's box for characters, the collision box for props and 0 otherwise, vehicles and class 0x365f7679 get no offset, the use marker test uses the port's marker slots, the bob clock starts at the level start, and the quad is drawn with the particle renderer's blend and depth state. needs: SCRIPT-329, SCRIPT-330. evidence: [objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002).
- SCRIPT-S27 Nine batch 31 natives (minimap wipe, scanner, pan map, map dummy, DisableIcon) only record call counts and last values. needs: SCRIPT-73. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- SCRIPT-S23 The 16 batch 30 natives are stand-ins: tip text, dialogue line and dialogue background are drawn with the layout of the draw code (fitted rest HUD-S8) and the measured five-update tip fade; mission title and summary only store the key; ShowIntroText, ShowCharacterIntro and InformationBox follow the handlers (HUD-S14 for the box); the other five only count calls. needs: SCRIPT-66. evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).

## 5. Open items

- [ ] SCRIPT-66 — ShowTally, ShowBossHearts, ShowRaceStart, ShowPortraitOverVehicle and showinfopointer only count their calls; their screens are not drawn (ShowLevelText, ShowIntroText, ShowCharacterIntro and InformationBox are drawn since 2026-10-14). needs: HUD-22. evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).
- [ ] SCRIPT-67 — SetMissionTitle / SetMissionSummary store the key (0x375f98 / 0x375f9c); no reader found, so the title and summary are not shown (pause screen, map?). needs: HUD-5. evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).
- [ ] SCRIPT-68 — The skip byte 0x37751c of both tip natives (1 = no tip) is not modelled (treated as 0, the value in the oracle). evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).
- [ ] SCRIPT-69 — The speaker object of the dialogue natives (0x17cc58) and the follow-up call 0x1ffaac (player row == id) are not modelled; the speaker is only stored. evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).
- [ ] SCRIPT-70 — The tip alpha follows its writer 0x2de284 (0.2 per update, found); the dialogue fade is from code (5.0 per second in, 0.25 s out); the state machine 0x23cc00 is not transcribed (state times are approximated by dt sums). evidence: [script-text.md §2026-10-14T15:54](../../re/script-text.md#ts-20261014-155429).
- [ ] SCRIPT-73 — Map and scanner natives (EnableMinimapWipe 0x187f28, EnableScanner, OverrideScannable, PanMapToObject, map dummy location, DisableIcon, scanner marker) only record their arguments. needs: HUD-11. blocks: SCRIPT-S27. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- [ ] SCRIPT-349 — The writer of *(0x529c18 + 0x24) (0x230ef4, 24 callers) and the reader of *(0x529c18 + 0x28) (0x2b4dac): decide whether a waypoint can be that entity and what the vehicle waypoint steers. blocks: SCRIPT-S269. evidence: [objective-hud.md §2026-10-19T23:06:37](../../re/objective-hud.md#ts-20261019-230637).
- [ ] SCRIPT-329 — The arrow hide request 0x2b9c28 comes from the use hint 0x1cba8c (kinds 122, 123, 159 when their use query answers 1), which is not ported, so carried objects and carry targets never hide their arrows. blocks: SCRIPT-S270. evidence: [objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002)
- [ ] SCRIPT-330 — Compare a group dot, a group slot with its edge arrow and an objective arrow with oracle pictures (needs an `--rpc` session: mission 1-1 after `1-1_Get_Car`, mission 1-2 at the detective spot). blocks: SCRIPT-S270. evidence: [objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002)
- [ ] SCRIPT-331 — Enemy groups (0x230e60, type 1, called at 0x22b328 and 0x2bdfbc), carry groups (0x22fbe8, type 2, at 0x231c78) and the carry arrow list (0x22fb44) are filled by code that is not ported. evidence: [objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002).
- [ ] SCRIPT-346 — The ShowDialogBG byte and the byte +0x485 make 0x2d9d0c hide the timed object 0x5190bc that it otherwise shows for the nearest of ten list entries in range; the port has neither the object nor the search. uses: ShowDialogBG. evidence: [script-text-bottom-screen.md §2026-10-19T23:15:01](../../re/script-text-bottom-screen.md#ts-20261019-231501).

## 6. Findings summary

- SetActiveWayPoint disable path, map slots, vehicle waypoint, WaypointType: [objective-hud.md §2026-10-19T23:06:37](../../re/objective-hud.md#ts-20261019-230637).
- SetActiveWayPoint chain, sub-objective manager update, group map slots and arrows: [objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002).
- BlackoutMap 0x1b23f4 and the readers of the ShowDialogBG byte: [script-text-bottom-screen.md §2026-10-19T23:15:01](../../re/script-text-bottom-screen.md#ts-20261019-231501).
- Batch 31 handlers (SetActiveWayPoint, SubObjectiveGoToEnable registry, waypoint marker measured on the oracle): [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- Batch 30 handlers (tip text, dialogue lines, dialogue background, mission title; timed objects 0x517ef8 / 0x5189cc, measured fade and layout): [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).

## 7. Code and tests

- Objectives: `ObjectiveState` (`src/runtime/objectives.*`, dispatched by `ScriptRunner::objectiveNative`), its update `src/runtime/sub_objectives.*` (`ScriptRunner::updateObjectiveMarkers`), glue `src/game_objective_markers.cpp`; CTest `objectives`, `objectives-real-data`, `sub-objectives`, `waypoints` (`disableAndSlots`: disable without prior enable, 16 slots, vehicle waypoint), `waypoints-real-data`, `waypoint-marker-shot`, `waypoint-marker-position`.
- Script text: `ScriptTextState` (`src/runtime/script_text.*`, dispatched by `ScriptRunner::textNative`), `drawTipPage` / `drawDialogueBox` / `drawDialogueBackground` (`src/frontend/script_text_pages.*`); CTest `script-text`, `script-text-real-data`, `tip-text-shot`, `tip-text-panel` (`tools/check_tip_shot.py`).
