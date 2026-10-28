# Plan: Quality-of-life features

## 1. Summary
The plan delivers optional quality-of-life features for the PC port: a true full-screen view (the top screen fills the window), the content of the bottom screen moved into a minimap, mouse-clicked panels and PC menus, and a normal settings menu that switches every QoL feature on and off. The faithful game stays the default and priority one. The plan does not recover original behaviour (that is the `linux-port` plan) and does not add Windows work.

## 2. Gate
Opened by the user on 2026-10-28T09:21:35+01:00: "I am opening the QOL of Life Gate." This is earlier than the old condition (after the game is finished for PC); the user's words decide.

## 3. Requirements
- R1: Every QoL feature is switched in the settings menu; there is no other player-facing switch; accept: a test per feature drives the toggle in the menu and checks the feature on and off.
- R2: The faithful version stays priority one: with all QoL off the game and the port's tests are unchanged; accept: the port's tests pass unchanged and no QoL code runs on the default path except its toggle check.
- R3: Everything is tested first in original mode (all QoL off), then with the features enabled; accept: each feature's tests run in both modes.
- R4: The settings menu holds a three-state master toggle (Original, Custom, Improved), one toggle per feature, window mode and resolution, and key bindings; opens from the pause menu Options entry and the title screen, with no extra key; choices saved in `config/qol.cfg`, never in the game save; off by default; accept: tests of the store, the derived master state and strict file parsing.
- R5: True full screen: the 3D view fills the window, widened with the same vertical field of view, HUD anchored to the corners, no pop-in at the edges; accept: pictures and tests pinned to the spec entry.
- R6: The bottom screen is hidden when its features are on: the map is a corner minimap, the characters button, character swap and fast-travel map are full-window panels clicked with the mouse, pause and Options are PC menus; accept: per feature as R3.

## 4. Scope and non-goals
In scope: R1 to R6, in the order S1 to S4. Out of scope: Windows; any gameplay change (difficulty, balance, new content); volumes in the settings menu (not selected by the user); original behaviour that is not recovered yet (stays a port item, [rules](../rules/qol.md)). The input mapping stays the port's job.

## 5. Design decisions
- Settings menu, master toggle, rules: [docs/design/qol-settings.md §2026-10-28T09:21:31](../design/qol-settings.md#ts-20261028-092131)
- Full screen, HUD, bottom-screen content: [docs/design/qol-screen-layout.md §2026-10-28T09:21:33](../design/qol-screen-layout.md#ts-20261028-092133)

## 6. Steps
- S1 (R1, R2, R4): toggle store, master state and settings menu ([qol-settings-menu](features/qol-settings-menu.md)); needs: none; verify: store and menu tests, original-mode tests unchanged. Nothing else starts before S1.
- S2 (R5): full-screen view and corner HUD ([qol-fullscreen](features/qol-fullscreen.md)); needs: S1; verify: off and on tests, picture check of the edges.
- S3 (R6): minimap ([qol-minimap](features/qol-minimap.md)); needs: S2; verify: off and on tests.
- S4 (R6): panels and PC menus for the rest of the bottom screen ([qol-bottom-screen](features/qol-bottom-screen.md)); needs: S2; verify: off and on tests.

## 7. Verification
Linux x86_64 first. Build and test commands: CLAUDE.md "Commands". A feature counts as verified when its tests pass in original mode (feature off, the port's tests unchanged) and with the feature on, and the review confirmed the rule file's checks. Steps that need the port's missing pieces (UI-1, LEVOBJ-120) wait for them.

## 8. Rules
Rule file: [docs/rules/qol.md](../rules/qol.md). Features: under `## Quality of life (`qol` plan)` in [features/README.md](features/README.md).
