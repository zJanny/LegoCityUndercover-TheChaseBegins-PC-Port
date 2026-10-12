"""ctypes binding to the C++ asset runtime (build/libchase_assets_c.so).

Set CHASE_ASSETS_LIB to use a library outside the default build directory.
"""
import ctypes
import os
from pathlib import Path

DEFAULT_LIBRARY = Path(__file__).resolve().parents[1] / 'build' / 'libchase_assets_c.so'
_library = None


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
        _library = lib
    return _library


def skin_matrices(skeleton_data, animation_data, frame, max_joints=1024):
    """Per-joint skin matrices from chase::assets::skinMatrices, as 4x4 row lists."""
    lib = library()
    out = (ctypes.c_float * (16 * max_joints))()
    count = lib.chase_skin_matrices(skeleton_data, len(skeleton_data), animation_data, len(animation_data),
                                    float(frame), out, len(out))
    if count < 0:
        raise ValueError(lib.chase_last_error().decode())
    return [[list(out[16 * j + 4 * r:16 * j + 4 * r + 4]) for r in range(4)] for j in range(count)]
