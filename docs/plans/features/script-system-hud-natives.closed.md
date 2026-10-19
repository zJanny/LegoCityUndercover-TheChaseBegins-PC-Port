# Closed items of script-system-hud-natives

Done items of [script-system-hud-natives.md](script-system-hud-natives.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] SCRIPT-140 — Closed 2026-10-14: the same gap as SCRIPT-110 (waypoint class update 0x17acbc starts the `TriggerScript` script), tracked there. evidence: [objective-hud.md §2026-10-14T21:23](../../re/objective-hud.md#ts-20261014-212357).
- [x] SCRIPT-71 — Closed 2026-10-19: the record update 0x2be934 is transcribed; its marker is a map slot of icon 14 / 13 / 15 at the centroid, drawn with the target icons (oracle comparison: SCRIPT-330). evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- [x] SCRIPT-72 — Closed 2026-10-19: the arrow lists, their update 0x2ed8d8 and the world quads of 0x29de50 / 0x1589fc are ported (the arrows have no screen-edge rule; oracle comparison: SCRIPT-330). evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- [x] SCRIPT-74 — Closed 2026-10-19: bit 4 is AutoTrigger == 0; the chain 0x229c98 (map slot, disable fade over FadeScaleInTime, badge hidden) is ported (rest SCRIPT-S269). evidence: [objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002)
- [x] SCRIPT-S26 — removed 2026-10-19: SetActiveWayPoint follows 0x229c98 and the group marker is drawn (SCRIPT-71, SCRIPT-74). evidence: [objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002)
- [x] SCRIPT-67 — SetMissionTitle / SetMissionSummary: the reader is the ticker of the pause top screen (0x29ec00), ported. evidence: [script-text.md §2026-10-20T00:18:56](../../re/script-text.md#ts-20261020-001856).
