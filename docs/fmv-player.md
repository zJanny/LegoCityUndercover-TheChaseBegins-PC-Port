# Video player

`chase-fmv` plays the game's cutscene videos, with subtitles. It needs the extracted RomFS in `artifacts/romfs` (see [Extracting game data](extracting-game-data.md)) and FFmpeg (see [Building the tools](building.md)).

The videos are in `artifacts/romfs/romfs/cutscenes/` and end in `.moflex`.

```sh
build/chase-fmv artifacts/romfs/romfs/cutscenes/01_gameintro.moflex --lang us
```

- Press Escape or close the window to stop. The player also stops at the end of the video.
- `--lang` picks the subtitle language: `us`, `uk`, `de`, `fr`, `fc`, `es`, `la`, `it`, `nl`, `po`, `da` or `ru`.
- Without `--lang`, no subtitles are shown.
- The videos whose names end in `_de` are the German versions. Use `--lang de` with them.
- `--no-audio` plays the video without sound.
- `--subtitles FILE.srt` uses your own subtitle file instead.
- `--font NAME` draws the subtitles with another game font from the `fonts/` folder, for example `frontendfontlarge`. The default is `frontendfont`.

Subtitles are drawn with the game's own font. Long lines wrap at seven eighths of the video width (the original's exact width is not known yet). Where the game places them on the screen, and in which font, is not known yet.

## Save a single picture

```sh
build/chase-fmv artifacts/romfs/romfs/cutscenes/01_gameintro.moflex --lang us --time 3 --screenshot out.bmp
```

This saves the frame at 3 seconds, with its subtitle, and plays no sound. Without a screen, for example on a server, add `SDL_VIDEODRIVER=offscreen` in front of the command.

Without a sound card, run the player with `SDL_AUDIODRIVER=dummy`.
