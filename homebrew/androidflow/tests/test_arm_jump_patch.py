import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from patch_arm_jmp import PACKAGE, ORIGINAL, REPLACEMENT, LOG_ANCHOR, patch_source

FIXTURE = (
    'static const uint32_t small_code[] = {\n'
    '};\n'
    + ORIGINAL + '\n'
    'static void *small_in_guest(const char *name, uint64_t thunk) {\n'
    '    if (is_setjmp(name)) return &code[70];\n'
    '    if (is_longjmp(name)) return &code[84];\n'
    '    return (void *)thunk;\n'
    '}\n'
    'void *arm_import(const char *name, int is_data) {\n'
    + LOG_ANCHOR + '\n'
    '                  ((is_setjmp(name) || is_longjmp(name)) && droid_game_in(ARM_JMP_GAMES))))\n'
    '        return small_in_guest(name, thunk);\n'
    '    return (void *)thunk;\n'
    '}\n'
)

class ArmJumpTests(unittest.TestCase):
    def test_preserves_existing_game_and_adds_fruity(self):
        new = patch_source(FIXTURE)
        self.assertIn(REPLACEMENT, new)
        self.assertIn('com.king.candycrushsaga', new)
        self.assertIn(PACKAGE, new)
        self.assertIn('if (is_setjmp(name)) return &code[70];', new)
        self.assertIn('if (is_longjmp(name)) return &code[84];', new)
        self.assertIn('droid_game_in(ARM_JMP_GAMES)', new)
        self.assertIn('androidflow: ARM64-native %s enabled for Fruity Game', new)

    def test_source_guard_prevents_double_patch(self):
        new = patch_source(FIXTURE)
        with self.assertRaises(RuntimeError):
            patch_source(new)

    def test_native_guest_stubs_required(self):
        with self.assertRaises(RuntimeError):
            patch_source(FIXTURE.replace('&code[84]', '&code[95]'))

    def test_other_packages_unaffected(self):
        whitelist = REPLACEMENT.split('"')[1].split(',')
        matches = lambda pkg: any(pkg.startswith(name) for name in whitelist)
        self.assertTrue(matches(PACKAGE))
        self.assertTrue(matches("com.king.candycrushsaga"))
        self.assertFalse(matches("com.robtopx.geometryjumplite"))
        self.assertFalse(matches("com.example.other"))

if __name__ == "__main__":
    unittest.main()
