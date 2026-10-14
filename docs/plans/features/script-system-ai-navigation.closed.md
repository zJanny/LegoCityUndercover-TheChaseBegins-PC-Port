# Closed items of script-system-ai-navigation

Done items of [script-system-ai-navigation.md](script-system-ai-navigation.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] SCRIPT-90 — Closed 2026-10-14: the finder 0x204284 (0x1c2ccc, 0x1d0700, 0x294768, 0x208338, 0x1f9ea0, 0x1fc6a4, 0x27d90c, 0x215e08 and the cell search), the in-grid follower 0x27ddb4 and the arrival of a plain AIMove (event at 0x2c630c) are read and ported with tests; the rest is SCRIPT-93, SCRIPT-94 and SCRIPT-95. evidence: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).
- [x] SCRIPT-91 — Closed 2026-10-14: state 11 (0x2c3bd8) is the AISuperJump state requested by 0x213300; its arrival (MinDistance, y test with bit 6 of `+0x314`) and heading are ported as `stepGoToTarget`; wiring, the follow-up lookup and class values moved to SCRIPT-96. evidence: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).
