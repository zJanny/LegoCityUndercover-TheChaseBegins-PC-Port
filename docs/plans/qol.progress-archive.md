# Session log archive

Session log lines that left [qol.progress.md](qol.progress.md), oldest first, unchanged. tools/check_doc_links.py --fix appends new lines at the end.

- 2026-10-28T09:21:35+01:00 — Planner: opened the qol plan. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T09:32:53+01:00 — Planner: recorded the user's answers on menu access, content, layout, minimap, aspect and separation. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T09:38:37+01:00 — Planner: recorded the user's answers on the master button, window modes, files, HUD corners, toggles and panel look. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T09:41:06+01:00 — Planner: recorded Custom and hiding decisions, three new open questions. See [qol-bottom-screen](features/qol-bottom-screen.md).
- 2026-10-28T09:49:01+01:00 — Planner: recorded that the master button alone switches the bottom screen; two new open questions. See [qol-bottom-screen](features/qol-bottom-screen.md).
- 2026-10-28T09:50:53+01:00 — Planner: recorded minimap and bottom-screen decisions; one new open question. See [qol-minimap](features/qol-minimap.md).
- 2026-10-28T09:53:50+01:00 — Planner: recorded that the master state is stored; no open user decision left. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T21:33:48+01:00 — Agent 20261028-qol-A (toggle store): built library chase_qol with the master state, feature table and strict qol.cfg, test qol-settings. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T21:34:16+01:00 — Agent 20261028-qol-B (qol-separation guard): added the qol-separation guard test with accepting and rejecting fixtures. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T21:43:23+01:00 — Lead (recorded): the user approved the `config/qol.cfg` key names ("Keep these names"): `master` with Original, Improved, Custom; snake_case feature keys with on or off. See [qol-settings §2026-10-28T21:43:23](../design/qol-settings.md#ts-20261028-214323).
- 2026-10-28T21:55:52+01:00 — Lead (20261028-qol end): merged 20261028-qol-A (toggle store, library chase_qol and config/qol.cfg), 20261028-qol-B (qol-separation guard) and the recorder branch (approved qol.cfg key names) through tools/merge_agent.py with all tests passing; C (settings menu model) dropped by the user until A is merged; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-menu](features/qol-settings-menu.md)
