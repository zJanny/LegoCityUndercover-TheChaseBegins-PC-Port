"""ctypes binding to the C++ asset runtime (build/libchase_assets_c.so, build/libchase_assets_c.dll on Windows).

Set CHASE_ASSETS_LIB to use a library outside the default build directory.
"""
import array
import ctypes
import os
from pathlib import Path

BUILD_DIRECTORY = Path(__file__).resolve().parents[1] / 'build'


def default_library(windows=os.name == 'nt'):
    return BUILD_DIRECTORY / ('libchase_assets_c.dll' if windows else 'libchase_assets_c.so')


DEFAULT_LIBRARY = default_library()
_library = None

ENCODINGS = ('PCM8', 'PCM16', 'DSP-ADPCM', 'IMA-ADPCM')

_TEXT_ENTRY = ctypes.CFUNCTYPE(None, ctypes.c_uint32, ctypes.c_char_p, ctypes.c_void_p)
_LEVEL_OBJECT = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p)
_LEVEL_PARAM = ctypes.CFUNCTYPE(None, ctypes.c_uint32, ctypes.c_uint16, ctypes.c_uint32, ctypes.c_size_t,
                                ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_size_t), ctypes.c_void_p)
_NAME_ENTRY = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_void_p)
_CENSUS_CLASS = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint64, ctypes.c_uint64, ctypes.c_uint32,
                                 ctypes.c_void_p)
_CENSUS_FUNCTION = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_int, ctypes.c_uint64, ctypes.c_uint32, ctypes.c_void_p)

SCRIPT_SUPPORT = ('none', 'implemented', 'stand-in')


class NativeUnavailable(RuntimeError):
    pass


def library_path():
    return Path(os.environ.get('CHASE_ASSETS_LIB', DEFAULT_LIBRARY))


def available():
    return library_path().exists()


def library():
    global _library
    if _library is None:
        path = library_path()
        if not path.exists():
            raise NativeUnavailable('C++ asset library not built: %s (run cmake --build build)' % path)
        lib = ctypes.CDLL(str(path))
        lib.chase_last_error.restype = ctypes.c_char_p
        lib.chase_skin_matrices.restype = ctypes.c_int
        lib.chase_skin_matrices.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_size_t,
                                            ctypes.c_double, ctypes.POINTER(ctypes.c_float), ctypes.c_size_t]
        lib.chase_fib_open.restype = ctypes.c_void_p
        lib.chase_fib_open.argtypes = [ctypes.c_char_p]
        lib.chase_fib_close.argtypes = [ctypes.c_void_p]
        lib.chase_fib_count.restype = ctypes.c_size_t
        lib.chase_fib_count.argtypes = [ctypes.c_void_p]
        lib.chase_fib_entry.restype = ctypes.c_int
        lib.chase_fib_entry.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_uint32)]
        lib.chase_fib_read.restype = ctypes.c_int
        lib.chase_fib_read.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t,
                                       ctypes.POINTER(ctypes.c_uint64)]
        lib.chase_texture_decode.restype = ctypes.c_void_p
        lib.chase_texture_decode.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
        lib.chase_texture_info.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_int32)]
        lib.chase_texture_rgba.restype = ctypes.c_void_p
        lib.chase_texture_rgba.argtypes = [ctypes.c_void_p]
        lib.chase_texture_free.argtypes = [ctypes.c_void_p]
        lib.chase_sound_decode.restype = ctypes.c_void_p
        lib.chase_sound_decode.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
        lib.chase_sound_info.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
        lib.chase_sound_encoding.restype = ctypes.c_int
        lib.chase_sound_encoding.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        lib.chase_sound_channel.restype = ctypes.c_void_p
        lib.chase_sound_channel.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        lib.chase_sound_free.argtypes = [ctypes.c_void_p]
        lib.chase_text_key.restype = ctypes.c_uint32
        lib.chase_text_key.argtypes = [ctypes.c_char_p]
        lib.chase_loc_decode.restype = ctypes.c_int
        lib.chase_loc_decode.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_size_t,
                                         _TEXT_ENTRY, ctypes.c_void_p]
        lib.chase_font_layout.restype = ctypes.c_int
        lib.chase_font_layout.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_char_p,
                                          ctypes.c_size_t, ctypes.POINTER(ctypes.c_int32), ctypes.c_size_t]
        lib.chase_level_walk.restype = ctypes.c_int
        lib.chase_level_walk.argtypes = [ctypes.c_char_p, ctypes.c_size_t, _LEVEL_OBJECT, _LEVEL_PARAM,
                                         ctypes.c_void_p]
        lib.chase_file_decoder.restype = ctypes.c_char_p
        lib.chase_file_decoder.argtypes = [ctypes.c_char_p]
        lib.chase_script_handlers.argtypes = [_NAME_ENTRY, ctypes.c_void_p]
        lib.chase_level_census.restype = ctypes.c_int
        lib.chase_level_census.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, _CENSUS_CLASS, _CENSUS_FUNCTION, ctypes.c_void_p]
        _library = lib
    return _library


