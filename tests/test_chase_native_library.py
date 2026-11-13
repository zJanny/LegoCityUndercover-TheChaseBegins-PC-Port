import os
import sys
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'tools'))

import chase_native  # noqa: E402

BUILD = Path(__file__).resolve().parent.parent / 'build'


class DefaultLibraryTest(unittest.TestCase):
    def test_linux_default_is_the_shared_object_of_the_build(self):
        self.assertEqual(chase_native.default_library(windows=False), BUILD / 'libchase_assets_c.so')

    def test_windows_default_is_the_dll_of_the_build(self):
        self.assertEqual(chase_native.default_library(windows=True), BUILD / 'libchase_assets_c.dll')

    def test_platform_decides_the_module_default(self):
        self.assertEqual(chase_native.DEFAULT_LIBRARY, chase_native.default_library(windows=os.name == 'nt'))

    def test_environment_variable_names_another_library(self):
        with mock.patch.dict(os.environ, {'CHASE_ASSETS_LIB': 'elsewhere/lib.dll'}):
            self.assertEqual(chase_native.library_path(), Path('elsewhere/lib.dll'))

    def test_without_the_variable_the_default_is_used(self):
        with mock.patch.dict(os.environ):
            os.environ.pop('CHASE_ASSETS_LIB', None)
            self.assertEqual(chase_native.library_path(), chase_native.DEFAULT_LIBRARY)

    def test_missing_library_is_reported_with_its_path(self):
        with mock.patch.dict(os.environ, {'CHASE_ASSETS_LIB': 'missing/lib.dll'}), \
                mock.patch.object(chase_native, '_library', None):
            self.assertFalse(chase_native.available())
            with self.assertRaisesRegex(chase_native.NativeUnavailable, 'C\\+\\+ asset library not built: missing.lib.dll'):
                chase_native.library()


@unittest.skipUnless((BUILD / chase_native.DEFAULT_LIBRARY.name).exists(), 'C++ asset library not built')
class BuiltLibraryTest(unittest.TestCase):
    def test_default_library_loads_and_answers(self):
        with mock.patch.dict(os.environ):
            os.environ.pop('CHASE_ASSETS_LIB', None)
            with mock.patch.object(chase_native, '_library', None):
                self.assertTrue(chase_native.available())
                self.assertEqual(chase_native.text_key('ok'), 0x4f13bc)


if __name__ == '__main__':
    unittest.main()
