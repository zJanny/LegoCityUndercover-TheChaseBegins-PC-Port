# Sound test

`chase-soundtest` plays sounds from your extracted game data. It needs the extracted RomFS in `artifacts/romfs` (see [Extracting game data](extracting-game-data.md)).

```sh
build/chase-soundtest music/act_bass_break.bwav sounds/lego_heart_collected_01.bwav
build/chase-soundtest --id 1 --id 5
```

- All listed sounds play at the same time.
- Looping sounds play for 10 seconds. Use `--seconds N` to change that.
- `--id N` plays a sound effect by its number in the game's sound table, at the volume the table gives. You can repeat `--id` and mix it with file names.
- Which sound the game plays for which event is not known yet.

Without a sound card, for example on a server, run it with `SDL_AUDIODRIVER=dummy`.
