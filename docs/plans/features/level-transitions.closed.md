# Closed items of level-transitions

Done items of [level-transitions.md](level-transitions.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] TRANS-1 — closed 2026-10-14: cause of the silent bound and the trap chain recorded. Remaining question moved to TRANS-7.
- [x] TRANS-7 — closed 2026-10-14: the `SceneChange` native requests the switch through the scenechange entity (slot +0x2c); `0x303480` runs after the loading-page A press. evidence: [streaming-transitions.md §2026-10-14T11:13](../../re/streaming-transitions.md#ts-20261014-111312).
- [x] TRANS-4 — closed 2026-10-14: trigger dispatch runs in `chase-game` (`ScriptRunner::checkTriggers`, 0x1746c0). evidence: [scripts-runtime.md §2026-10-14T10:06](../../re/scripts-runtime.md#ts-20261014-100640).
- [x] TRANS-2 — closed 2026-10-14: destination semantics and both landing positions pinned (see Verified behaviour). evidence: [streaming-transitions.md §2026-10-14T12:02](../../re/streaming-transitions.md#ts-20261014-120258). The qualification rule moved to TRANS-10.
- [x] TRANS-9 — closed 2026-10-14: the scenechange update 0x1c47e8 is the walk-in path (enabled bound plus player inside, then slot +0x28 or +0x2c); the lobby exit uses it. evidence: [streaming-transitions.md §2026-10-14T12:02](../../re/streaming-transitions.md#ts-20261014-120258).