def _error(lib):
    return ValueError(lib.chase_last_error().decode())


def skin_matrices(skeleton_data, animation_data, frame, max_joints=1024):
    """Per-joint skin matrices from chase::assets::skinMatrices, as 4x4 row lists."""
    lib = library()
    out = (ctypes.c_float * (16 * max_joints))()
    count = lib.chase_skin_matrices(skeleton_data, len(skeleton_data), animation_data, len(animation_data),
                                    float(frame), out, len(out))
    if count < 0:
        raise ValueError(lib.chase_last_error().decode())
    return [[list(out[16 * j + 4 * r:16 * j + 4 * r + 4]) for r in range(4)] for j in range(count)]


class FuseArchive:
    """A FUSE1.00 .fib archive read by chase::assets::FuseArchive."""

    def __init__(self, path):
        self._lib = library()
        self._handle = self._lib.chase_fib_open(str(path).encode())
        if not self._handle:
            raise _error(self._lib)
        count = self._lib.chase_fib_count(self._handle)
        raw = (ctypes.c_uint32 * 4)()
        self.entries = []
        for index in range(count):
            self._lib.chase_fib_entry(self._handle, index, raw)
            self.entries.append(tuple(raw))

    def read(self, index):
        """(data, stored size) of the entry at `index` in directory order."""
        size = self.entries[index][2]
        buffer = ctypes.create_string_buffer(size)
        stored = ctypes.c_uint64()
        if self._lib.chase_fib_read(self._handle, index, buffer, size, ctypes.byref(stored)) != 0:
            raise _error(self._lib)
        return buffer.raw, stored.value

    def close(self):
        if self._handle:
            self._lib.chase_fib_close(self._handle)
            self._handle = None

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()


def decode_texture(data):
    """Decode a .btga/.bpng/.bdds with chase::assets::decodeTexture.

    Returns (info, rgba): info has width, height (all rows, six faces of a cubemap stacked), format, faces and
    levels; rgba holds width * height * 4 bytes, top row first.
    """
    lib = library()
    handle = lib.chase_texture_decode(data, len(data))
    if not handle:
        raise _error(lib)
    try:
        raw = (ctypes.c_int32 * 5)()
        lib.chase_texture_info(handle, raw)
        info = dict(zip(('width', 'height', 'format', 'faces', 'levels'), raw))
        rgba = ctypes.string_at(lib.chase_texture_rgba(handle), info['width'] * info['height'] * 4)
    finally:
        lib.chase_texture_free(handle)
    return info, rgba


def decode_sound(data):
    """Decode a .bwav with chase::assets::decodeSound.

    Returns (info, channels): info has rate, loop, loop_start, frames, version and the encoding name of each part;
    channels is a list of signed 16-bit arrays of equal length.
    """
    lib = library()
    handle = lib.chase_sound_decode(data, len(data))
    if not handle:
        raise _error(lib)
    try:
        raw = (ctypes.c_uint32 * 7)()
        lib.chase_sound_info(handle, raw)
        rate, loop, loop_start, frames, channel_count, parts, version = raw
        channels = []
        for index in range(channel_count):
            samples = array.array('h')
            samples.frombytes(ctypes.string_at(lib.chase_sound_channel(handle, index), frames * 2))
            channels.append(samples)
        encodings = [ENCODINGS[lib.chase_sound_encoding(handle, part)] for part in range(parts)]
    finally:
        lib.chase_sound_free(handle)
    return {'rate': rate, 'loop': bool(loop), 'loop_start': loop_start, 'frames': frames, 'version': version,
            'encodings': encodings}, channels


