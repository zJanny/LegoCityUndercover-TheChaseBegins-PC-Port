# Closed items of qol-settings-drawing

Done items of [qol-settings-drawing.md](qol-settings-drawing.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] QOLSET-44 — The English "Yes" and "No" of the Reset all question become the gametext words (the model has no gametext); the notice and question builders are joined in QOLSET-40. evidence: [ui-layouts.md §2026-10-29T15:54:31](../../re/ui-layouts.md#ts-20261029-155431).
- [x] QOLSET-40 — Join the list form builders of `src/qol/menu_list_page.hpp` (`listPageScene`, `listHitAreas`) into `ListForm` in place of `standInListForm`, and draw the notice and Reset all question scenes (`noticeScene`, `questionScene`) over the page while the Controls tab waits or asks. blocks: QOLSET-33, QOLSET-43, QOLSET-44. Done (20261029-qolhook-A): `controlsListForm` joins the builders, `ListForm::overlay` draws the notice and question over the page with the `page_alerttext` layout, the question takes the gametext Yes/No of `MenuPageSetup`. evidence: [qol-settings-drawing.md §2026-10-29T12:11:55](../../design/qol-settings-drawing.md#ts-20261029-121155).
