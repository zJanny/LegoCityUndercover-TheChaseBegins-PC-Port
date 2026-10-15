# Closed items of script-system-natives-animation

Done items of [script-system-natives-animation.md](script-system-natives-animation.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] SCRIPT-112 — closed 2026-10-15: the animation natives play through the slot player 0x23afc0 for level characters (0x295a8c rules, the loaded check, `AnimationOffset`, the waits); the rest is split into SCRIPT-270 to SCRIPT-274. evidence: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).
- [x] SCRIPT-271 — Closed 2026-10-15: PlayAnimation, StopAnim and WaitForCurrentAnim drive the skinned placed props (garage shutters, dog cage, dumpster, crane, lawnmowers, gardeners, dog, crow) through the slot player and `Scene::pose`; the clips are skeletal, so the per-node transforms were not needed. evidence: [script-natives-animation.md §2026-10-15T20:04](../../re/script-natives-animation.md#ts-20261015-200440).
