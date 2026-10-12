# Sound container, CWAV and mixer

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-12T15:45+02:00 — Sound container decoded (.bwav)

- The .bwav wrapper starts with u32 version 0x400 or 0x600.
  - v4 (1263 files): one standard CWAV at 0x44.
  - v6 (98 files, all music): one mono CWAV per channel, back to back from 0x4c, with the second CWAV starting exactly where the first ends.
- The other wrapper fields (rate copy, sample/loop counts, a 4-byte value before CWAV that may be a hash) are not interpreted. The CWAV structure is the public BCWAV layout.
- Added tools/convert_audio.py (PCM8/PCM16/DSP-ADPCM/IMA-ADPCM to WAV, v6 parts merged as channels, loop points kept in audio-manifest.json) and tests/test_convert_audio.py (4 tests: DSP frame arithmetic including floor shift, PCM8/16, IMA, two-part merge + WAV output, malformed inputs).
- `python3 tools/convert_audio.py --census artifacts/fib/files` — 1361/1361 parsed; all DSP-ADPCM: 12000 Hz 1068 (+98 two-part), 11999 69, 12001 75; the rest are between 7025 and 12003 Hz.
- `python3 tools/convert_audio.py artifacts/fib/files --output artifacts/audio` — PASS, 1361 WAVs, 0 failures, 40.4 s.
- Checks:
  - The first DSP frame header equals the context pred/scale byte (0x39 in pickup_positive_1_notloop), which confirms data alignment at DATA+0x20.
  - ffmpeg astats: voice-over and music peak at about −0.2 to −2.4 dBFS with RMS between −10 and −23 dB, which reads as normalized audio rather than noise. Zero-crossing rate is 0.06 for the music sample.
  - music/act_01 decodes to silence because its frames are genuinely all zero, so it is a placeholder.
- `python3 -m unittest discover -s tests -p 'test_*.py' -v` — PASS 30/30.

## 2026-10-12T18:25+02:00 — S4a complete (CWAV in C++), S4d mixer and sound test

- Added src/assets/audio.cpp: decodeSound splits the .bwav through parsePooled (0x2c record plus CWAV parts) and decodes PCM8/PCM16/DSP-ADPCM/IMA. The DSP sum is computed in 64-bit and floor-shifted, as in the reference.
- assets-core gains a synthetic DSP vector ([112, −16, −16], matching the Python test) and a malformed-record rejection.
- assets-real-data: 1361 sounds / 122501647 samples / 123 looping, equal to audio-manifest.json. The FNV-1a over act_bass_break's interleaved PCM is 0xdcbf1f85, bit-identical to the Python WAV output.
- Added the chase_audio library (src/audio/mixer.{hpp,cpp}, no SDL): voices with linear resampling, mono→stereo, gain, loop wrap to loopStart, and one-shot expiry.
- Added CTest `audio-mixer` (tests/mixer_test.cpp).
- Added chase-soundtest (SDL audio callback at 48 kHz float stereo). It plays archive sounds and keeps loops going for 10 s by default or `--seconds`.
- Added the opt-in CTest `soundtest-real-smoke` (SDL_AUDIODRIVER=dummy).
- `ctest --test-dir build --output-on-failure` — PASS 7/7; 0 warnings.

