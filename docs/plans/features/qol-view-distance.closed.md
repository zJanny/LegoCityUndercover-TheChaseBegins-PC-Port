# Closed items of qol-view-distance

Done items of [qol-view-distance.md](qol-view-distance.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] QOLVD-1 — The slider row of the Display tab, the `view_distance` key of `config/qol.cfg` (strict, no line at x1.0) and the factor (Original always x1.0). Done 2026-11-02 by 20261102-qolmenu-E. blocks: QOLVD-2, QOLVD-3. evidence: [qol-view-distance.md §2026-11-02T18:43:07](../../design/qol-view-distance.md#ts-20261102-184307).
- [x] QOLVD-2 — The far plane of every camera (follow, cutscene, free camera, also in the scanner view) and the fog start, end and fog table scale with the factor. Done 2026-11-02 by 20261102-qolmenu-E. needs: QOLVD-1. evidence: [qol-view-distance.md §2026-11-02T18:43:07](../../design/qol-view-distance.md#ts-20261102-184307).
- [x] QOLVD-3 — The pedestrian distances scale: the fade-in and fade-out distances of the distance fade table (spawn window, node fade and draw test), not the radius tiers, which are sizes. Done 2026-11-02 by 20261102-qolmenu-E. needs: QOLVD-1. evidence: [qol-view-distance.md §2026-11-02T18:43:07](../../design/qol-view-distance.md#ts-20261102-184307).
