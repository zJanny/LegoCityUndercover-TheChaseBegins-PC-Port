#!/usr/bin/env python3
"""Decode TT Fusion .bwav sounds (wrapped Nintendo CWAV) from an unpacked .fib to WAV.

The sounds are decoded by the C++ asset runtime (chase::assets::decodeSound) through
tools/chase_native.py; build the project first. Parts become extra channels. Loop points
are kept in audio-manifest.json. The format is described in docs/re/audio.md.
"""
import argparse
import array
import json
import sys
import wave
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import chase_native as native  # noqa: E402


def decode(data):
    """Return (manifest entry, channels) for a whole .bwav."""
    sound, channels = native.decode_sound(data)
    first = sound['encodings'][0]
    others = sorted(set(sound['encodings']) - {first})
    info = {'encoding': first + ''.join('+' + name for name in others), 'loop': sound['loop'],
            'rate': sound['rate'], 'loop_start': sound['loop_start'], 'samples': sound['frames'],
            'version': '%08x' % sound['version'], 'parts': len(sound['encodings']), 'channels': len(channels)}
    return info, channels


def write_wav(path, rate, channels):
    frames = array.array('h', bytes(2 * len(channels) * len(channels[0])))
    for index, channel in enumerate(channels):
        frames[index::len(channels)] = channel
    with wave.open(str(path), 'wb') as out:
        out.setnchannels(len(channels))
        out.setsampwidth(2)
        out.setframerate(rate)
        out.writeframes(frames.tobytes())


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', type=Path, nargs='+', help='.bwav files or directories to scan')
    parser.add_argument('--output', type=Path, help='WAV output directory (must not exist)')
    parser.add_argument('--census', action='store_true', help='only decode and print a summary')
    args = parser.parse_args()
    if not args.census and not args.output:
        parser.error('--output is required unless --census is given')
    try:
        native.library()
    except native.NativeUnavailable as exc:
        parser.exit(1, str(exc) + '\n')
    files = []
    for item in args.inputs:
        files += sorted(item.rglob('*.bwav')) if item.is_dir() else [item]
    if args.output:
        if args.output.exists():
            parser.exit(1, 'output already exists; choose a new directory\n')
        args.output.mkdir(parents=True)
    roots = [i for i in args.inputs if i.is_dir()]
    census, failures, sounds = {}, [], {}
    for path in files:
        try:
            info, channels = decode(path.read_bytes())
            if not args.census:
                base = next((r for r in roots if r in path.parents), path.parent)
                rel = path.relative_to(base)
                target = args.output / rel.with_suffix('.wav')
                target.parent.mkdir(parents=True, exist_ok=True)
                write_wav(target, info['rate'], channels)
                sounds[str(rel)] = info
        except (ValueError, OSError) as exc:
            failures.append({'path': str(path), 'error': str(exc)})
            continue
        key = '%s %d Hz x%d' % (info['encoding'], info['rate'], info['parts'])
        census[key] = census.get(key, 0) + 1
    if args.output:
        (args.output / 'audio-manifest.json').write_text(json.dumps(sounds, indent=1) + '\n')
    print(json.dumps({'files': len(files), 'kinds': dict(sorted(census.items())),
                      'failures': failures[:20], 'failure_count': len(failures)}, indent=1))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