def text_key(name):
    """Localisation key of an identifier (chase::assets::textKey)."""
    return library().chase_text_key(name.encode('latin-1'))


def decode_localisation(data, extended=''):
    """{key: text} of a .loc file, with the escapes for extended characters resolved to UTF-8 text."""
    lib = library()
    entries = {}

    def collect(key, text, _user):
        entries[key] = text.decode('utf-8')

    packed = extended.encode('utf-8')
    if lib.chase_loc_decode(data, len(data), packed, len(packed), _TEXT_ENTRY(collect), None) != 0:
        raise _error(lib)
    return entries


def layout_text(font_data, text, extended=''):
    """(line height, [(x, atlas x, atlas y, width, height)]) of `text` laid out in a .tnf font."""
    lib = library()
    capacity = 1 + 5 * max(1, len(text))
    out = (ctypes.c_int32 * capacity)()
    packed = extended.encode('utf-8')
    count = lib.chase_font_layout(font_data, len(font_data), text.encode('utf-8'), packed, len(packed), out, capacity)
    if count < 0:
        raise _error(lib)
    return out[0], [tuple(out[1 + 5 * i:6 + 5 * i]) for i in range(count)]


def level_objects(data):
    """[(class name, instance, room, [(hash, type, value, [block bytes])])] of a .blvl level."""
    lib = library()
    objects = []

    def on_object(class_name, instance, room, _user):
        objects.append((class_name.decode('latin-1'), instance.decode('latin-1'), room.decode('latin-1'), []))

    def on_param(name_hash, kind, value, count, blocks, sizes, _user):
        objects[-1][3].append((name_hash, kind, value, [ctypes.string_at(blocks[i], sizes[i]) for i in range(count)]))

    if lib.chase_level_walk(data, len(data), _LEVEL_OBJECT(on_object), _LEVEL_PARAM(on_param), None) < 0:
        raise _error(lib)
    return objects


def file_decoder(archive_path):
    """Name of the chase_assets function that decodes an archive file, or '' when there is none."""
    return library().chase_file_decoder(archive_path.encode('latin-1')).decode()


def script_handlers():
    """Lower-case names of the script functions the level-start executor handles."""
    names = []

    def collect(name, _user):
        names.append(name.decode('latin-1'))

    library().chase_script_handlers(_NAME_ENTRY(collect), None)
    return names


def level_census(fib_path, names_path, main_levels_only=False):
    """Census of the level files (levels/levels/*.blvl, or only the *_main.blvl ones), computed by chase_runtime and
    chase_assets.

    Returns (level files, classes, functions). classes maps a class name to a dict with behaviours (list of names),
    instances, behaving (instances with a behaviour) and levels; functions maps a lower-case script function name to
    a dict with support ('none', 'implemented' or 'stand-in'), calls and levels.
    """
    lib = library()
    classes = {}
    functions = {}

    def on_class(name, behaviours, instances, behaving, levels, _user):
        text = behaviours.decode()
        classes[name.decode('latin-1')] = {'behaviours': text.split(',') if text else [], 'instances': instances,
                                           'behaving': behaving, 'levels': levels}

    def on_function(name, support, calls, levels, _user):
        functions[name.decode('latin-1')] = {'support': SCRIPT_SUPPORT[support], 'calls': calls, 'levels': levels}

    count = lib.chase_level_census(str(fib_path).encode(), str(names_path).encode(), int(main_levels_only), _CENSUS_CLASS(on_class),
                                   _CENSUS_FUNCTION(on_function), None)
    if count < 0:
        raise _error(lib)
    return count, classes, functions
