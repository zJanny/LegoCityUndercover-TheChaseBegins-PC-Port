# Extracting game data

You need your own decrypted `.cci` image of the game. This project never includes game data.

Every tool reads the image without changing it. Every tool refuses to write into an output folder that already exists. If you want to run a tool again, choose a new folder or delete the old one first.

## 1. Extract the executable

```sh
python3 tools/extract_game.py path/to/game.cci --output artifacts/game
```

The tool stops with an error if the image is encrypted, cut short, or fails a hash check. On success it writes these files:

| File | Contents |
|---|---|
| `manifest.json` | offsets, SHA256 hashes and ARM segment addresses |
| `exheader.bin` | the extended header |
| `exefs/*.bin` | the ExeFS members |
| `code.bin` | the decompressed ARM code |
| `game.arm.elf` | a wrapper for disassemblers. It is not an original or runnable program. |

## 2. Extract the game files (RomFS)

```sh
python3 tools/extract_romfs.py path/to/game.cci --output artifacts/romfs
```

The tool checks all hashes before it writes anything. It writes `romfs/` and `romfs-manifest.json`.

Most other tools, the model viewer and the sound test expect the files in `artifacts/romfs`.

## 3. Unpack the asset archive

Almost every asset is inside one archive, `romfs/lego_city3ds.fib`. The cutscene videos (`.moflex`) are the exception.

```sh
python3 tools/extract_fib.py artifacts/romfs/romfs/lego_city3ds.fib \
    --names artifacts/romfs/romfs/lego_city3ds.csv --output artifacts/fib
```

The files go to `artifacts/fib/files/`. A file missing from the name list goes to `files/unnamed/<crc>.bin`.

## 4. Convert textures to PNG

```sh
python3 tools/convert_textures.py artifacts/fib/files --output artifacts/textures
```

- Add `--census` to only check the files without writing images.
- Only the largest size of each texture is written.
- Cubemaps become a vertical strip of six faces. Which face is which is not confirmed yet.

## 5. Convert sounds to WAV

```sh
python3 tools/convert_audio.py artifacts/fib/files --output artifacts/audio
```

- Add `--census` to only check the files.
- Music with several channels becomes one WAV file.
- `audio-manifest.json` lists each sound's sample rate, length and loop points.

## 6. Read the game text

```sh
python3 tools/convert_loc.py artifacts/fib/files/localisation/gametext_uk.loc \
    --extended artifacts/fib/files/localisation/extended_chars_utf8.txt > gametext_uk.json
```

- The output lists every text of one language, as UTF-8.
- `--extended` is needed for Russian and Polish letters.
- Most entries are named by a number, because the original names are only stored as hashes. Add `--names artifacts/game/code.bin` to recover the names that appear in the program.

## 7. Look at models

`preview_model.py` draws a model into a PNG image. It is slow but needs no graphics card.

```sh
python3 tools/preview_model.py artifacts/fib/files/models/char_bear/char_bear.bfnmdl \
    --assets artifacts/fib/files --output bear.png
```

To pose a character, add its skeleton, an animation and a frame number:

```sh
    --skeleton artifacts/fib/files/models/char_bear/char_bear.bfnskl \
    --anim artifacts/fib/files/models/char_bear/walk.bfnanm --frame 12
```

- `--exclude WORD` hides parts whose texture name contains WORD, for example `sky`.
- `--yaw` and `--pitch` turn the camera, in degrees.

For a faster, interactive view, use the [model viewer](model-viewer.md).

## Checking files in bulk

These commands check every file of a type and print a summary. They write nothing.

```sh
python3 tools/tt_resource.py --census artifacts/fib/files       # all b* resource files
python3 tools/convert_models.py --census artifacts/fib/files    # models
python3 tools/convert_levels.py --census artifacts/fib/files    # level files
python3 tools/convert_anims.py --census artifacts/fib/files     # skeletons and animations
python3 tools/convert_nav.py --census artifacts/fib/files       # navigation grids
```

To print the first vertices of one model, use `python3 tools/convert_models.py --vertices 3 path/to/model.bfnmdl`.
