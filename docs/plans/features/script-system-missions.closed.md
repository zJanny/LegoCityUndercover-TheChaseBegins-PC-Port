# Closed items of script-system-missions

Done items of [script-system-missions.md](script-system-missions.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] SCRIPT-110 — The waypoint class (init 0x17aa34, update 0x17acbc) is ported in `src/runtime/waypoints.*`: TriggerBound cylinder test, states 0 to 3, thread start 14 updates after the hit, self switch-off; oracle d3 / d2 match. blocks: SCRIPT-1. evidence: [scripts-missions.md §2026-10-14T22:48](../../re/scripts-missions.md#ts-20261014-224825).
- [x] SCRIPT-160 — Collision boxes and doors follow EnableObject / EnableCollision calls that run after the startup (mail box collision of `1-1_MissionStart`, enabled objects): the state is applied every update. evidence: [collision.md §2026-10-15T07:58](../../re/collision.md#ts-20261015-075843).
- [x] SCRIPT-170 — closed 2026-10-15: carry-fired events now follow 0x244f40 (enable the object, message 0xff, ObjectTriggered of the referenced entity): `TriggerPickup` at the attachment, `TriggerObject` of the target at its state change, `TriggerOnRespawn` at the respawn (`ScriptRunner::triggerObjectEnabling`, `applyCarryActions`); the order matches the oracle. Remaining gaps: SCRIPT-190. blocks: SCRIPT-111. evidence: [carrying.md §2026-10-15T11:11](../../re/carrying.md#ts-20261015-111115).
