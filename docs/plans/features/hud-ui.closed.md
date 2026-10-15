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
- [x] HUD-21 — Closed 2026-10-14: pitch 17 = line height 23 - 6, line top = centre - lines * pitch / 2, `_16` icon textures at their own size (HUD-S8, HUD-S12 removed). evidence: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500).
- [x] HUD-24 — Closed 2026-10-14: the one pixel per side was the squared box alpha, the parameter 10 / 7 is not read by 0x254d90. evidence: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500).
- [x] HUD-26 — InformationBox (box, OK row, A icon, no B) and ShowIntroText compared with the oracle states `info-barrier` / `mission-1-2-start`; replaces HUD-S14. evidence: [script-text.md §2026-10-14T20:46](../../re/script-text.md#ts-20261014-204600).
- [x] HUD-31 — closed 2026-10-14: the port's picture differed only by test set-up (`--call-script` at update 0 freezes the HUD fade-in; the pending mission 1-1 tip owns the bottom screen); evidence: [script-text.md §2026-10-14T21:41](../../re/script-text.md#ts-20261014-214151).
- [x] HUD-17 — Closed 2026-10-15: the badge was missing because it is a child object of the class file (`dummymeshbouncer`), not part of the model; it is instantiated and turns and hops by the class code (HUD-S11 removed). evidence: [objective-hud.md §2026-10-15T18:31](../../re/objective-hud.md#ts-20261015-183130).
- [x] HUD-16 — Closed 2026-10-15: waypoint marker from the original's drawing code (placement, edge ellipse and arrow, halo, grow, rings); HUD-S5 and HUD-S10 removed; the other icon types are HUD-60. evidence: [objective-hud.md §2026-10-15T19:56](../../re/objective-hud.md#ts-20261015-195600).
