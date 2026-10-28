# Super bricks: the super build-it prompt placeholders

Oracle pictures of the two brick placeholders of the super build-it prompt; the rest of the topic is in [super-bricks.md](super-bricks.md).

## Current state

- The prompt's two placeholders show as two small turning blue bricks in the oracle; the picture does not separate the sprite from the model icon (HUD-86). [§2026-10-28T23:57:17+01:00](#ts-20261028-235717)

<a id="ts-20261028-235717"></a>

## 2026-10-28T23:57:17+01:00 — Oracle pictures of the super build-it prompt placeholders (recorded from a save state by 20261028-uiport-servant, request 20261028-uiport-E-1, 2026-10-28; worker 20261028-uiport-E)

On a helipad of mission 1-3 with 296 and with 302 super bricks the prompt shows "Helipad", "300" and two small blue bricks left and right of the cost, at the same height. Four pictures 0.3 s apart in each state show the two bricks in changing phases (flat, edge-on, diamond), both bricks in the same phase, in both states. The counter 0x593430 reads 296 / 302; the fader table 0x519830..0x51986f is identical in both states and only the spin word 0x375890 varies between loads.

The picture does not decide between the 16-frame sprite of `Pickups_superbrick` and the spinning model icon of the faders: both are blue bricks that turn, and no rendered difference was separated. Nothing was drawn in the port.

Unknowns:
- Sprite, model icon or both (HUD-86): a shot with one of the two draws suppressed, or a per-update series compared with the sprite frame `(updates >> 1) & 15` and the spin 0x375890, would tell.
