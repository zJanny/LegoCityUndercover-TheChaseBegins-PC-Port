import importlib.util
import struct
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('azahar_oracle', Path(__file__).parents[1] / 'tools/azahar_oracle.py')
oracle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle)


class StickCaveTest(unittest.TestCase):
    def test_cave_words_match_the_assembler(self):
        words = struct.unpack('<6I', oracle.STICK_CAVE_CODE)
        # llvm-mc -triple=armv6k -show-encoding: ldr r0,[pc,#40] / strh r0,[r4,#14] / ldr r0,[pc,#36] / strh r0,[r4,#12] / ldrsh r0,[r4,#14]
        self.assertEqual(words[:5], (0xe59f0028, 0xe1c400be, 0xe59f0024, 0xe1c400bc, 0xe1d400fe))

    def test_cave_resumes_after_the_replaced_instruction(self):
        word = struct.unpack('<6I', oracle.STICK_CAVE_CODE)[5]
        target = oracle.STICK_CAVE + 20 + 8 + (((word & 0xffffff) ^ 0x800000) - 0x800000) * 4
        self.assertEqual(target, oracle.PAD_STICK + 4)

    def test_patch_branches_into_the_cave(self):
        word = oracle.STICK_LOAD_PATCHED
        target = oracle.PAD_STICK + 8 + (((word & 0xffffff) ^ 0x800000) - 0x800000) * 4
        self.assertEqual(target, oracle.STICK_CAVE)

    def test_mailbox_words_lie_inside_the_cave_area(self):
        self.assertGreaterEqual(oracle.STICK_MAILBOX_Y, oracle.STICK_CAVE + len(oracle.STICK_CAVE_CODE))
        self.assertEqual(oracle.STICK_MAILBOX_X, oracle.STICK_MAILBOX_Y + 4)
        self.assertLess(oracle.STICK_MAILBOX_X + 4, 0x357000)


if __name__ == '__main__':
    unittest.main()
