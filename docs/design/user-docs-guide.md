# User documentation design guide

The rules every user-facing document follows: `README.md`, `CONTRIBUTING.md` and the top-level `docs/*.md` (not `re/`, `design/`, `rules/`, `plans/`). They apply to all plans. The docs reviewer checks every branch that changes these files against them ([docs-review](../../.claude/skills/lead-round/references/docs-review.md)).

<a id="ts-20261112-101747"></a>

## 2026-11-12T10:17:47+01:00 — User documentation design guide

**Approved by the user, 2026-11-12:** guide "Approve"; enforcement "Yes, enforce it"; size limits "Accept 12 KB / 40 KB" (split `commands.md` and `game-runtime.md` by subtopic); badges "Keep four for now" (a build or test badge only once CI exists); wording "Accept both" (25-word sentences, no dates in user docs); troubleshooting "Later, when it grows" (README section only for now); language "English only" (no translation rules); weighting "Lean on small CLI tools" (where scrcpy, nvm, uv, ComfyUI and ollama differ from the majority, follow them).

**Evidence:** a survey of 99 code projects with more than 60 000 stars (state of 2026-11-12; awesome-lists, books, tutorials, interview prep, prompt and skill collections and data lists excluded). Counts are "n of 99", approximate (about 3 either way). Pointer READMEs (60 to 500 words, mostly frameworks) were 52; self-contained READMEs with install and usage were 47. This project is an end-user tool with a hard setup, so it follows the second kind.

### README

1. MUST open with the project name, then one or two sentences on what it is and for whom. A logo or badge row may come first (24 of 99 start with title and pitch).
2. MUST state the current state in the first screen in plain words ("not playable yet"). Only about 11 of 99 do; here it is mandatory.
3. MUST give requirements before the first install command: platform, graphics card, disk, tool versions, the user's own game image (install section 74, requirements about 55).
4. MUST have a numbered "Quick start" of copy-paste blocks; each block is followed by one sentence on what the step does and what the user should see (install block 63).
5. MUST have at most 4 badges, each true and each a committed file in `docs/badges/` (78 have badges, median 4, 21 none, outliers 11 to 37). A build or test badge only once CI exists.
6. MUST NOT have a table of contents; use a one-line anchor row (full TOCs: 17, all in READMEs over about 1000 words).
7. MUST link to the docs index, `CONTRIBUTING.md` and the license (docs link about 95, contributing about 80, license about 58).
8. MUST have a short "If something goes wrong" section for the common failures (21; the norm in end-user tools such as scrcpy, nvm, uv, ComfyUI). A separate page only when the list grows.
9. MUST keep the "not affiliated" notice and the generated progress section. The README is at most 12 KB; technical detail lives in `docs/`, linked, never deleted.
10. MUST NOT show screenshots or other assets of the original game.

### docs/ layout and pages

11. MUST have one entry list of all top-level user pages with one line each (in the README "More documentation" section).
12. MUST name pages by task or topic and keep one topic per page. A page over 40 KB is split by subtopic, with a pointer line in the old page.
13. MUST start every page with a one-sentence purpose line and a link to the page that leads to it.
14. MUST put reference material in tables or lists and walkthroughs in numbered steps.
15. MUST NOT add a changelog: the project has no releases (changelog in the README: about 15).
- **Getting started:** the long form of the README path: same steps, same order, plus details. A step is added to both or neither.
- **Commands:** every option and program once, with a one-line meaning, grouped by program, an example for the non-obvious ones. CTest `doc-links` requires every option to appear in a user doc.
- **Controls:** one table of the default bindings (action, key, mouse, original button), then rebinding. Bottom-screen menus are clicked with the mouse.
- **Status:** "What works" and "Not recovered yet", one short sentence per item, details linked; agrees with the README state line.
- **Contributing:** setup, commands, commit style, CI, how to report problems, where the developer docs are; the legal notice stays.

### Wording

16. MUST use sentences of at most 25 words, one idea each (about 96).
17. MUST address the reader as "you" and give commands in the imperative (about 92).
18. MUST describe the current state only: no dates, "now", "new", "recently" or history.
19. MUST NOT use marketing words, emoji in headings or exclamation marks (emoji: 14).
20. MUST NOT claim more than is true: no "playable", "complete" or "port" without "early" or "work in progress". Every statement is checked against the code.
21. MUST use one name per thing on all pages (`chase-game`, "decrypted `.cci` image", "game data").
22. MUST explain a term on first use per page or link to where it is explained (3DS, `.cci`, romfs, oracle).

### Formatting, links, examples

23. MUST use sentence-case headings, one H1 per page, no skipped levels.
24. MUST mark every command, path, option, key and file name as code; commands in fenced blocks with a language, without a prompt character.
25. MUST NOT mix commands and their output in one block.
26. MUST use relative links inside the repository and absolute links only for external pages; every link resolves; link text names the target, never "here".
27. MUST NOT copy text between pages; the short fact stays where it belongs and the others link.
28. Examples MUST be real: run or verified against the code; no invented option, path or output.
29. Tables have at most 5 columns, a header row, no empty cells and no cell longer than one sentence.

### Project fit

- The honest state line stays in the README, `status.md` and `getting-started.md` until the user says otherwise.
- No original assets or screenshots of the game; a picture's content is described in words.
- The README progress section is generated by `merge_agent.py` and never edited by hand.
- CTest `doc-links` and `tools/user_docs.py` keep passing; the legal notice is never weakened.
- User docs are English only.
