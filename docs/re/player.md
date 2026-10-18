# Player entity and controller

Findings about the player character at run time: where it lives, how input becomes intent, and (later) how it moves.

Split by subtopic because of the 40 KB limit:

- [player-entity-input.md](player-entity-input.md): entity pointer and scene node, stick-to-intent, position and character source, head attachment, camera yaw word, appearance rule
- [player-locomotion.md](player-locomotion.md): locomotion measurements, replica and differential test, ground following, derivation of walk/run speeds
- [player-falling-jumping.md](player-falling-jumping.md): falling and landing, jumping and its height curve, landing stun rule, step-off slide, slope slow-down, state table
- [player-attack.md](player-attack.md): the attack on foot (Y tap, combat action states 0xa3 / 0xa4, target search, hit test, hit message)

## Current state

- This file holds no dated entries; the current facts are in the subtopic files above.
- Entity, input and appearance: [player-entity-input.md](player-entity-input.md); movement: [player-locomotion.md](player-locomotion.md); falling and jumping: [player-falling-jumping.md](player-falling-jumping.md); attack on foot: [player-attack.md](player-attack.md).
- Open: see the Current state section of each subtopic file.
