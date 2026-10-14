# Script system: objective and text natives

## 1. Scope

The objective natives (SetActiveWayPoint, SubObjectiveGoToEnable, arrows, map and scanner natives) and the text natives (tip text, dialogue lines, mission title, intro and tally screens) of batches 30 and 31. See [script-system-natives.md](script-system-natives.md) for the other natives and [script-system.md](script-system.md) for the runner.

## 2. Status

IN_PROGRESS, updated 2026-10-14T16:48:13+02:00. The 14 objective natives of batch 31 run as stand-ins: an active waypoint shows its object and a map marker. The 16 text natives of batch 30 run as stand-ins: tip text and dialogue lines are drawn with the layout of the draw code.

## 3. Verified behaviour

- The waypoint marker position and the tip fade were measured on the oracle: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919), [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).

## 4. Stand-ins

- SCRIPT-S26 SetActiveWayPoint shows or hides the waypoint object (disable always applies) and SubObjectiveGoToEnable keeps the registry group without its marker effect. needs: SCRIPT-71, SCRIPT-74. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- SCRIPT-S27 Twelve batch 31 natives (arrows, minimap wipe, scanner, pan map, map dummy, blackout, DisableIcon) only record call counts and last values. needs: SCRIPT-72, SCRIPT-73. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- SCRIPT-S23 The 16 batch 30 natives are stand-ins: tip text, dialogue line and dialogue background are drawn with the layout of the draw code (fitted rest HUD-S8) and the measured five-update tip fade; mission title and summary only store the key; the other nine only count calls. needs: SCRIPT-66. evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).

## 5. Open items

- [ ] SCRIPT-66 — ShowIntroText, ShowLevelText, ShowCharacterIntro, ShowTally, ShowBossHearts, ShowRaceStart, InformationBox, ShowPortraitOverVehicle, showinfopointer only count their calls; their screens are not drawn. needs: HUD-13. evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).
- [ ] SCRIPT-67 — SetMissionTitle / SetMissionSummary store the key (0x375f98 / 0x375f9c); no reader found, so the title and summary are not shown (pause screen, map?). needs: HUD-5. evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).
- [ ] SCRIPT-68 — The skip byte 0x37751c of both tip natives (1 = no tip) is not modelled (treated as 0, the value in the oracle). evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).
- [ ] SCRIPT-69 — The speaker object of the dialogue natives (0x17cc58) and the follow-up call 0x1ffaac (player row == id) are not modelled; the speaker is only stored. evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).
- [ ] SCRIPT-70 — The tip keeps the measured five-update fade (alpha writer not found); the dialogue fade is from code (5.0 per second in, 0.25 s out); the state machine 0x23cc00 is not transcribed. needs: HUD-20. evidence: [script-text.md §2026-10-14T15:54](../../re/script-text.md#ts-20261014-155429).
- [ ] SCRIPT-71 — SubObjectiveGoToEnable keeps its group (registry 0x1fce14 / 0x2a62b8) but the group marker is not drawn: update 0x2be934 (centroid of the targets, effect 0x0e / 0x0d / 0x0f, fade) is not transcribed or compared with the oracle. needs: HUD-16. blocks: SCRIPT-S26. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- [ ] SCRIPT-72 — The arrow natives (SubObjectiveArrowGoToEnable 0x2c57b0 -> 0x2c289c, SubObjectiveArrowEnemyEnable 0x2c87f0 -> 0x230dbc) only record; their arrows (textures SubObjectiveArrow*) are not drawn; mission 1-2 and later use them. blocks: SCRIPT-S27. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- [ ] SCRIPT-73 — Map and scanner natives (EnableMinimapWipe 0x187f28, EnableScanner, OverrideScannable, PanMapToObject, map dummy location, BlackoutMap, DisableIcon, scanner marker) only record their arguments. needs: HUD-11. blocks: SCRIPT-S27. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- [ ] SCRIPT-74 — SetActiveWayPoint 0x26e168 skips a disable when bit 4 of component +0x8e is clear (meaning unknown), and its show / hide chain 0x229c98 / 0x242cf0 is not transcribed. blocks: SCRIPT-S26. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).

## 6. Findings summary

- Batch 31 handlers (SetActiveWayPoint, SubObjectiveGoToEnable registry, waypoint marker measured on the oracle): [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- Batch 30 handlers (tip text, dialogue lines, dialogue background, mission title; timed objects 0x517ef8 / 0x5189cc, measured fade and layout): [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).

## 7. Code and tests

- Objectives: `ObjectiveState` (`src/runtime/objectives.*`, dispatched by `ScriptRunner::objectiveNative`); CTest `objectives`, `objectives-real-data`, `waypoint-marker-shot`, `waypoint-marker-position`.
- Script text: `ScriptTextState` (`src/runtime/script_text.*`, dispatched by `ScriptRunner::textNative`), `drawTipPage` / `drawDialogueBox` / `drawDialogueBackground` (`src/frontend/script_text_pages.*`); CTest `script-text`, `script-text-real-data`, `tip-text-shot`, `tip-text-panel` (`tools/check_tip_shot.py`).
