# Sound test

`chase-soundtest` plays sounds from your extracted game data. It needs the extracted RomFS in `artifacts/romfs` (see [Extracting game data](extracting-game-data.md)).

Back to the [documentation list](../README.md#more-documentation).

```sh
build/chase-soundtest music/act_bass_break.bwav sounds/lego_heart_collected_01.bwav
build/chase-soundtest --id 1 --id 5
```

- All listed sounds play at the same time.
- The program ends when the longest sound has played. A looping sound counts as 10 seconds. Use `--seconds N` to set the time yourself.
- `--id N` plays a sound effect by its number in the game's sound table, at the volume the table gives. You can repeat `--id` and mix it with file names.
- The sound for each event is known only for some events, for example the character and the coins. See [Sound](game-runtime.md#sound).

Without a sound card, for example on a server, run it with `SDL_AUDIODRIVER=dummy`.
