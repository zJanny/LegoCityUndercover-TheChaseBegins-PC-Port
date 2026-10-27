# User playtests

What the user saw when playing `chase-game` against their knowledge of the original game: confirmed behaviour and the bugs they found. These are observations, not code findings; each bug gets an item in its feature file.

## Current state

- Mission 1-2 played end to end in `chase-game`: flag poles, trampoline, the scanner's look, the dog at the cage and its release, the punch and the dog footprints in detective mode look and work as in the original; four bugs were found (zipline poles, scanner held model, dog and dog truck in the end cutscene). [§2026-10-19T17:56:41](#ts-20261019-175641)
- Mission 1-3: the ladder to the roof is not fully visible in the port. [§2026-10-19T18:02:53](#ts-20261019-180253)
- Mission 1-3: the ladder now draws but cannot be climbed. [§2026-10-19T20:23:38](#ts-20261019-202338)
- The zipline poles can be walked through in the port. [§2026-10-19T20:15:35](#ts-20261019-201535)
- The handcuffs Chase hangs from on the zipline were not drawn (user report; cause: [player-traversal.md §2026-10-19T20:23:05](player-traversal.md#ts-20261019-202305)); they are drawn now ([hand-items.md §2026-10-27T18:27:37](hand-items.md#ts-20261027-182737)), not yet re-checked by the user.
- Mission 1-3: the key to pick up could not be picked up in the port, so the mission stopped at its first step. [§2026-10-25T13:56:15](#ts-20261025-135615) The port now picks it up; where the user's save state stands (1.71 from the key) A does nothing in the original either, the key's zone ends about 0.6 closer. [carrying.md §2026-10-25T14:37:21](carrying.md#ts-20261025-143721)

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

While making the mission 1-3 save states the user noted that the ladder that is climbed to reach the roof is not fully visible in the port. The save state in front of the crane ladder of mission 1-3 stands in front of it in the original.

Unknowns:
- Which object the ladder is and which part of it is missing in the port.

<a id="ts-20261019-201535"></a>

## 2026-10-19T20:15:35+02:00 — The zipline poles can be walked through (user report)

The user reported (2026-10-19) that in `chase-game` the player walks through the zipline poles (the pole model of the anchor class `props/parkour_zipline`, drawn since [player-traversal.md §2026-10-19T18:16:50](player-traversal.md#ts-20261019-181650)). In the original they are solid, from the user's knowledge of the game.

Unknowns:
- Whether the original's pole collision comes from the pole model's collision section, a class bound or the object collision rules ([collision.md](collision.md)); the port's pole collision was not checked (the same question is an unknown of [player-traversal.md §2026-10-19T18:16:50](player-traversal.md#ts-20261019-181650)).

<a id="ts-20261019-202338"></a>

## 2026-10-19T20:23:38+02:00 — Mission 1-3: the ladder draws but cannot be climbed (user report)

After the merge of round 37 the user reported that the ladder to the roof of mission 1-3 (`1-4_Ladder01`, `props/prop_crane_ladderlarge`, its upper half drawn since [object-kinds.md §2026-10-19T19:25:32](object-kinds.md#ts-20261019-192532)) is now fully drawn, but Chase cannot climb it in `chase-game`. Climbing a ladder is not in the port; the ladders are object kind 56 ([object-kinds.md](object-kinds.md)). The save state in front of the crane ladder of mission 1-3 stands in front of the ladder in the original.

Unknowns:
- The ladder's use, mount, climb and dismount code and clips are not read.

<a id="ts-20261025-135615"></a>

## 2026-10-25T13:56:15+01:00 — Mission 1-3: the key cannot be picked up (user report)

The user reported, in their words: "where i am blocked on mission 1-3. Standing infront of the key you have to pickup, which you cannot do in the port, the key also has a arrow pointing to it floating above".

In the original the mission goes on in this order (the user's description):
- A floating arrow points at the key; Chase picks it up.
- The key is used at a place in front of Chase.
- Chase stands on the trigger of a garage door.
- A parts crate stands in front of Chase; it is carried to a drop-off spot that has a green arrow pointing down.
- Chase builds a generator from the parts and stands on the generator's trigger spot.
- The mission ends.

In the port the key cannot be picked up, so none of the later steps can be reached by playing. The later steps were seen in the original only and were not checked in the port.

Unknowns:
- Which object the key is and why the pickup does nothing (the arrow above it was not checked in the port either).
- What the key opens and which objects take part in each later step.
