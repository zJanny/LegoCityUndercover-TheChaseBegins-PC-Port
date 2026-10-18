# Closed items of level-objects-carry

Done items of [level-objects-carry.md](level-objects-carry.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] LEVOBJ-73 — (User report 2026-10-16) Carrying the donut box looked wrong (no animations, box floating). Done: turn, pickup / carry / put-down clips, hand attachment, carrying step; the rest is LEVOBJ-S30..S32, S72, S73. evidence: [carrying-animation.md §2026-10-16T09:40](../../re/carrying-animation.md#ts-20261016-094004)
- [x] LEVOBJ-74 — The end of the place and drop states and the track that sends the state event: both end when the put-down clip has played once (update 27 at dt 0.033) and release at the rise of the track 0x98d31b7e (update 8), which sends the player message 39 kind 0 (state event type 0); ported in `carryTiming`. evidence: [carrying-animation.md §2026-10-18T10:02:21](../../re/carrying-animation.md#ts-20261018-100221).
