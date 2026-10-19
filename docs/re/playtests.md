# User playtests

What the user saw when playing `chase-game` against their knowledge of the original game: confirmed behaviour and the bugs they found. These are observations, not code findings; each bug gets an item in its feature file.

## Current state

- Mission 1-2 played end to end in `chase-game`: flag poles, trampoline, the scanner's look, the dog at the cage and its release, the punch and the dog footprints in detective mode look and work as in the original; four bugs were found (zipline poles, scanner held model, dog and dog truck in the end cutscene). [§2026-10-19T17:56:41](#ts-20261019-175641)
- Mission 1-3: the ladder to the roof is not fully visible in the port. [§2026-10-19T18:02:53](#ts-20261019-180253)
- The zipline poles can be walked through in the port. [§2026-10-19T20:15:35](#ts-20261019-201535)
- The handcuffs Chase hangs from on the zipline are not drawn (user report; cause: [player-traversal.md §2026-10-19T20:23:05](player-traversal.md#ts-20261019-202305)).

<a id="ts-20261019-175641"></a>

## 2026-10-19T17:56:41+02:00 — Mission 1-2 playthrough (user, `chase-game`)

The user played mission 1-2 through in `chase-game` (2026-10-19) and compared it with the original from their own knowledge of the game.

**Confirmed as in the original:**
- The flag poles work (swing and jump across).
- The trampoline works.
- The scanner view looks right.
- The dog at the cage and its release work.
- The punch feels correct.
- The dog footprints in detective mode look correct ([abilities-detective.md §2026-10-19T17:50:50](abilities-detective.md#ts-20261019-175050)).

**Bugs:**
- The zipline rope is drawn, but not the poles it hangs from. The anchor class `props/parkour_zipline` has the model `models/ability_parkour_zip_line/ability_parkour_zip_line.bfnmdl` in the archive; why the port does not draw it was not checked.
- Chase holds no model while using the scanner.
- The dog is not visible in the ending cutscene of the mission.
- The dog truck (with the dog inside) sits in the ground in the ending cutscene.

Unknowns:
- The other user-visible checks of rounds 33 and 34 (objective arrows and map group icons in mission 1-1, the screen shake, goons falling apart in lvl_02) were not answered yet.

<a id="ts-20261019-180253"></a>

## 2026-10-19T18:02:53+02:00 — Mission 1-3: the ladder to the roof (user report)

While making the mission 1-3 save states ([oracle-states.md §2026-10-19T18:02:53](oracle-states.md#ts-20261019-180253)) the user noted that the ladder that is climbed to reach the roof is not fully visible in the port. The state `m13-2-ladder` stands in front of it in the original.

Unknowns:
- Which object the ladder is and which part of it is missing in the port.

<a id="ts-20261019-201535"></a>

## 2026-10-19T20:15:35+02:00 — The zipline poles can be walked through (user report)

The user reported (2026-10-19) that in `chase-game` the player walks through the zipline poles (the pole model of the anchor class `props/parkour_zipline`, drawn since [player-traversal.md §2026-10-19T18:16:50](player-traversal.md#ts-20261019-181650)). In the original they are solid, from the user's knowledge of the game.

Unknowns:
- Whether the original's pole collision comes from the pole model's collision section, a class bound or the object collision rules ([collision.md](collision.md)); the port's pole collision was not checked (the same question is an unknown of [player-traversal.md §2026-10-19T18:16:50](player-traversal.md#ts-20261019-181650)).
