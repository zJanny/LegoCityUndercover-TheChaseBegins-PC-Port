# Level transitions

How the original switches between the police station and the street, and what the port still lacks for it.

<a id="ts-20261014-133958"></a>

## 2026-10-14T13:39:58+02:00 — Details moved from feature items TRANS-5, TRANS-8

Moved from the feature files so that the items stay short (CLAUDE.md, "Documentation rules"). The text below is the former wording of each item.

- **TRANS-5** (level-transitions.md): Natives needed by the transition scripts: `WarningDialogBox`, `PositionObject` (player), `SceneChange` and `FadeScreen` (stand-in) run in `chase-game` (SCRIPT-7); still without handler: `PositionPlayer`, `SnapCamera`, `AIMove`, `StartDCam`, `WaitForDCam`, `AIEnable`, `DCamActive`, `CharSetAllowVehicleEntry`, `RelockChasePlainClothes`. evidence: [scripts-runtime.md §2026-10-14T10:54](scripts-runtime.md#ts-20261014-105432). needs: SCRIPT-1.
- **TRANS-8** (level-transitions.md): Loading page code: which input read gives the A edge, whether A counts before the load finishes, what the badge and line do while loading, the text key and the source of the top key art. The page object is vtable 0x361fb4 (enter 0x2e1324), the A wait is in the generic menu widget (0x2a8540 state table). evidence: [streaming-transitions.md §2026-10-14T12:02](streaming-transitions.md#ts-20261014-120258). blocks: TRANS-S3, TRANS-S4. needs: FE-3.
