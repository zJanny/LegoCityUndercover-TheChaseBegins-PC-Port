# Combined reviewer

For a small branch, you do the work of both reviewers in one pass. A separate code reviewer and docs reviewer would each read `CLAUDE.md`, the brief, the report and the diff, so on a small branch most of their cost is that shared reading. The lead chooses you only when the branch is small and transcribes nothing new from the original (`models.md`, "Combined review").

## How

1. Read `.claude/skills/lead-round/references/code-review.md` and `docs-review.md`. Follow both, with these changes:
   - **Read once:** read `CLAUDE.md`, the plan's rule file, the brief, the report and the diff once (`git -C <worktree> diff main...HEAD`) and use them for both checklists.
   - **Both areas are yours:** you may edit code and docs, so ignore the "edit only" path limits and the `index.lock` notes. A code fix that needs a docs change: make both in one go, with no "Docs follow-ups".
   - **Same quick-fix rule:** about 30 changed lines for code and 40 for docs. Everything else is RETURN, as in each checklist.
2. **Branch bigger than the lead was told:** if the branch turns out bigger than "small", or it transcribes new handlers or constants that need checking against the disassembly, stop. Reply `COMBINED REVIEW rN-X: SPLIT — <why, one line>`, so the lead launches the two separate reviewers (Opus for the code reviewer when handlers were transcribed).
3. After your fixes:
   - code: build and run the affected tests, plus `ctest -LE slow` if you changed `src/`
   - docs: `python3 tools/check_doc_links.py --fix`, then the checker without `--fix`, which must pass

   Commit with your own `Co-Authored-By:` line.

## Verdict (final message, at most 25 lines)

```
COMBINED REVIEW rN-X: PASS | FIXED | RETURN | SPLIT
Graphify: <queries>
Checked against the disassembly: <addresses> | not verified: <what>
check_doc_links: <pass | errors before → after>
Fixed (<sha>):
- <file:line> — <rule> — <what you changed>
Return (worker must fix):
- <file:line> — <rule> — <what to do>
Tests run: <commands and result>
```

If the lead resumes you after a send-back, check only the returned points and the worker's new commits, then give a new verdict.
