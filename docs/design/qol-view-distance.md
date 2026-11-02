# QoL: view distance

The user's approved design of the View distance slider: how far the game draws and loads with the slider. Plan: [qol.md](../plans/qol.md); rules: [qol.md](../rules/qol.md); feature file: [qol-view-distance.md](../plans/features/qol-view-distance.md).

<a id="ts-20261102-184307"></a>

## 2026-11-02T18:43:07+01:00 — View distance slider

Asked of the user by the lead on 2026-11-02 (round 20261102-qolmenu); the quoted labels are the user's selections, recorded by 20261102-qolmenu-E.

- Tab: "Display tab". The slider is a row of the Display tab of the PC settings menu, and it is the feature's only switch.
- Range: "1.0x to 4.0x", raised on 2026-11-02 to x1.0 to x20.0 (see the answers below). x1.0 is the original: far plane 260, street fog end 260.
- Fog: "Start and end scale together": the fog start and end of every level and room are multiplied by the same factor.
- What it extends: "Everything is loaded further": the far plane, the fog, the room loading and the pedestrian and traffic distances scale with it.
- Presets: "Always live": the slider works in every preset except Original, which always keeps x1.0; no preset changes its value.
- Steps and label: "0.25x steps, shows "x1.0"": 29 positions from x1.0 to x20.0 (see the answers below).
- Scope: "All cameras": also the scanner view, cutscenes and the free camera; independent of the widened view's extra room loading ([qol-screen-layout.md §2026-11-02T15:42:07](qol-screen-layout.md#ts-20261102-154207)).
- Reset: "Back to x1.0": "Reset to defaults" on the Display tab also sets the slider to x1.0.

Answers of the user to the worker's questions (2026-11-02, relayed by the lead):
- Labels and file (approved): the labels read x1.0, x1.25, x1.5 … x4.0 in quarter steps, then x5.0 … x20.0; `config/qol.cfg` stores `view_distance <factor>` (for example `view_distance 1.25`), parsed strictly: a value that is not one of the 29 steps (such as `1.1`, `4.25`, `20.25` or `1`) stops with an error naming the line. The line follows the rule of the other keys for a default value: like `resolution` at the desktop size, there is no `view_distance` line while the value is x1.0.
- The row in Original (approved, "Greyed out at x1.0"): while the preset is Original the row shows x1.0, is greyed out and skipped by Up and Down; the stored value stays and returns in Improved or Custom.

Maximum raised (2026-11-02, mid-round 20261102-qolmore): the user said "increase the max view distance to 20x. Keep all the rules about how we established rule distance increasing works". Answers (2026-11-02): maximum "x20.0 maximum"; steps "Coarser above x4": 0.25x steps from x1.0 to x4.0, then 1.0x steps from x4.0 to x20.0, 13 + 16 = 29 positions (28 steps); `config/qol.cfg` accepts exactly those values (`1.0` to `4.0` in quarters, `5.0` to `20.0` whole) and rejects everything else with an error naming the line. All other rules above stay.

Room and traffic loading (2026-11-02, relayed by the lead): scope "Next round", they are not part of the x20 slider package; room cap "No cap, measure": when rooms are scaled, every room in range is loaded and the worker measures memory and frame time at x20 and reports (QOLVD-5, QOLVD-6, QOLFS-20).

Open (not decided by the user):
- The help text of the row: none is shown until the user approves a wording (QOLVD-4 in the feature file).
- The room loading rule itself: the port loads rooms by the room graph (the current room, its neighbours and up to three preloads), not by a distance; the user wants every room in range loaded with no cap (above), the rule that picks "in range" is the next round's work (QOLVD-5).
