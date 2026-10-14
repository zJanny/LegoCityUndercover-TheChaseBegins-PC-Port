# Closed items of hud-ui

Done items of [hud-ui.md](hud-ui.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] HUD-1 — Draw the HUD (hearts, stud counter) from the `.bxaml` layouts. Done 2026-10-13. blocks: STUD-2 (done).
- [x] HUD-2 — Studs flying to the counter. Done 2026-10-13 (STUD-S4 removed).
- [x] HUD-4 — The stud counter's display and per-level stud total: display recovered ([hud.md](../../re/hud.md)); the per-level total (0x593420..) is STUD-6.
- [x] HUD-14 — Closed 2026-10-14: tip and dialogue layout from the draw code, badge at natural size, dialogue fade by the code; fitted rest is HUD-S8. evidence: [script-text.md §2026-10-14T15:54](../../re/script-text.md#ts-20261014-155429).
- [x] HUD-13 — Closed 2026-10-14 for the level heading box (ShowLevelText) and the dialogue portrait frame; the other text screens and the background fade are HUD-22. evidence: [script-text.md §2026-10-14T16:51](../../re/script-text.md#ts-20261014-165143).
- [x] HUD-19 — Closed 2026-10-14: the formula reproduces the oracle once the measure call's offset 0 is used; portrait and hexagon frame drawn. evidence: [script-text.md §2026-10-14T16:51](../../re/script-text.md#ts-20261014-165143).
- [x] HUD-20 — Closed 2026-10-14: the tip alpha writer is 0x2de284 (0.2 per update); the port holds 1.0 through the fade-out state. evidence: [script-text.md §2026-10-14T16:51](../../re/script-text.md#ts-20261014-165143).
- [x] HUD-23 — Closed 2026-10-14: ShowLevelText is the last statement of `startup`, nothing delays it (HUD-S13 removed); the remaining parts are HUD-24. evidence: [script-text.md §2026-10-14T18:57](../../re/script-text.md#ts-20261014-185739).
