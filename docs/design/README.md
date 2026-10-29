# Design decisions

Specs the user approved for the plans that have no original to check against (Windows port, quality-of-life features), one file per topic. Each file holds only the current version of each decision: one dated entry with an anchor per decision, and a split above 40 KB like [docs/re](../re/README.md). When the user changes a decision, its entry is edited in place and the old version is deleted (git keeps it); there are no `Corrected by` lines, no `Current state` section and no history files (CLAUDE.md "Documentation rules"). An entry records what the user approved, in their words and with the date, and what is still open; never a choice an agent made alone. Read this index, then only the entry a feature file links.

| File | Contents |
|---|---|
| [qol-settings.md](qol-settings.md) | `qol`: settings menu, master toggle (Original / Custom / Improved), per-feature toggles, `config/qol.cfg`, the user's two rules |
| [qol-settings-drawing.md](qol-settings-drawing.md) | `qol`: how the settings menu is drawn (original art and font, scaling, text, navigation), Reset all source |
| [qol-screen-layout.md](qol-screen-layout.md) | `qol`: true full-screen view, corner-anchored HUD, minimap and panels for the bottom screen |

No topics yet for the `windows-port` plan: it is gated ([plans/README.md](../plans/README.md)).
