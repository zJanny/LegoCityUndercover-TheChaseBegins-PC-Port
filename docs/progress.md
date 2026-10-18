# How progress is measured

The README shows a small progress table. It is made by one command. Each number compares what the port does with a total that comes from the original game, not from our own plans.

There is no single percentage for "how far is the port". The game has no list of features to count. So the table has separate measures. Each has its own meaning.

## The measures

| Measure | What is counted | The total comes from |
|---|---|---|
| Script commands | Different commands (such as `EnableObject` or `Wait`) that the level scripts call, and how many of them the port runs. Also counted by how often the scripts call them. | The 30 level files in the game. |
| Object types | Different kinds of objects placed in the levels, and how many have rules the port recovered from the original (collision, doors, coins). Also counted by number of placed objects. | The same 30 level files. |
| Game files read | Files in the game archive that the port can decode, by count and by size. "Can decode" means a decoder exists. It does not mean every detail of the file is used. | The archive index of the game. |
| Open work | Open items and stand-ins (temporary guesses) in the feature notes in `docs/plans/features`. | None. Watch the trend, not a percentage. |
| Checked against the original | Number of automatic tests, and number of recordings of the original game (taken in the emulator) that tests compare with. | None. |
| Game code documented | Functions of the game program that the notes in `docs/re` or the source code point to by address. | An estimate of the number of functions in the game program. It is about 6 % lower than the count of a disassembler (8086). |

Some script commands run with a stand-in result (for example "is this level unlocked" always answers no). They are not counted as done. The table shows how many there are.

The 30 level files include the 8 main levels and test levels that the game ships. `--json` and the text output also show the numbers for the 8 main levels only.

## Trend graphic

The picture under the README table shows four of the measures over time: script commands, object types, open work and game code documented. Each point is one earlier version of the README, read from the git history of `README.md`. Its date and commit come from the "Updated" line of that version. Versions without a readable table are left out. The graphic is `docs/progress-trend.svg`, and `--update-readme` writes it.

## Run it

```sh
python3 tools/progress_report.py
```

It needs the built library (`cmake --build build`) and the extracted game data (see [Extracting game data](extracting-game-data.md)). It takes a few seconds. Parts that need missing data are skipped and named.

- `--json PATH` writes all numbers, with a breakdown per command, object type, file type and feature, the time and the git commit.
- `--update-readme README.md` rewrites the text between `<!-- progress:start -->` and `<!-- progress:end -->` in the README.
- `--check README.md` fails if the README table no longer matches the project. The test `progress-readme` runs it.

After a change that moves a number, run `--update-readme` and commit the README with the change.

## Priority ranking

`docs/plans/priorities.md` ranks the open items and stand-ins of the feature files by how much of the game they touch: script calls and placed objects in the 8 main levels, and how many other items they unblock. `python3 tools/priority_report.py --update docs/plans/priorities.md` writes it from the feature files and the level census; `tools/merge_agent.py` regenerates it at every merge and the test `priorities-current` checks it. The file explains its columns and limits. Reach is a hint for choosing work, not an importance score.
