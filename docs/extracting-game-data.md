# Extracting game data

You need your own decrypted `.cci` image of the game. This project never includes game data.

Back to the [documentation list](../README.md#more-documentation).

Every tool reads the image without changing it. The tools that take `--output` refuse to write into an output folder that already exists. If you want to run one again, choose a new folder or delete the old one first.

## 1. Extract the executable

```sh
python3 tools/extract_game.py path/to/game.cci --output artifacts/game
```

The tool stops with an error if the image is encrypted, cut short, or fails a hash check. On success it writes these files (and prints the manifest):

| File | Contents |
|---|---|
| `manifest.json` | offsets, SHA256 hashes and ARM segment addresses |
| `exheader.bin` | the extended header |
| `exefs/*.bin` | the ExeFS members (`banner`, `icon`, `logo` and the compressed code) |
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

Steps 3 to 6 and the tools in "More tools" use the project's C++ library. Build the project first (see [Building](building.md)), or set `CHASE_ASSETS_LIB` to the path of `libchase_assets_c.so`.

```sh
python3 tools/extract_fib.py artifacts/romfs/romfs/lego_city3ds.fib \
    --names artifacts/romfs/romfs/lego_city3ds.csv --output artifacts/fib
```

The files go to `artifacts/fib/files/`, and `fib-manifest.json` lists them. A file missing from the name list goes to `files/unnamed/<crc>.bin`.

## 4. Convert textures to PNG

```sh
python3 tools/convert_textures.py artifacts/fib/files --output artifacts/textures
```

- Add `--census` to only check the files without writing images.
- Only the largest size of each texture is written.
- Cubemaps become a vertical strip of six faces, in the order +X, -X, +Y, -Y, +Z, -Z. This is the order the game hands them to the graphics chip. The direction of each face was taken from the emulator, not checked on a console.

## 5. Convert sounds to WAV

```sh
python3 tools/convert_audio.py artifacts/fib/files --output artifacts/audio
```

- Add `--census` to only check the files.
- A sound with several parts becomes one WAV file with extra channels.
- `audio-manifest.json` lists each sound's sample rate, length and loop points.

## 6. Read the game text

```sh
python3 tools/convert_loc.py artifacts/fib/files/localisation/gametext_uk.loc \
    --extended artifacts/fib/files/localisation/extended_chars_utf8.txt > gametext_uk.json
```

- The output lists every text of one language, as UTF-8.
- `--extended` is needed for Russian and Polish letters.
- Most entries are named by a number, because the original names are only stored as hashes. Add `--names artifacts/game/code.bin` to recover the names that appear in the program.

To see how a text looks in one of the game's fonts:

```sh
python3 tools/convert_font.py artifacts/fib/files/fonts/frontendfont.tnf "Pause Menu" --output text.png \
    --extended artifacts/fib/files/localisation/extended_chars_utf8.txt
```

This needs the font image next to the `.tnf` file, which is where `extract_fib.py` puts it.

## 7. Look at models

Use the [model viewer](model-viewer.md). It can also save a picture without opening a window.

## 8. Look at the data tables

The files in `tables/` hold the game's data tables, for example characters, levels, animations and sound effects. `chase-tabledump` prints them as text, one row per line:

```sh
build/chase-tabledump tables/tableslevels.bxls
```

- It reads the asset archive in `artifacts/romfs`, like the model viewer. Build the project first (see [Building](building.md)).
- The files contain no column names, so the columns are numbered only.
- `-` marks a column that has no value stored in the file.

## More tools

These are for looking at the data. They write nothing, except to the standard output.

```sh
python3 tools/convert_levels.py --param-names artifacts/game/code.bin artifacts/fib/files/levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl
python3 tools/dump_dvlb.py artifacts/game/code.bin
```

- `convert_levels.py` lists the objects of a level file with their parameters. The parameter names are stored only as hashes. `--param-names` takes the decompressed program from step 1 to name them.
- `dump_dvlb.py` lists the shader programs inside the decompressed program (`code.bin`).
- The other programs of the project are in the [command reference](commands.md).
