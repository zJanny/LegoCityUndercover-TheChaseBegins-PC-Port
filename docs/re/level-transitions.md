# Level transitions

How the original switches between the police station and the street, and what the port still lacks for it.

## Current state

- The only entry moves the open details of TRANS-5 (natives the transition scripts need) and TRANS-8 (loading page code) out of the feature files. [§2026-10-14T13:39:58](#ts-20261014-133958)
- `WarningDialogBox`, `PositionObject` (player), `SceneChange` and `FadeScreen` (stand-in) run in `chase-game`; nine natives (`PositionPlayer`, `SnapCamera`, `AIMove`, `StartDCam`, `WaitForDCam`, `AIEnable`, `DCamActive`, `CharSetAllowVehicleEntry`, `RelockChasePlainClothes`) still have no handler. [§2026-10-14T13:39:58](#ts-20261014-133958)
- The loading page object is vtable 0x361fb4 (enter 0x2e1324); the A wait is in the generic menu widget (state table 0x2a8540). [§2026-10-14T13:39:58](#ts-20261014-133958)
- Corrected by the entry below: of the nine natives, only `CharSetAllowVehicleEntry` and `RelockChasePlainClothes` had no handler, and they have one now (stand-ins); the 31 transition scripts of the shipped levels call only handled natives except `SetTrafficMaxVehicles`, `PlayCredits`, `RoomStreamingWait`, `PlayScriptOnStartup` and `StudMagnetScene`. [§2026-10-19T15:54:20](#ts-20261019-155420)
- Open: which input read gives the A edge on the loading page, what the badge and line do while loading, the text key and the top key art source.

<a id="ts-20261014-133958"></a>

## 2026-10-14T13:39:58+02:00 — Details moved from feature items TRANS-5, TRANS-8

Corrected by [level-transitions.md §2026-10-19T15:54:20](#ts-20261019-155420): of the nine natives listed for TRANS-5 only two lacked a handler, and they have one now.

Moved from the feature files so that the items stay short (CLAUDE.md, "Documentation rules"). The text below is the former wording of each item.

- **TRANS-5** (level-transitions.md): Natives needed by the transition scripts: `WarningDialogBox`, `PositionObject` (player), `SceneChange` and `FadeScreen` (stand-in) run in `chase-game` (SCRIPT-7); still without handler: `PositionPlayer`, `SnapCamera`, `AIMove`, `StartDCam`, `WaitForDCam`, `AIEnable`, `DCamActive`, `CharSetAllowVehicleEntry`, `RelockChasePlainClothes`. evidence: [scripts-runtime.md §2026-10-14T10:54](scripts-runtime.md#ts-20261014-105432). needs: SCRIPT-1.
- **TRANS-8** (level-transitions.md): Loading page code: which input read gives the A edge, whether A counts before the load finishes, what the badge and line do while loading, the text key and the source of the top key art. The page object is vtable 0x361fb4 (enter 0x2e1324), the A wait is in the generic menu widget (0x2a8540 state table). evidence: [streaming-transitions.md §2026-10-14T12:02](streaming-transitions.md#ts-20261014-120258). blocks: TRANS-S3, TRANS-S4. needs: FE-3.

<a id="ts-20261019-155420"></a>

## 2026-10-19T15:54:20+02:00 — The natives of the transition scripts

[Corrects level-transitions.md §2026-10-14T13:39:58](#ts-20261014-133958): the list of nine natives without a handler is out of date.

Method: `parseLevel` of every `*.blvl`, all scripts that call `SceneChange`, `CharSetAllowVehicleEntry` or `RelockChasePlainClothes` (31 scripts in lvl_01 to lvl_07), each call looked up with `scriptFunctionSupport`. No emulator run.

- Calls without a handler in those scripts: `SetTrafficMaxVehicles` (lvl_01 script d5a16ff1), `PlayCredits` and `PlayScriptOnStartup` (lvl_07 b26fa928), `RoomStreamingWait` and `StudMagnetScene` (lvl_07 6645da8e).
- Of the nine natives, `PositionPlayer`, `SnapCamera`, `AIMove`, `AIEnable`, `DCamActive` run as stand-ins, `StartDCam` and `WaitForDCam` run, `CharSetAllowVehicleEntry` and `RelockChasePlainClothes` record their effect since batch 35 ([script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302)).
- The other stand-ins these scripts use most: `AIEnable` 27 calls, `FadeScreen` 25, `PositionPlayer` 14, `AttachObject` 13, `SnapCamera` 8.

Unknowns: what the five unhandled natives do in the original (batches SCRIPT-29, SCRIPT-36, SCRIPT-38, SCRIPT-39).
